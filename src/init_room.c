#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "init_room.h"
#include "log.h"
#include "options.h"
#include "scandir.h"
#include "util.h"


static int scandir_filter(const char *path, const struct dirent *d, void *baton) {
    log_debug("Examining %s/%s", path, d->d_name);
    if (d->d_name[0] == '.')
        return 0;

    return 1;
}


void recurse_create_bufs(char *full_path) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    const char *base_path = opts.path;
    int results;
    int i;
    int rv;
    char *path;
    const char *path_start = full_path;

    if (strlen(base_path) > strlen(full_path)) {
        die("Base path %s is longer than full path %s!", base_path, full_path);
    } else if (strcmp(base_path, full_path) == 0) {
        log_debug("base path and full path are both %s", base_path);
        path = strdup(base_path);
    } else {
        /* TODO: write a full path -> rel path helper function */
        path = strdup(strlen(base_path)); /* skip last char and trailing slash */
    }
    log_debug("path is %s", path);

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
    for (i = 0; i < results; i++) {
        dir = dir_list[i];
        ds_asprintf(&file_path, "%s%s", full_path, dir->d_name);
        ds_asprintf(&file_path_rel, "%s%s", path, dir->d_name);
        log_debug("file %s", file_path_rel);
    }
}
