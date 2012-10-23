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

#include "diff.h"
#include "mmap.h"
#include "log.h"
#include "net.h"
#include "scandir.h"
#include "options.h"
#include "util.h"


struct timeval now;


int scandir_filter(const char *path, const struct dirent *d, void *baton) {
    log_debug("Examining %s", d->d_name);
    if (d->d_name[0] == '.')
        return 0;

    struct stat dir_info;
    char *full_path = NULL;
    ds_asprintf(&full_path, "%s/%s", path, d->d_name);
    int rv = lstat(full_path, &dir_info);
    if (rv != 0) {
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
    return rv;
}


int send_diff_chunk(void *baton, dmp_operation_t op, const void *data, uint32_t len) {
    diff_info_t *di = (diff_info_t*)baton;
    off_t offset;
    char *msg;
    int msg_len = 0;
    char *data_str = NULL;
    char *action_str = NULL;
    json_t *obj = NULL;
    int json_dumps_flags = JSON_ENSURE_ASCII;

    switch (op) {
        case DMP_DIFF_EQUAL:
            /* Don't care */
            log_debug("equal");
            return 0;
        break;

        case DMP_DIFF_DELETE:
            offset = data - di->mf1->buf;
            data_str = malloc(len + 1);
            strncpy(data_str, data, len + 1);
            ds_asprintf(&action_str, "-%u@%lld", len, (lli_t)offset);
        break;

        case DMP_DIFF_INSERT:
            offset = data - di->mf2->buf;
            data_str = malloc(len + 1);
            strncpy(data_str, data, len + 1);
            ds_asprintf(&action_str, "+%u@%lld", len, (lli_t)offset);
        break;

        default:
            die("WTF?!?!");
    }
    obj = json_pack("{s:s s:s s:{s:s s:s s:s}}", "path", di->path, "action", "patch", "patch", "action", action_str, "data", data_str, "md5", "test");
    msg = json_dumps(obj, json_dumps_flags);
    msg_len = strlen(msg) + 1;

    msg = realloc(msg, msg_len+1);
    strcat(msg, "\n");
    log_debug("msg: %s", msg);
    send_bytes(msg, msg_len);
    fwrite(data, (size_t)len, 1, stdout);
    free(msg);
    if (data_str)
        free(data_str);
    if (action_str)
        free(action_str);
    json_decref(obj);
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
    const char *path_start = full_path;

    gettimeofday(&now, NULL);

    for (i = 0; base_path[i] == full_path[i] && i < (int)strlen(base_path); i++) {
        path_start = full_path + i;
    }
    path = strdup(path_start + 2); /* skip last char and trailing slash */
    log_debug("path is %s", path);

    results = ds_scandir(full_path, &dir_list, &scandir_filter, &full_path);
    if (results == -1) {
        log_debug("Error scanning directory %s: %s", full_path, strerror(errno));
        return;
    } else if (results == 0) {
        log_debug("No results found in directory %s", full_path);
        return;
    }

    char *orig_path;
    char *file_path;
    char *file_path_rel;
    struct stat dir_info;
    for (i = 0; i < results; i++) {
        dir = dir_list[i];
        ds_asprintf(&file_path, "%s%s", full_path, dir->d_name);
        ds_asprintf(&file_path_rel, "%s%s", path, dir->d_name);
        if (ignored(file_path_rel)) {
            /* we triggered this event */
            unignore_path(file_path_rel);
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

        ds_asprintf(&orig_path, "%s%s", TMP_BASE, file_path);

        const char *f1 = orig_path;
        const char *f2 = file_path;
        dmp_diff *diff = NULL;
        dmp_options opts;
        memset(&opts, 0, sizeof(opts));
        opts.timeout = 0.5; /* give up diffing after 0.5 seconds of processing */

        mmapped_file_t *mf1;
        mmapped_file_t *mf2;
        struct stat file_stats;
        off_t f1_size;
        off_t f2_size;

        rv = lstat(f1, &file_stats);
        if (rv != 0) {
            die("Error lstat()ing file %s.", f1);
        }
        f1_size = file_stats.st_size;
        rv = lstat(f2, &file_stats);
        if (rv != 0) {
            die("Error lstat()ing file %s.", f2);
        }
        f2_size = file_stats.st_size;
        f1_size = f2_size > f1_size ? f2_size : f1_size;

        mf2 = mmap_file(f2, f2_size, 0, 0);
        mf1 = mmap_file(f1, f1_size, PROT_WRITE | PROT_READ, 0);

        if (is_binary(mf1->buf, mf1->len)) {
            log_debug("%s is binary. skipping", file_path);
            goto diff_cleanup;
        }

        if (mf1 && mf2)
            dmp_diff_new(&(diff), &opts, mf1->buf, mf1->len, mf2->buf, mf2->len);

        if (!diff) {
            log_err("diff is null. I guess someone wrote the exact same bytes to this file?");
            goto diff_cleanup;
        }

        di.path = file_path_rel;
        di.mf1 = mf1;
        di.mf2 = mf2;
        dmp_diff_print_raw(stderr, diff);
        dmp_diff_foreach(diff, send_diff_chunk, &di);

        if (mf1->len != mf2->len) {
            if (ftruncate(mf1->fd, mf2->len) != 0) {
                die("resizing %s failed", f1);
            }
            log_debug("resized %s to %u bytes", f1, mf2->len);
        }

        munmap(mf1->buf, mf1->len);
        mf1->buf = mmap(0, mf2->len, PROT_WRITE | PROT_READ, MAP_SHARED, mf1->fd, 0);
        mf1->len = mf2->len;
        memcpy(mf1->buf, mf2->buf, mf1->len);
        rv = msync(mf1->buf, mf1->len, MS_SYNC);

        log_debug("rv %i wrote %i bytes to %s", rv, mf1->len, f1);

        diff_cleanup:;
        if (diff)
            dmp_diff_free(diff);
        munmap_file(mf1);
        munmap_file(mf2);
        free(mf1);
        free(mf2);
        free(orig_path);
        cleanup:;
        free(file_path);
        free(file_path_rel);
    }
    free(path);
}


void apply_diff(char *path, dmp_operation_t op, char *buf, size_t len, off_t offset) {
    struct stat file_stats;
    int rv;
    mmapped_file_t *mf;

    log_debug("patching %s: %lu bytes at %lu", path, len, offset);

    rv = lstat(path, &file_stats);
    if (rv != 0) {
        die("Error lstat()ing file %s.", path);
    }

    off_t file_size = file_stats.st_size;
    if (op == DMP_DIFF_INSERT) {
        file_size += len;
    }
    mf = mmap_file(path, file_size, PROT_WRITE | PROT_READ, 0);
    if (!mf) {
        die("mmap of %s failed!", path);
        return; /* never get here, but make the static analyzer happy */
    }

    if (mf->len < offset)
        die("%s is too small to apply patch to!", path);

    void *op_point = mf->buf + offset;
    if (op == DMP_DIFF_INSERT) {
        if (ftruncate(mf->fd, file_size) != 0) {
            die("resizing %s failed", path);
        }
        log_debug("memmove(%u, %u, %u)", (size_t)(op_point + len), (size_t)op_point, (file_size - len) - offset);
        memmove(op_point + len, op_point, (file_size - len) - offset);
        memcpy(op_point, buf, len);
    } else if (op == DMP_DIFF_DELETE) {
        file_size = mf->len - len;
        memmove(op_point, op_point + len, file_size - offset);
        if (ftruncate(mf->fd, file_size) != 0) {
            die("resizing %s failed", path);
        }
    }
    log_debug("resized %s to %u bytes", path, file_size);
    rv = msync(mf->buf, file_size, MS_SYNC);
    log_debug("rv %i wrote %i bytes to %s", rv, file_size, path);
    munmap_file(mf);
    free(mf);
}
