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
#include "file.h"
#include "log.h"
#include "net.h"
#include "util.h"


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


int send_diff_chunk(void *baton, dmp_operation_t op, const void *data, uint32_t len) {
    diff_info_t *di = (diff_info_t*)baton;
    off_t offset;
    char *msg;
    int msg_len;
    char *data_str;
    char *data_safe;

    /* Just so you know, I know this is bad. */
    switch (op) {
        case DMP_DIFF_EQUAL:
            /* Don't care */
            log_debug("equal");
            return 0;
        break;

        /* TODO: escape stuff. especially newlines and double-quotes */
        case DMP_DIFF_DELETE:
            offset = data - di->mf1->buf;
            log_debug("delete. offset: %i bytes", offset);
            data_str = malloc(len + 1);
            strncpy(data_str, data, len + 1);
            data_safe = escape_data(data_str);
            msg_len = asprintf(&msg, "{ \"path\": \"%s\", \"action\": \"-%u@%lld\", \"data\": \"%s\" }\n", di->path, len, offset, data_safe);
        break;

        case DMP_DIFF_INSERT:
            offset = data - di->mf2->buf;
            log_debug("insert. offset: %i bytes", offset);
            data_str = malloc(len + 1);
            strncpy(data_str, data, len + 1);
            data_safe = escape_data(data_str);
            msg_len = asprintf(&msg, "{ \"path\": \"%s\", \"action\": \"+%u@%lld\", \"data\": \"%s\" }\n", di->path, len, offset, data_safe);
        break;

        default:
            die("WTF?!?!");
    }
    log_debug("msg: %s", msg);
    send_bytes(msg, msg_len);
    fwrite(data, (size_t)len, 1, stdout);
    free(data_str);
    free(data_safe);

    return 0;
}


void push_changes(const char *base_path, const char *full_path) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    int results;
    int i;
    int rv;
    diff_info_t di;
    ftc_diff_t ftc_diff;
    char *path;
    const char *path_start = full_path;
    gettimeofday(&now, NULL);

    for (i = 0; base_path[i] == full_path[i] && i < (int)strlen(base_path); i++) {
        path_start = &full_path[i];
    }
    path = strdup(path_start + 2);
    log_debug("path is %s", path);

    results = scandir(path, &dir_list, &modified_filter, &alphasort);
    if (results == -1) {
        log_debug("Error scanning directory %s", path);
        return;
    } else if (results == 0) {
        log_debug("No results found in directory %s", path);
        return;
    }

    char *orig_path;
    char *file_path;
    char *file_path_rel;
    struct stat dir_info;
    for (i = 0; i < results; i++) {
        dir = dir_list[i];
        asprintf(&file_path, "%s%s", full_path, dir->d_name);
        asprintf(&file_path_rel, "%s%s", path, dir->d_name);
        if (ignored(file_path)) {
            /* we triggered this event */
            unignore_path(file_path);
            continue;
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
            continue;
        }

        asprintf(&orig_path, "%s%s", TMP_BASE, file_path);

        ftc_diff.diff = NULL;
        diff_files(&ftc_diff, orig_path, file_path);
        if (!ftc_diff.diff) {
            log_err("diff is null. I guess someone wrote the exact same bytes to this file?");
            goto cleanup;
        }

        mmapped_file_t *mf1 = ftc_diff.mf1;
        mmapped_file_t *mf2 = ftc_diff.mf2;
        di.path = file_path_rel;
        di.mf1 = mf1;
        di.mf2 = mf2;
        dmp_diff_print_raw(stderr, ftc_diff.diff);
        dmp_diff_foreach(ftc_diff.diff, send_diff_chunk, &di);

        if (mf1->len != mf2->len) {
            if (ftruncate(mf1->fd, mf2->len) != 0) {
                die("resizing %s failed", ftc_diff.f1);
            }
            log_debug("resized %s to %u bytes", ftc_diff.f1, mf2->len);
        }

        munmap(mf1->buf, mf1->len);
        mf1->buf = mmap(0, mf2->len, PROT_WRITE | PROT_READ, MAP_SHARED, mf1->fd, 0);
        mf1->len = mf2->len;
        memcpy(mf1->buf, mf2->buf, mf1->len);
        rv = msync(mf1->buf, mf1->len, MS_SYNC);

        log_debug("rv %i wrote %i bytes to %s", rv, mf1->len, ftc_diff.f1);

        ftc_diff_cleanup(&ftc_diff);
        cleanup:;
        free(orig_path);
        free(file_path);
    }
}


void diff_files(ftc_diff_t *f, const char *f1, const char *f2) {
    struct stat file_stats;
    int rv;
    off_t f1_size;
    off_t f2_size;
    dmp_options opts;

    memset(&opts, 0, sizeof(opts));
    opts.timeout = 0.5; /* give up diffing after 0.5 seconds of processing */

    f->f1 = f1; /* not sure if this is a good idea*/
    f->f2 = f2;

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

    f->mf2 = mmap_file(f2, f2_size, 0, 0);
    f->mf1 = mmap_file(f1, f1_size, PROT_WRITE | PROT_READ, 0);

    if (f->mf1 && f->mf2)
        dmp_diff_new(&(f->diff), &opts, f->mf1->buf, f->mf1->len, f->mf2->buf, f->mf2->len);
}


void ftc_diff_cleanup(ftc_diff_t *f) {
    dmp_diff_free(f->diff);
    munmap_file(f->mf1);
    munmap_file(f->mf2);
    free(f->mf1);
    free(f->mf2);
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
        memmove(op_point + len, op_point, (file_size - len) - offset);
        memcpy(op_point, buf, len);
    } else if (op == DMP_DIFF_DELETE) {
        file_size = mf->len - len;
        memmove(op_point, op_point + len, file_size - offset);
    }
    if (ftruncate(mf->fd, file_size) != 0) {
        die("resizing %s failed", path);
    }
    log_debug("resized %s to %u bytes", path, file_size);
    rv = msync(mf->buf, file_size, MS_SYNC);
    log_debug("rv %i wrote %i bytes to %s", rv, file_size, path);
    munmap_file(mf);
    free(mf);
}
