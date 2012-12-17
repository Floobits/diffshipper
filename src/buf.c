#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/uio.h>

#include "buf.h"
#include "log.h"
#include "options.h"
#include "util.h"


void set_buf(buf_t *buf) {
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
}
