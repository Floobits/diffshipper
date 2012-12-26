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
    free(bufs);
}


void save_buf(buf_t *buf) {
    char *full_path;
    int fd;
    ssize_t bytes_written;
    ignore_path(buf->path);
    ds_asprintf(&full_path, "%s/%s", opts.path, buf->path);
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
