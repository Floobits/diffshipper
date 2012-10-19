#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "util.h"


int run_cmd(const char *fmt, ...) {
    char *cmd;
    int rv;
    va_list args;

    va_start(args, fmt);
    if (vasprintf(&cmd, fmt, args) == -1)
        die("vasprintf returned -1");
    va_end(args);

    log_debug("Running %s", cmd);
    rv = system(cmd);

    return rv;
}


void ignore_path(const char *path) {
    int i;

    pthread_mutex_lock(&ignore_mtx);
    ignored_paths_len++;
    ignored_paths = realloc(ignored_paths, ignored_paths_len * sizeof(char*));
    /* TODO: do a binary search to figure out the position */
    for (i = ignored_paths_len - 1; i > 0; i--) {
        if (strcmp(path, ignored_paths[i-1]) > 0) {
            break;
        }
        ignored_paths[i] = ignored_paths[i-1];
    }
    ignored_paths[i] = strdup(path);
    log_debug("ignoring path %s", path);
    pthread_mutex_unlock(&ignore_mtx);
}


void unignore_path(const char *path) {
    int i;
    /* TODO: do a binary search to figure out the position */
    pthread_mutex_lock(&ignore_mtx);
    /* totally unsafe */
    for (i = 0; i < ignored_paths_len - 1; i++) {
        if (strcmp(path, ignored_paths[i]) >= 0) {
            ignored_paths[i] = ignored_paths[i+1];
        }
    }
    ignored_paths_len--;
    ignored_paths = realloc(ignored_paths, ignored_paths_len * sizeof(char*));
    log_debug("unignoring path %s", path);
    pthread_mutex_unlock(&ignore_mtx);
}


int ignored(const char *path) {
    int i;
    int rv = 0;
    pthread_mutex_lock(&ignore_mtx);
    /* TODO: binary search */
    for (i = 0; i < ignored_paths_len; i++) {
        if (strcmp(ignored_paths[i], path) == 0) {
            rv = 1;
            break;
        }
    }
    pthread_mutex_unlock(&ignore_mtx);
    if (rv) {
        log_debug("file %s is ignored", path);
    } else {
        log_debug("file %s is not ignored", path);
    }
    return rv;
}


char *escape_data(char *data) {
    char *escaped;
    int data_len = strlen(data);
    int i;
    int offset = 0;

    escaped = malloc(data_len);

    for (i = 0; data[i] != '\0'; i++) {
        escaped[i + offset] = data[i];
        if (data[i] == '\n') {
            offset++;
            escaped = realloc(escaped, data_len + offset);
            strcpy(&escaped[i + offset - 1], "\\n");
        }
    }
    return escaped;
}


char *unescape_data(char *data) {
    /* LOL pranked you */
    return data;
}


/* To get around gcc's -Wunused-result */
void ds_asprintf(char **ret, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vasprintf(ret, fmt, args) == -1)
        die("vasprintf returned -1");
    va_end(args);
}


int is_binary(const void* buf, const int buf_len) {
    int suspicious_bytes = 0;
    int total_bytes = buf_len > 1024 ? 1024 : buf_len;
    const unsigned char *buf_c = buf;
    int i;

    if (buf_len == 0) {
        return 0;
    }

    for (i = 0; i < total_bytes; i++) {
        if (buf_c[i] == '\0') {
            /* NULL char. It's binary */
            return 1;
        } else if ((buf_c[i] < 7 || buf_c[i] > 14) && (buf_c[i] < 32 || buf_c[i] > 127)) {
            suspicious_bytes++;
            /* Disk IO is so slow that it's worthwhile to do this calculation after every suspicious byte. */
            /* This is true even on a 1.6Ghz Atom with an Intel 320 SSD. */
            if ((suspicious_bytes * 100) / total_bytes > 10) {
                return 1;
            }
        }
    }

    return 0;
}