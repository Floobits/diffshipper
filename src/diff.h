#ifndef DIFF_H
#define DIFF_H

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

void push_changes(const char *path);

void diff_files(ftc_diff_t *f, const char *f1, const char *f2);
void ftc_diff_cleanup(ftc_diff_t *ftc_diff);


#endif
