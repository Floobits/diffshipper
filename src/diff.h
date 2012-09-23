#ifndef DIFF_H
#define DIFF_H

typedef struct {
    void *buf;
    off_t len;
    int fd;
} mmapped_file_t;

mmapped_file_t *mmap_file(const char *path);
void munmap_file(mmapped_file_t *mf);

void get_changes(const char *path);
void diff_files(const char *orig, const char *new);

#endif
