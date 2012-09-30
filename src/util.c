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
    vasprintf(&cmd, fmt, args);
    va_end(args);

    log_debug("Running %s", cmd);
    rv = system(cmd);

    return rv;
}


void ignore_path(const char *path) {
    int i;

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
}


void unignore_path(const char *path) {
    int i;
    /* totally unsafe */
    /* TODO: do a binary search to figure out the position */
    for (i = 0; i < ignored_paths_len - 1; i++) {
        if (strcmp(path, ignored_paths[i]) >= 0) {
            ignored_paths[i] = ignored_paths[i+1];
        }
    }
    ignored_paths_len--;
    ignored_paths = realloc(ignored_paths, ignored_paths_len * sizeof(char*));
    log_debug("unignoring path %s", path);
}


int ignored(const char *path) {
    int i;
    for (i = 0; i < ignored_paths_len; i++) {
        if (strcmp(ignored_paths[i], path) == 0)
            return 1;
    }
    return 0;
}
