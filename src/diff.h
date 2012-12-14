#ifndef DIFF_H
#define DIFF_H

#include <dirent.h>

#include "dmp.h"

#include "mmap.h"

#define TMP_BASE "/tmp/diffshipper"

typedef struct {
  char *path;
  mmapped_file_t *mf1;
  mmapped_file_t *mf2;
} diff_info_t;

int scandir_filter(const char *path, const struct dirent *d, void *baton);
int send_diff_chunk(void *baton, dmp_operation_t op, const void *data, uint32_t len);

void push_changes(const char *base_path, const char *full_path);

void apply_diff(char *path, dmp_operation_t op, char *buf, size_t len, off_t offset);

void apply_patch(int buf_id, char *patch);

#endif
