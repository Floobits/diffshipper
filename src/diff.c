#include <stdlib.h>

#include "dmp.h"
#include "dmp_pool.h"

#include "diff.h"
#include "file.h"
#include "log.h"


void push_changes(const char *path) {
    char orig_base[] = "/tmp/fuck_yo_couch";
    char *old_path;
    dmp_diff *diff;

    old_path = malloc(strlen(orig_base) + strlen(path) + 1);
    log_debug("Getting changes for %s", path);
    diff = diff_files(old_path, path);
    log_debug("diff: ");
    if (diff) {
        dmp_diff_free(diff);
    }
    free(old_path);
}


dmp_diff *diff_files(const char *f1, const char *f2) {
    mmapped_file_t *mf1;
    mmapped_file_t *mf2;
    dmp_diff *diff;
    dmp_options opts;

    mf1 = mmap_file(f1);
    mf2 = mmap_file(f2);

    if (mf1 == NULL || mf2 == NULL) {
        log_err("SHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIT! AVENGE ME!");
        return NULL;
    }

    dmp_diff_new(&diff, &opts, mf1->buf, mf1->len, mf2->buf, mf2->len);

    munmap_file(mf1);
    munmap_file(mf2);
    free(mf1);
    free(mf2);
    return diff;
}

