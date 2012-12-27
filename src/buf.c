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
        if (buf->id < bufs[i]->id) {
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
    if (index > 0) {
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
    ssize_t bytes_written;
    log_debug("saving buf %i path %s", buf->id, buf->path);
    /* TODO: not sure if this is right. shouldn't we be writing to both files? */
    ds_asprintf(&full_path, "%s/%s", opts.path, buf->path);
    ignore_path(full_path);
    fd = open(full_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        die("Error opening file %s: %s", buf->path, strerror(errno));
    }
    buf->len = strlen(buf->buf); /* TODO: not binary-safe */
    bytes_written = write(fd, buf->buf, buf->len);
    log_debug("wrote %i bytes to %s", bytes_written, buf->path);
    close(fd);
    free(full_path);
}


/*void apply_diff(char *path, dmp_operation_t op, char *buf, size_t len, off_t offset)*/
void apply_patch(buf_t *buf, char *patch_text) {
    struct stat file_stats;
    int rv;
    mmapped_file_t *mf;
    char *path = buf->path;

    lli_t len = 0;
    lli_t offset = 0;
    int op = DMP_DIFF_INSERT;

    log_debug("patching %s: %lu bytes at %lu", path, len, offset);

    rv = lstat(path, &file_stats);
    if (rv) {
        die("Error lstat()ing file %s.", path);
    }

    off_t file_size = file_stats.st_size;
    if (op == DMP_DIFF_INSERT) {
        file_size += len;
    }
    mf = mmap_file(path, file_size, PROT_WRITE | PROT_READ, 0);
    if (!mf) {
        /* return to make the static analyzer happy */
        return die("mmap of %s failed!", path);
    }

    if (mf->len < offset)
        die("%s is too small to apply patch to!", path);

    void *op_point = mf->buf + offset;
    if (op == DMP_DIFF_INSERT) {
        if (ftruncate(mf->fd, file_size) != 0) {
            die("resizing %s failed", path);
        }
        log_debug("memmove(%u, %u, %u)", (size_t)(op_point + len), (size_t)op_point, (file_size - len) - offset);
        memmove(op_point + len, op_point, (file_size - len) - offset);
        memcpy(op_point, buf, len);
    } else if (op == DMP_DIFF_DELETE) {
        file_size = mf->len - len;
        memmove(op_point, op_point + len, file_size - offset);
        if (ftruncate(mf->fd, file_size) != 0) {
            die("resizing %s failed", path);
        }
    }
    log_debug("resized %s to %u bytes", path, file_size);
    rv = msync(mf->buf, file_size, MS_SYNC);
    log_debug("rv %i wrote %i bytes to %s", rv, file_size, path);
    munmap_file(mf);
    free(mf);
}
