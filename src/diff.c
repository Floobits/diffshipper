#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "dmp.h"
#include "dmp_pool.h"

#include "diff.h"
#include "file.h"
#include "log.h"

struct timeval now;


int modified_filter(const struct dirent *dir) {
    struct stat dir_info;
    lstat(dir->d_name, &dir_info);
    /* check it out if it's modified in the last 10 seconds */
    if (dir_info.st_mtimespec.tv_sec > now.tv_sec - 10) {
        return 1;
    }
    return 0;
}


int print_chunk(void *baton, dmp_operation_t op, const void *data, uint32_t len) {
    if (baton == NULL) {
        log_err("baton is null");
    }
    else {
        log_err("baton is %s", (char*)baton);
    }
    switch (op) {
        case DMP_DIFF_EQUAL:
            log_debug("equal");
        break;

        case DMP_DIFF_DELETE:
            log_debug("delete");
        break;

        case DMP_DIFF_INSERT:
            log_debug("insert");
        break;

        default:
            log_err("WTF?!?!");
            exit(1);
    }
    log_debug("len: %u", (size_t)len);
    log_err("omg baton is %s", (char*)baton);
    if (op != DMP_DIFF_EQUAL) {
        char *temp = malloc(len+1);
        strlcpy(temp, data, len+1);
        log_err(temp);
/*        fwrite(data, (size_t)len, 1, stdout);*/
    }

    return 0;
}

void push_changes(const char *path) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    dmp_diff *diff;
    gettimeofday(&now, NULL);

    results = scandir(path, &dir_list, &modified_filter, &alphasort);
    if (results == 0) {
        log_debug("No results found in directory %s", path);
    }

    char orig_base[] = "/tmp/fuck_yo_couch";
    char *orig_path;
    int orig_path_len;
    char *file_path;
    int file_path_len;
    struct stat dir_info;
    for (i = 0; i < results; i++) {
        dir = dir_list[i];

        /* If a link points to a directory then we need to treat it as a directory. */
        if (dir->d_type == DT_LNK) {
            if (stat(file_path, &dir_info) != -1) {
                if (S_ISDIR(dir_info.st_mode)) {
                    dir->d_type = DT_DIR;
                }
            }
            else {
                log_err("stat() failed on %s", file_path);
                /* If stat fails we may as well carry on and hope for the best. */
            }
        }

        if (dir->d_type == DT_DIR) {
            /* todo: figure out if we need to recurse */
            continue;
        }

        orig_path_len = strlen(orig_base) + strlen(path) + strlen(dir->d_name) + 1;
        orig_path = malloc(orig_path_len);
        strlcpy(orig_path, orig_base, orig_path_len);
        strlcat(orig_path, path, orig_path_len);
        strlcat(orig_path, dir->d_name, orig_path_len);

        file_path_len = strlen(path) + strlen(dir->d_name) + 1;
        file_path = malloc(file_path_len);
        strlcat(file_path, path, file_path_len);
        strlcat(file_path, dir->d_name, file_path_len);

        log_debug("Diffing. original %s, new %s", orig_path, file_path);

        diff = diff_files(orig_path, file_path);
        log_debug("diff: ");
        char *baton = calloc(100, 1);
        strlcpy(baton, "baton baton baton baton baton baton baton", 100);
        if (diff) {
            dmp_diff_foreach(diff, print_chunk, baton);
            dmp_diff_free(diff);
        }
        else {
            log_err("damn. diff is null");
        }
        free(orig_path);
        free(file_path);
    }
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

