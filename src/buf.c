#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include "buf.h"
#include "log.h"
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
    bytes_written = write(fd, buf->buf, strlen(buf->buf)); /* TODO: not binary-safe */
    log_debug("wrote %i bytes to %s", bytes_written, buf->path);
    close(fd);
    free(full_path);
}


void apply_patch(buf_t *buf, char *patch_text) {
    
}
