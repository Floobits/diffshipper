#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mmap.h"
#include "log.h"


mmapped_file_t *mmap_file(const char *path, off_t size, int prot, int flags) {
    int fd = -1;
    off_t f_len = 0;
    char *buf = NULL;
    struct stat statbuf;
    int rv = 0;
    mmapped_file_t *mf;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        log_err("Error opening file %s: %s Skipping...", path, strerror(errno));
        goto cleanup;
    }

    rv = fstat(fd, &statbuf);
    if (rv) {
        log_err("Error fstat()ing file %s. Skipping...", path);
        goto cleanup;
    }
    if ((statbuf.st_mode & S_IFMT) == 0) {
        log_err("%s is not a file. Mode %u. Skipping...", path, statbuf.st_mode);
        goto cleanup;
    }
    if (statbuf.st_mode & S_IFIFO) {
        log_err("%s is a named pipe. Skipping...", path);
        goto cleanup;
    }

    f_len = size > statbuf.st_size ? size : statbuf.st_size;
    prot = prot ? prot : PROT_READ;
    flags = flags ? flags : MAP_SHARED;
    log_debug("mmapping %s %u bytes", path, f_len);
    buf = mmap(0, f_len, prot, flags, fd, 0);
    if (buf == MAP_FAILED) {
        die("Error mmap()ing %s: %s.", path, strerror(errno));
    }

    mf = malloc(sizeof(mmapped_file_t));
    mf->buf = buf;
    mf->len = statbuf.st_size; /* yeah I know this is confusing */
    mf->fd = fd;
    return mf;

    cleanup:;
    if (buf) {
        munmap(buf, f_len);
    }
    if (fd != -1) {
        close(fd);
    }
    return NULL;
}


void munmap_file(mmapped_file_t *mf) {
    munmap(mf->buf, mf->len);
    close(mf->fd);
}
