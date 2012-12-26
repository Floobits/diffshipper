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

void push_changes(const char *base_path, const char *full_path);

#endif
