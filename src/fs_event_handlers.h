#ifndef DIFF_H
#define DIFF_H

#include <dirent.h>

#include "dmp.h"

#include "buf.h"
#include "mmap.h"

#define TMP_BASE "/tmp/diffshipper"

typedef struct {
  buf_t *buf;
  mmapped_file_t *mf1;
  mmapped_file_t *mf2;
  char *patch_str;
} diff_info_t;

void push_changes(const char *base_path, const char *full_path);

#endif
