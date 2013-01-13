#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include "dmp.h"

#include "buf.h"
#include "log.h"
#include "mmap.h"
#include "options.h"
#include "util.h"


void init_bufs() {
    bufs_len = 0;
    bufs_size = BUFS_START_SIZE;
    bufs = calloc(bufs_size, sizeof(buf_t*));
}


void cleanup_bufs() {
    size_t i;
    for (i = 0; i < bufs_len; i++) {
        free(bufs[i]);
    }
    free(bufs);
}


void add_buf_to_bufs(buf_t *buf) {
    size_t i;

    bufs_len++;

    if (bufs_len > bufs_size) {
        bufs_size *= 1.5;
        bufs = realloc(bufs, bufs_size * sizeof(buf_t*));
    }

    for (i = bufs_len - 1; i > 0; i--) {
        if (buf->id > bufs[i-1]->id) {
            break;
        }
        bufs[i] = bufs[i-1];
    }
    bufs[i] = buf;
    log_debug("added buf id %i to position %i", buf->id, i);
}


static int binary_search(const int buf_id, int start, int end) {
    int mid;

    if (start == end) {
        return -1;
    }

    mid = (start + end) / 2; /* can screw up on arrays with > 2 billion elements */

    if (buf_id < bufs[mid]->id) {
        return binary_search(buf_id, start, mid);
    } else if (buf_id > bufs[mid]->id) {
        return binary_search(buf_id, mid + 1, end);
    }

    return mid;
}


buf_t *get_buf_by_id(const int buf_id) {
    int index;
    buf_t *buf = NULL;

    index = binary_search(buf_id, 0, bufs_len);
    if (index >= 0) {
        buf = bufs[index];
    }

    return buf;
}


buf_t *get_buf(const char *path) {
    size_t i;
    buf_t *buf = NULL;

    for (i = 0; i < bufs_len; i++) {
        log_debug("path %s buf path %s", path, bufs[i]->path);
        if (strcmp(bufs[i]->path, path) == 0) {
            buf = bufs[i];
            break;
        }
    }

    return buf;
}


void save_buf(buf_t *buf) {
    char *full_path;
    int fd;
    int rv;
    ssize_t bytes_written;
    log_debug("saving buf %i path %s", buf->id, buf->path);
    ds_asprintf(&full_path, "%s/%s", opts.path, buf->path);

    char *buf_path = strdup(full_path);
    int i;
    for (i = strlen(buf_path); i > 0; i--) {
        if (buf_path[i] == '/') {
            buf_path[i] = '\0';
            break;
        }
    }
    if (strlen(buf_path) > 0) {
        rv = run_cmd("mkdir -p %s", buf_path);
        if (rv)
            die("error creating temp directory %s", buf_path);
    }

    ignore_path(full_path);
    fd = open(full_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
        die("Error opening file %s: %s", full_path, strerror(errno));

    buf->len = strlen(buf->buf); /* TODO: not binary-safe */
    if (ftruncate(fd, buf->len) != 0)
        die("resizing %s failed", full_path);

    bytes_written = write(fd, buf->buf, buf->len);
    log_debug("wrote %i bytes to %s", bytes_written, buf->path);
    close(fd);
    free(buf_path);
    free(full_path);
}


int apply_patch(buf_t *buf, char *patch_text) {
    int rv;

    lli_t len = 0;
    size_t offset = 0;
    int add_len, add_off, del_len, del_off;

    char *patch_header = strdup(patch_text);
    char *patch_header_end = strchr(patch_header, '\n');
    char *patch_body = strchr(patch_text, '\n');
    if (patch_header_end != NULL) {
        *patch_header_end = '\0';
    } else {
        die("Couldn't find newline in patch!");
    }
    rv = sscanf(patch_header, "@@ -%i,%i +%i,%i @@", &del_off, &del_len, &add_off, &add_len);
    if (rv != 4) {
        rv = sscanf(patch_header, "@@ -%i +%i,%i @@", &del_off, &add_off, &add_len);
        del_len = 0;
        if (rv != 3) {
            rv = sscanf(patch_header, "@@ -%i,%i +%i @@", &del_off, &del_len, &add_off);
            add_len = 0;
        }
        if (rv != 3) {
            log_debug("rv %i @@ -%i,%i +%i,%i @@", rv, del_off, del_len, add_off, add_len);
            die("Couldn't sscanf patch");
        }
    }
    free(patch_header);

    log_debug("rv %i @@ -%i,%i +%i,%i @@", rv, del_off, del_len, add_off, add_len);
    len = add_len - del_len;
    offset = del_off - 1;
    if (del_off != add_off) {
        die("FUCK");
    }

    log_debug("patching %s: %li bytes at %lu", buf->path, len, offset);
    log_debug("Patch body: %s", patch_body);
    char *patch_row;
    char *insert_data = NULL;
    char *unescaped;
    char *escaped_data;
    char *escaped_data_end;
    patch_row = strchr(patch_body, '\n');
    int ignore_context = 0;
    while (patch_row != NULL) {
        patch_row++;
        log_debug("patch row: %s", patch_row);
        switch (patch_row[0]) {
            case ' ':
                if (ignore_context)
                    break;
                escaped_data = strdup(patch_row + 1);
                escaped_data_end = strchr(escaped_data, '\n');
                if (escaped_data_end != NULL) {
                    *escaped_data_end = '\0';
                }
                unescaped = unescape_data(escaped_data);
                offset += strlen(unescaped);
                free(unescaped);
                free(escaped_data);
                log_debug("offset: %lu", offset);
            break;
            case '+':
                if (insert_data != NULL) {
                    die("insert data already contained data: %s", insert_data);
                }
                escaped_data = strdup(patch_row + 1);
                escaped_data_end = strchr(escaped_data, '\n');
                if (escaped_data_end != NULL) {
                    *escaped_data_end = '\0';
                }
                insert_data = unescape_data(escaped_data);
                free(escaped_data);
                ignore_context = 1;
            break;
            case '-':
                ignore_context = 1;
            break;
            default:
                if (strlen(patch_row) != 0) {
                    die("BAD PATCH");
                }
        }
        patch_row = strchr(patch_row, '\n');
    }

    if (buf->len < offset)
        die("%s is too small to apply patch to! (%lu bytes, need %lu bytes)", buf->path, buf->len, offset);

    buf->buf = realloc(buf->buf, buf->len + len + 1);
    void *op_point = buf->buf + offset;
    if (len > 0) {
        log_debug("memmove(%u, %u, %u)", (size_t)(op_point + len), (size_t)op_point, buf->len - offset);
        memmove(op_point + len, op_point, buf->len - offset);
        if (insert_data) {
            if ((lli_t)strlen(insert_data) != len) {
                die("insert data is %i bytes but we want to insert %li", strlen(insert_data), len);
            }
            memcpy(op_point, insert_data, len);
        } else {
            die("New length is longer but we couldn't find any data to insert!");
        }
    } else if (len < 0) {
        memmove(op_point, op_point - len, buf->len + len - offset);
    }
    buf->len += len;
    buf->buf[buf->len] = '\0';

    log_debug("resized %s to %u bytes", buf->path, buf->len);
    if (insert_data) {
        free(insert_data);
    }

    free(buf->md5);
    buf->md5 = md5(buf->buf, buf->len);

    return 1;
}
