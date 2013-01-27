#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ignore.h"
#include "log.h"
#include "util.h"

void ignore_change(const char *path) {
    int i;

    pthread_mutex_lock(&ignore_changes_mtx);
    ignored_changes_len++;
    ignored_changes = realloc(ignored_changes, ignored_changes_len * sizeof(char*));
    /* TODO: do a binary search to figure out the position */
    for (i = ignored_changes_len - 1; i > 0; i--) {
        if (strcmp(path, ignored_changes[i-1]) > 0) {
            break;
        }
        ignored_changes[i] = ignored_changes[i-1];
    }
    ignored_changes[i] = strdup(path);
    log_debug("ignoring path %s", path);
    pthread_mutex_unlock(&ignore_changes_mtx);
}


void unignore_change(const char *path) {
    int i;
    /* TODO: do a binary search to figure out the position */
    pthread_mutex_lock(&ignore_changes_mtx);
    /* totally unsafe */
    for (i = 0; i < ignored_changes_len - 1; i++) {
        if (strcmp(path, ignored_changes[i]) >= 0) {
            ignored_changes[i] = ignored_changes[i+1];
        }
    }
    ignored_changes_len--;
    ignored_changes = realloc(ignored_changes, ignored_changes_len * sizeof(char*));
    log_debug("unignoring path %s", path);
    pthread_mutex_unlock(&ignore_changes_mtx);
}


int is_ignored(const char *path) {
    int i;
    int rv = 0;
    pthread_mutex_lock(&ignore_changes_mtx);
    /* TODO: binary search */
    for (i = 0; i < ignored_changes_len; i++) {
        if (strcmp(ignored_changes[i], path) == 0) {
            rv = 1;
            break;
        }
    }
    pthread_mutex_unlock(&ignore_changes_mtx);
    if (rv) {
        log_debug("file %s is ignored", path);
    } else {
        log_debug("file %s is not ignored", path);
    }
    return rv;
}
