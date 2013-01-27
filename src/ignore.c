#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ignore.h"
#include "log.h"
#include "util.h"

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
