#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "buf.h"
#include "fs_event_handlers.h"
#include "ignore.h"
#include "log.h"
#include "mmap.h"
#include "net.h"
#include "options.h"
#include "scandir.h"
#include "util.h"


struct timeval now;


static int changed_filter(const char *path, const struct dirent *d, void *baton) {
    struct stat dir_info;
    char *full_path = NULL;
    char *tmp = NULL;
    int rv;

    log_debug("Examining %s%s", path, d->d_name);

    ds_asprintf(&tmp, "%s/%s", path, d->d_name);
    full_path = realpath(tmp, NULL);
    free(tmp);

    rv = lstat(full_path, &dir_info);
    if (rv) {
        /* TODO: strerror isn't thread-safe on many platforms */
        log_err("lstat failed for %s: %s", full_path, strerror(errno));
        rv = 0;
    } else if (dir_info.st_mtime > now.tv_sec - opts.mtime) {
        log_debug("file %s modified in the last %u seconds. GO TIME!", full_path, opts.mtime);
        rv = 1;
    } else {
        log_debug("skipping d.mtime %li now %li opts %li", dir_info.st_mtime, now.tv_sec, opts.mtime);
        rv = 0;
    }

    free(full_path);

    return rv && scandir_filter(path, d, baton);
}


static int make_patch(void *baton, dmp_operation_t op, const void *data, uint32_t len) {
    diff_info_t *di = (diff_info_t*)baton;
    buf_t *buf = di->buf;
    off_t offset;
    char *action_str = NULL;
    char *cur_patch_str;
    char *escaped_data = NULL;
    size_t patch_len;

    switch (op) {
        case DMP_DIFF_EQUAL:
            return 0;
        break;

        case DMP_DIFF_DELETE:
            offset = data - (void *)buf->buf;
            escaped_data = escape_data(data, len);
            ds_asprintf(&action_str, "@@ -%u,%lld +%u @@", (lli_t)offset, len, (lli_t)offset);
            ds_asprintf(&cur_patch_str, "%s\n-%s\n", action_str, escaped_data);
        break;

        case DMP_DIFF_INSERT:
            offset = data - di->mf->buf;
            escaped_data = escape_data(data, len);
            ds_asprintf(&action_str, "@@ -%u +%u,%lld @@", (lli_t)offset, (lli_t)offset, len);
            ds_asprintf(&cur_patch_str, "%s\n+%s\n", action_str, escaped_data);
        break;

        default:
            die("WTF?!?!");
    }
    log_debug("cur_patch_str: %s", cur_patch_str);
    patch_len = strlen(cur_patch_str) + strlen(di->patch) + 1;
    if (patch_len > di->patch_size) {
        di->patch = realloc(di->patch, patch_len);
        di->patch_size = patch_len;
    }
    strcat(di->patch, cur_patch_str);
    if (escaped_data)
        free(escaped_data);
    free(action_str);
    return 0;
}


void push_changes(const char *base_path, const char *full_path) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    int rv;
    diff_info_t di;
    char *path;

    if (strncmp(base_path, full_path, strlen(base_path)) != 0)
        die("wtf? %s != %s len %li", base_path, full_path, strlen(base_path));

    gettimeofday(&now, NULL);

    path = strdup(full_path + strlen(base_path) + 1); /* Skip trailing slash in full_path */
    log_debug("relative path is %s", path);

    scandir_baton_t baton;
    baton.ig = root_ignores;
    baton.base_path = base_path;
    baton.level = 0;

    results = ds_scandir(full_path, &dir_list, &changed_filter, &baton);
    if (results == -1) {
        log_debug("Error scanning directory %s: %s", full_path, strerror(errno));
        return;
    } else if (results == 0) {
        log_debug("No results found in directory %s", full_path);
        return;
    }

    char *file_path;
    char *file_path_rel;
    struct stat dir_info;
    for (i = 0; i < results; i++) {
        dir = dir_list[i];
        ds_asprintf(&file_path, "%s%s", full_path, dir->d_name);
        ds_asprintf(&file_path_rel, "%s%s", path, dir->d_name);
        if (is_ignored(file_path)) {
            /* we triggered this event */
            unignore_change(file_path);
            goto cleanup;
        }

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
            /* TODO: figure out if we need to recurse */
            goto cleanup;
        }

        buf_t *buf = get_buf(file_path_rel);
        if (buf == NULL) {
            log_err("buf not found for path %s", file_path_rel);
            goto cleanup;
        }

        const char *f2 = file_path;
        dmp_diff *diff = NULL;
        dmp_options opts;
        dmp_options_init(&opts);

        mmapped_file_t *mf;
        struct stat file_stats;
        off_t f2_size;

        rv = lstat(f2, &file_stats);
        if (rv) {
            die("Error lstat()ing file %s.", f2);
        }
        f2_size = file_stats.st_size;

        mf = mmap_file(f2, f2_size, 0, 0);
        if (is_binary(mf->buf, mf->len)) {
            log_debug("%s is binary. skipping", file_path);
            goto diff_cleanup;
        }

        if (dmp_diff_new(&(diff), &opts, buf->buf, buf->len, mf->buf, mf->len) != 0)
            die("dmp_diff_new failed");

        if (!diff) {
            log_err("diff is null. I guess someone wrote the exact same bytes to this file?");
            goto diff_cleanup;
        }

        di.buf = buf;
        di.mf = mf;
        di.patch_size = 100;
        di.patch = malloc(di.patch_size);
        strcpy(di.patch, "");

        dmp_diff_print_raw(stdout, diff);

        dmp_diff_foreach(diff, make_patch, &di);

        if (strlen(di.patch) == 0) {
            log_debug("no change. not sending patch");
            goto diff_cleanup;
        }

        char *md5_after = md5(mf->buf, mf->len);

        send_json(
            "{s:s s:i s:s s:s s:s s:s}",
            "name", "patch",
            "id", buf->id,
            "patch", di.patch,
            "path", buf->path,
            "md5_before", buf->md5,
            "md5_after", md5_after
        );

        free(md5_after);
        free(di.patch);

        buf->buf = realloc(buf->buf, mf->len + 1);
        buf->len = mf->len;
        memcpy(buf->buf, mf->buf, buf->len);

        diff_cleanup:;
        if (diff)
            dmp_diff_free(diff);
        munmap_file(mf);
        free(mf);
        cleanup:;
        free(file_path);
        free(file_path_rel);
        free(dir);
    }
    free(dir_list);
    free(path);
}

