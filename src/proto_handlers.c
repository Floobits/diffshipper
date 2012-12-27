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
    buf_t *tmp = malloc(sizeof(buf_t));

    parse_json(json_obj, "{s:i s:s s:s s:s}",
        "id", &(tmp->id),
        "buf", &(tmp->buf),
        "md5", &(tmp->md5),
        "path", &(tmp->path)
    );

    buf = get_buf_by_id(tmp->id);
    if (buf) {
        buf->id = tmp->id;
        buf->buf = tmp->buf; /* TODO: memory leak */
        buf->md5 = tmp->md5;
        buf->path = tmp->path; /* TODO: ditto */
        free(tmp);
    } else {
        buf = tmp;
        add_buf_to_bufs(buf);
    }

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
    /* TODO: we don't know how to apply patches, so let's just re-get the buf each time */
    /*send_json("{s:s s:i}", "name", "get_buf", "id", buf_id);*/

    ignore_path(path);
    apply_patch(bufs[buf_id], patch_str);
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

    while (TRUE) {
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
