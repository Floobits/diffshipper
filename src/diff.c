#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

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
    if (op != DMP_DIFF_EQUAL) {
        fwrite(data, (size_t)len, 1, stdout);
    }

    return 0;
}

void push_changes(const char *path) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    int rv;
    ftc_diff_t ftc_diff;
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
            if (stat(file_path, &dir_info) == -1) {
                log_err("stat() failed on %s", file_path);
                /* If stat fails we may as well carry on and hope for the best. */
            }
            else if (S_ISDIR(dir_info.st_mode)) {
                dir->d_type = DT_DIR;
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

        diff_files(&ftc_diff, orig_path, file_path);
        if (!ftc_diff.diff) {
            log_err("damn. diff is null");
            goto cleanup;
        }

        dmp_diff_print_raw(stderr, ftc_diff.diff);
        memcpy(ftc_diff.mf1->buf, ftc_diff.mf2->buf, ftc_diff.mf2->len);
        rv = msync_file(ftc_diff.mf1, ftc_diff.mf2->len);
        log_debug("rv %i wrote %i bytes to %s", rv, ftc_diff.mf2->len, ftc_diff.f1);

        cleanup:;
        free(orig_path);
        free(file_path);
    }
}


void diff_files(ftc_diff_t *f, const char *f1, const char *f2) {
    f->f1 = f1; /* not sure if this is a good idea*/
    f->f2 = f2;
    f->mf2 = mmap_file(f2, 10000, 0, 0);
    f->mf1 = mmap_file(f1, 10000, PROT_WRITE | PROT_READ, 0);

    dmp_diff_new(&(f->diff), NULL, f->mf1->buf, f->mf1->len, f->mf2->buf, f->mf2->len);
}


void ftc_diff_cleanup(ftc_diff_t *f) {
/*    dmp_diff_free(f->diff);*/
    munmap_file(f->mf1);
    munmap_file(f->mf2);
    free(f->mf1);
    free(f->mf2);
}
