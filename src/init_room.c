#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "init_room.h"
#include "log.h"
#include "mmap.h"
#include "net.h"
#include "options.h"
#include "scandir.h"
#include "util.h"


static int scandir_filter(const char *path, const struct dirent *d, void *baton) {
    log_debug("Examining %s/%s", path, d->d_name);
    if (d->d_name[0] == '.')
        return 0;

    return 1;
}


void recurse_create_bufs(char *full_path, int depth) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    int rv;
    char *rel_path;

    if (depth > MAX_SEARCH_DEPTH) {
        log_err("Exceeded max depth of %i. Not adding files in %s", MAX_SEARCH_DEPTH, full_path);
        return;
    }

    if (strlen(opts.path) > strlen(full_path)) {
        die("Base path %s is longer than full path %s!", opts.path, full_path);
    } else if (strcmp(opts.path, full_path) == 0) {
        log_debug("base path and full path are both %s", opts.path);
        rel_path = strdup("");
    } else if (strncmp(opts.path, full_path, strlen(opts.path)) != 0) {
        die("wtf? %s != %s len %li", opts.path, full_path, strlen(opts.path));
        return;
    } else {
        ds_asprintf(&rel_path, "%s/", full_path + strlen(opts.path) + 1); /* skip slash */
    }
    log_debug("path is %s", rel_path);

    results = ds_scandir(full_path, &dir_list, &scandir_filter, &full_path);
    if (results == -1) {
        log_debug("Error scanning directory %s: %s", full_path, strerror(errno));
        return;
    } else if (results == 0) {
        log_debug("No results found in directory %s", full_path);
        return;
    }

    char *file_path;
    char *file_path_rel;
    mmapped_file_t *mf;
    struct stat dir_info;
    struct stat file_info;
    char *escaped_buf;

    for (i = 0; i < results; i++) {
        dir = dir_list[i];
        ds_asprintf(&file_path, "%s/%s", full_path, dir->d_name);
        ds_asprintf(&file_path_rel, "%s%s", rel_path, dir->d_name);
        log_debug("file %s", file_path_rel);

        /* If a link points to a directory then we need to treat it as a directory. */
        if (dir->d_type == DT_LNK) {
            if (stat(file_path, &dir_info) == -1) {
                log_err("stat() failed on %s", file_path);
                /* If stat fails we may as well carry on and hope for the best. */
            } else if (S_ISDIR(dir_info.st_mode)) {
                dir->d_type = DT_DIR;
            }
        }
        if (dir->d_type == DT_DIR) {
            recurse_create_bufs(file_path, depth + 1);
            goto cleanup;
        }

        rv = lstat(file_path, &file_info);
        if (rv)
            die("Error lstat()ing file %s.", file_path);

        mf = mmap_file(file_path, file_info.st_size, 0, 0);
        if (!mf) {
            log_err("Couldn't open file %s. Skipping.", file_path);
            goto cleanup;
        }
        if (is_binary(mf->buf, mf->len)) {
            log_debug("%s is binary. skipping", file_path);
        } else {
            escaped_buf = escape_data(mf->buf, mf->len);
            send_json(
                "{s:s s:s s:s}",
                "name", "create_buf",
                "path", file_path_rel,
                "buf", escaped_buf
            );
            free(escaped_buf);
        }

        munmap_file(mf);
        free(mf);

        cleanup:;
        free(file_path_rel);
        free(file_path);
    }

    free(rel_path);
}
