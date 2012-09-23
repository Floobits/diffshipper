#include <stdlib.h>

#include "diff.h"
#include "file.h"
#include "log.h"


void push_changes(const char *path) {
    log_debug("Getting changes for %s", path);
    
}


void diff_files(const char *f1, const char *f2) {
    mmapped_file_t *mf1;
    mmapped_file_t *mf2;
    mf1 = mmap_file(f1);
    mf2 = mmap_file(f2);
    munmap_file(mf1);
    munmap_file(mf2);
    free(mf1);
    free(mf2);
}

