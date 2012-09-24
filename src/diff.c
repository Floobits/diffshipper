#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "dmp.h"
#include "dmp_pool.h"

#include "diff.h"
#include "file.h"
#include "log.h"

struct timespec now;

int modified_filter(const struct dirent *dir) {
    struct stat dir_info;
    lstat(dir->d_name, &dir_info);
    /* check it out if it's modified in the last 10 seconds */
    if (dir_info.st_mtimespec.tv_sec > now.tv_sec - 10) {
        return 1;
    }
    return 0;
}

void push_changes(const char *path) {
    char orig_base[] = "/tmp/fuck_yo_couch";
    char *orig_path;
    int orig_path_len;
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    dmp_diff *diff;
    gettimeofday(&now, NULL);

    orig_path_len = strlen(orig_base) + strlen(path) + 1;
    orig_path = malloc(orig_path_len);
    strlcpy(orig_path, orig_base, orig_path_len);
    strlcat(orig_path, path, orig_path_len);

    results = scandir(path, &dir_list, &modified_filter, &alphasort);
    if (results == 0)
    {
        log_debug("No results found in directory %s", path);
    }

    for (i = 0; i < results; i++) {
        log_debug("Getting changes for %s", path);
        diff = diff_files(orig_path, path);
        log_debug("diff: ");
        if (diff) {
            dmp_diff_free(diff);
        }
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

