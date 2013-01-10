#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xlocale.h>

#include <jansson.h>

#include "buf.h"
#include "log.h"
#include "net.h"
#include "proto_handlers.h"
#include "util.h"


static void on_get_buf(json_t *json_obj) {
    buf_t *buf;
    buf_t tmp;

    parse_json(json_obj, "{s:i s:s s:s s:s}",
        "id", &(tmp.id),
        "buf", &(tmp.buf),
        "md5", &(tmp.md5),
        "path", &(tmp.path)
    );

    buf = get_buf_by_id(tmp.id);
    if (buf == NULL) {
        buf = calloc(1, sizeof(buf_t));
        buf->id = tmp.id;
        /* Strings from parse_json disappear once json_obj has 0 refcount.
         * This happens at the end of the loop in remote_change_worker.
         */
        buf->buf = strdup(tmp.buf);
        buf->md5 = strdup(tmp.md5);
        buf->path = strdup(tmp.path);
        add_buf_to_bufs(buf);
    }
    if (buf->buf)
        free(buf->buf);
    if (buf->md5)
        free(buf->md5);
    if (buf->path)
        free(buf->path);

    buf->id = tmp.id;
    buf->buf = strdup(tmp.buf);
    buf->md5 = strdup(tmp.md5);
    buf->path = strdup(tmp.path);

    save_buf(buf);
}


static void on_join(json_t *json_obj) {
    char *username;
    parse_json(json_obj, "{s:s}", "username", &username);
    log_msg("User %s joined the room", username);
}


static void on_msg(json_t *json_obj) {
    char *username;
    char *msg;

    parse_json(json_obj, "{s:s s:s}", "username", &username, "data", &msg);
    log_msg("Message from user %s: %s", username, msg);
}


static void on_part(json_t *json_obj) {
    char *username;

    parse_json(json_obj, "{s:s}", "username", &username);
    log_msg("User %s left the room", username);
}


static void on_patch(json_t *json_obj) {
    int buf_id;
    int user_id;
    char *username;
    char *md5_before;
    char *md5_after;
    char *patch_str;
    char *path;
    int rv;

    parse_json(
        json_obj, "{s:i s:i s:s s:s s:s s:s s:s}",
        "id", &buf_id,
        "user_id", &user_id,
        "username", &username,
        "patch", &patch_str,
        "path", &path,
        "md5_before", &md5_before,
        "md5_after", &md5_after
    );

    log_debug("patch for buf %i (%s) from %s (id %i)", buf_id, path, username, user_id);

    buf_t *buf = get_buf_by_id(buf_id);
    if (buf == NULL) {
        die("we got a patch for a nonexistent buf id: %i", buf_id);
        return;
    }
    rv = apply_patch(buf, patch_str);
    if (rv != 1) {
        send_json("{s:s s:i}", "name", "get_buf", "id", buf_id);
        return;
    }
    if (strcmp(buf->md5, md5_after) != 0) {
        log_err("Expected md5 %s but got %s after patching", md5_after, buf->md5);
        send_json("{s:s s:i}", "name", "get_buf", "id", buf_id);
        return;
    }
    save_buf(buf);
}


static void on_room_info(json_t *json_obj) {
    const char *buf_id_str;
    json_t *bufs_obj;
    json_t *buf_obj;
    parse_json(json_obj, "{s:o}", "bufs", &bufs_obj);
    json_object_foreach(bufs_obj, buf_id_str, buf_obj) {
        send_json("{s:s s:i}", "name", "get_buf", "id", atoi(buf_id_str));
    }
}


void *remote_change_worker() {
    char *name;

    pthread_cond_wait(&server_conn_ready, &server_conn_mtx);
    pthread_mutex_unlock(&server_conn_mtx);

    json_t *json_obj;

    while (1) {
        json_obj = recv_json();
        parse_json(json_obj, "{s:s}", "name", &name);
        log_debug("name: %s", name);
        /* TODO: handle create/rename/delete buf */
        if (strcmp(name, "room_info") == 0) {
            on_room_info(json_obj);
        } else if (strcmp(name, "get_buf") == 0) {
            on_get_buf(json_obj);
        } else if (strcmp(name, "join") == 0) {
            on_join(json_obj);
        } else if (strcmp(name, "msg") == 0) {
            on_msg(json_obj);
        } else if (strcmp(name, "part") == 0) {
            on_part(json_obj);
        } else if (strcmp(name, "patch") == 0) {
            on_patch(json_obj);
        } else {
            log_err("Unknown event name: %s", name);
        }

        json_decref(json_obj);
    }

    pthread_exit(NULL);
    return NULL;
}
