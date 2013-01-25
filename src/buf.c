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


static void free_buf(buf_t *buf) {
    free(buf->buf);
    free(buf->md5);
    free(buf->path);
    free(buf);
}


void cleanup_bufs() {
    size_t i;
    for (i = 0; i < bufs_len; i++) {
        free_buf(bufs[i]);
    }
    free(bufs);
}


char *get_full_path(char *rel_path) {
    char *full_path;
    ds_asprintf(&full_path, "%s/%s", opts.path, rel_path);
    return full_path;
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


void delete_buf(buf_t *buf) {
    int index;
    size_t i;

    index = binary_search(buf->id, 0, bufs_len);
    if (index < 0) {
        die("couldn't find buf id %i", buf->id);
    }

    free_buf(bufs[index]);

    if (bufs_len > 0) {
        /* Shift everything after */
        bufs_len--;
        for (i = index; i < bufs_len; i++) {
            bufs[i] = bufs[i+1];
        }
    }
}


void save_buf(buf_t *buf) {
    char *full_path;
    int fd;
    int rv;
    ssize_t bytes_written;
    log_debug("saving buf %i path %s", buf->id, buf->path);
    full_path = get_full_path(buf->path);

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
            die("error creating directory %s", buf_path);
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
    size_t add_len, add_off, del_len, del_off;

    char *patch_header = strdup(patch_text);
    char *patch_header_end = strchr(patch_header, '\n');
    char *patch_body = strchr(patch_text, '\n');

    if (patch_header_end != NULL)
        *patch_header_end = '\0';
    else
        die("Couldn't find newline in patch!");

    rv = sscanf(patch_header, "@@ -%lu,%lu +%lu,%lu @@", &del_off, &del_len, &add_off, &add_len);
    if (rv != 4) {
        rv = sscanf(patch_header, "@@ -%lu +%lu,%lu @@", &del_off, &add_off, &add_len);
        del_len = 1;
        if (rv != 3) {
            rv = sscanf(patch_header, "@@ -%lu,%lu +%lu @@", &del_off, &del_len, &add_off);
            add_len = 1;
        }
        if (rv != 3)
            die("Couldn't sscanf patch: rv %i @@ -%lu,%lu +%lu,%lu @@", rv, del_off, del_len, add_off, add_len);
    }
    free(patch_header);

    log_debug("rv %i @@ -%i,%i +%i,%i @@", rv, del_off, del_len, add_off, add_len);
    /* Patch offsets start at 1 */
    add_off--;
    del_off--;
    log_debug("patching %s: adding %lu bytes at %lu, deleting %lu bytes at %lu", buf->path, add_len, add_off, del_len, del_off);
    log_debug("Patch body: %s", patch_body);

    char *patch_row = strchr(patch_body, '\n');
    char *unescaped;
    char *escaped_data;
    char *escaped_data_end;
    size_t offset = 0;
    size_t change_offset = 0;
    void *op_point;
    while (patch_row != NULL) {
        patch_row++;
        log_debug("patch row: \"%s\"", patch_row);
        if (*patch_row == '\0')
            break;
        escaped_data = strdup(patch_row + 1);
        escaped_data_end = strchr(escaped_data, '\n');
        if (escaped_data_end != NULL) {
            *escaped_data_end = '\0';
        }
        unescaped = unescape_data(escaped_data);

        switch (patch_row[0]) {
            case ' ':
                offset = strlen(unescaped) + change_offset;
                log_debug("offset: %lu", offset);
            break;
            case '+':
                if (unescaped == NULL) {
                    die("No data to insert!");
                    return -1; /* make static analyzer happy */
                }
                add_len = strlen(unescaped);
                change_offset = add_len;
                add_off += offset;
                buf->len += add_len;
                log_debug("new buf len is %lu", buf->len);
                buf->buf = realloc(buf->buf, buf->len);
                op_point = buf->buf + add_off;
                log_debug("memmove(%u, %u, %u)", add_off + add_len, add_off, add_off);
                memmove(op_point + add_len, op_point, buf->len - add_off);
                memcpy(op_point, unescaped, add_len);
            break;
            case '-':
                del_len = strlen(unescaped);
                del_off += offset;
                op_point = buf->buf + del_off;
                log_debug("memmove(%u, %u, %u)", del_off, del_off + del_len, del_off);
                memmove(op_point, op_point + del_len, buf->len - del_off);
                buf->len -= del_len;
                log_debug("new buf len is %lu", buf->len);
                buf->buf = realloc(buf->buf, buf->len);
            break;
            case '@':
                /* Multi-patch. Someone did a big search-and-replace. */
                free(unescaped);
                free(escaped_data);
                return apply_patch(buf, patch_row);
            default:
                die("BAD PATCH");
        }
        free(unescaped);
        free(escaped_data);
        patch_row = strchr(patch_row, '\n');
    }

    if (buf->buf[buf->len] != '\0') {
        log_debug("buf: %s", buf->buf);
        log_err("OMG buf isn't null terminated");
        buf->buf[buf->len] = '\0';
    }

    free(buf->md5);
    buf->md5 = md5(buf->buf, buf->len);

    log_debug("buf: %s", buf->buf);

    return 1;
}
