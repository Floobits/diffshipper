#ifndef DIFF_H
#define DIFF_H

#include "dmp.h"

void push_changes(const char *path);
dmp_diff *diff_files(const char *f1, const char *f2);

#endif
