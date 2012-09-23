#ifndef FILE_H
#define FILE_H

#include <sys/types.h>

typedef struct {
    void *buf;
    off_t len;
    int fd;
} mmapped_file_t;

mmapped_file_t *mmap_file(const char *path);
void munmap_file(mmapped_file_t *mf);

#endif
