#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ignore.h"
#include "init_room.h"
#include "log.h"
#include "mmap.h"
#include "net.h"
#include "options.h"
#include "scandir.h"
#include "util.h"


static void recurse_create_bufs(const char *full_path, ignores_t *ig, int depth) {
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
        return;
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

    scandir_baton_t baton;
    baton.ig = root_ignores;
    baton.base_path = full_path;
    baton.level = 0;

    char *dir_full_path = NULL;
    const char *ignore_file = NULL;

    /* find dsignore/gitignore/hgignore/etc files to load ignore patterns from */
    for (i = 0; ignore_pattern_files[i] != NULL; i++) {
        ignore_file = ignore_pattern_files[i];
        ds_asprintf(&dir_full_path, "%s/%s", full_path, ignore_file);
        if (strcmp(SVN_DIR, ignore_file) == 0) {
            load_svn_ignore_patterns(ig, dir_full_path);
        } else {
            load_ignore_patterns(ig, dir_full_path);
        }
        free(dir_full_path);
        dir_full_path = NULL;
    }

    results = ds_scandir(full_path, &dir_list, &scandir_filter, &baton);
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
    char *buf_str;

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
            ignores_t *ig_child = init_ignore(ig);
            recurse_create_bufs(file_path, ig_child, depth + 1);
            goto cleanup;
        }

        rv = lstat(file_path, &file_info);
        if (rv)
            die("Error lstat()ing file %s.", file_path);

        if (file_info.st_size == 0)
            goto cleanup;

        mf = mmap_file(file_path, file_info.st_size, 0, 0);
        if (!mf) {
            log_err("Couldn't open file %s. Skipping.", file_path);
            goto cleanup;
        }
        if (is_binary(mf->buf, mf->len)) {
            log_debug("%s is binary. skipping", file_path);
        } else {
            buf_str = malloc(mf->len + 1);
            strncpy(buf_str, mf->buf, mf->len);
            buf_str[mf->len] = '\0';
            send_json(
                "{s:s s:s s:s}",
                "name", "create_buf",
                "path", file_path_rel,
                "buf", buf_str
            );
            free(buf_str);
        }

        munmap_file(mf);
        free(mf);

        cleanup:;
        free(file_path_rel);
        free(file_path);
    }

    free(rel_path);
}


void create_room(const char *path) {
    recurse_create_bufs(path, root_ignores, 0);
}
