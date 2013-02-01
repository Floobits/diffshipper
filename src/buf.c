#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include "dmp.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "buf.h"
#include "ignore.h"
#include "log.h"
#include "mmap.h"
#include "options.h"
#include "util.h"


void init_bufs() {
    bufs_len = 0;
    bufs_size = BUFS_START_SIZE;
    bufs = calloc(bufs_size, sizeof(buf_t*));
}


static void free_buf(buf_t *buf) {
    free(buf->buf);
    free(buf->md5);
    free(buf->path);
    free(buf);
}


void cleanup_bufs() {
    size_t i;
    for (i = 0; i < bufs_len; i++) {
        free_buf(bufs[i]);
    }
    free(bufs);
}


char *get_full_path(char *rel_path) {
    char *full_path;
    ds_asprintf(&full_path, "%s/%s", opts.path, rel_path);
    return full_path;
}


static int buf_binary_search(const int buf_id, unsigned int start, unsigned int end) {
    unsigned int mid;

    if (start == end) {
        return -1;
    }

    mid = (start + end) / 2; /* can screw up on arrays with > 2 billion elements */

    if (buf_id < bufs[mid]->id) {
        return buf_binary_search(buf_id, start, mid);
    } else if (buf_id > bufs[mid]->id) {
        return buf_binary_search(buf_id, mid + 1, end);
    }

    return mid;
}


buf_t *get_buf_by_id(const int buf_id) {
    int index;
    buf_t *buf = NULL;

    index = buf_binary_search(buf_id, 0, bufs_len);
    if (index >= 0) {
        buf = bufs[index];
    }

    return buf;
}


buf_t *get_buf(const char *path) {
    size_t i;
    buf_t *buf = NULL;

    for (i = 0; i < bufs_len; i++) {
        log_debug("path %s buf path %s", path, bufs[i]->path);
        if (strcmp(bufs[i]->path, path) == 0) {
            buf = bufs[i];
            break;
        }
    }

    return buf;
}


void add_buf_to_bufs(buf_t *buf) {
    size_t i;

    bufs_len++;

    if (bufs_len > bufs_size) {
        bufs_size *= 1.5;
        bufs = realloc(bufs, bufs_size * sizeof(buf_t*));
    }

    for (i = bufs_len - 1; i > 0; i--) {
        if (buf->id > bufs[i-1]->id) {
            break;
        }
        bufs[i] = bufs[i-1];
    }
    bufs[i] = buf;
    log_debug("added buf id %i to position %i", buf->id, i);
}


void delete_buf(buf_t *buf) {
    int index;
    size_t i;

    index = buf_binary_search(buf->id, 0, bufs_len);
    if (index < 0) {
        die("couldn't find buf id %i", buf->id);
    }

    free_buf(bufs[index]);

    if (bufs_len > 0) {
        /* Shift everything after */
        bufs_len--;
        for (i = index; i < bufs_len; i++) {
            bufs[i] = bufs[i+1];
        }
    }
}


void save_buf(buf_t *buf) {
    char *full_path;
    int fd;
    int rv;
    ssize_t bytes_written;
    log_debug("saving buf %i path %s", buf->id, buf->path);
    full_path = get_full_path(buf->path);

    char *buf_path = strdup(full_path);
    int i;
    for (i = strlen(buf_path); i > 0; i--) {
        if (buf_path[i] == '/') {
            buf_path[i] = '\0';
            break;
        }
    }
    if (strlen(buf_path) > 0) {
        rv = run_cmd("mkdir -p %s", buf_path);
        if (rv)
            die("error creating directory %s", buf_path);
    }

    ignore_change(full_path);
    fd = open(full_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
        die("Error opening file %s: %s", full_path, strerror(errno));

    buf->len = strlen(buf->buf); /* TODO: not binary-safe */
    if (ftruncate(fd, buf->len) != 0)
        die("resizing %s failed", full_path);

    bytes_written = write(fd, buf->buf, buf->len);
    log_debug("wrote %i bytes to %s", bytes_written, buf->path);
    close(fd);
    free(buf_path);
    free(full_path);
}


int apply_patch(buf_t *buf, char *patch_text) {
    int rv;
    lua_getglobal(l, "apply_patch");
    lua_pushstring(l, buf->buf);
    lua_pushstring(l, patch_text);
    rv = lua_pcall(l, 2, 2, 0);
    if (rv) {
        die("error calling lua: %s", lua_tostring(l, -1));
    }

    int clean_patch = lua_toboolean(l, -1);
    const char *new_text = lua_tostring(l, -2);

    log_debug("clean patch: %i. new text: %s", clean_patch, new_text);
    lua_settop(l, 0);

    free(buf->buf);
    buf->buf = strdup(new_text);
    free(buf->md5);
    buf->md5 = md5(buf->buf, buf->len);

    log_debug("buf: %s", buf->buf);

    return 1;
}
