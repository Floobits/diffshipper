#include <stdlib.h>

#include "dmp.h"
#include "dmp_pool.h"

#include "diff.h"
#include "file.h"
#include "log.h"


void push_changes(const char *path) {
    char orig_base[] = "/tmp/fuck_yo_couch";
    char *orig_path;
    dmp_diff *diff;

    int orig_path_len = strlen(orig_base) + strlen(path) + 1;
    orig_path = malloc(orig_path_len);
    strlcpy(orig_path, orig_base, orig_path_len);
    strlcat(orig_path, path, orig_path_len);
    log_debug("Getting changes for %s", path);
    diff = diff_files(orig_path, path);
    log_debug("diff: ");
    if (diff) {
        dmp_diff_free(diff);
    }
    free(orig_path);
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

