#ifndef DIFF_H
#define DIFF_H

#include <dirent.h>

#include "dmp.h"

#include "file.h"

#define TMP_BASE "/tmp/diff-patch-cloud"

typedef struct {
    const char *f1;
    const char *f2;
    dmp_diff *diff;
    dmp_options opts;
    mmapped_file_t *mf1;
    mmapped_file_t *mf2;
} ftc_diff_t;

typedef struct {
  char *path;
  mmapped_file_t *mf1;
  mmapped_file_t *mf2;
} diff_info_t;

int modified_filter(const struct dirent *dir);
int send_diff_chunk(void *baton, dmp_operation_t op, const void *data, uint32_t len);

void push_changes(const char *path);

void diff_files(ftc_diff_t *f, const char *f1, const char *f2);
void ftc_diff_cleanup(ftc_diff_t *ftc_diff);

void apply_diff(void *buf, size_t len);

#endif
