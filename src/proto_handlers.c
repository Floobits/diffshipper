#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <jansson.h>

#include "buf.h"
#include "diff.h"
#include "log.h"
#include "net.h"
#include "options.h"
#include "proto_handlers.h"
#include "util.h"


void on_get_buf(json_t *json_obj) {
    buf_t buf;
    parse_json(json_obj, "{s:s s:s s:s}", "buf", &(buf.buf), "md5", &(buf.md5), "path", &(buf.path));
    save_buf(&buf);
}


void on_join(json_t *json_obj) {
    char *username;
    parse_json(json_obj, "{s:s}", "username", &username);
    log_msg("User %s joined the room", username);
}


void on_msg(json_t *json_obj) {
    char *username;
    char *msg;

    parse_json(json_obj, "{s:s s:s}", "username", &username, "data", &msg);
    log_msg("Message from user %s: %s", username, msg);
}


void on_part(json_t *json_obj) {
    char *username;

    parse_json(json_obj, "{s:s}", "username", &username);
    log_msg("User %s left the room", username);
}


void on_patch(json_t *json_obj) {
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
    send_json("{s:s s:i}", "name", "get_buf", "id", buf_id);
    /*
    ignore_path(path);
    apply_patch(bufs[buf_id], patch_str);*/
}


void on_room_info(json_t *json_obj) {
    const char *buf_id_str;
    json_t *bufs_obj;
    json_t *buf_obj;
    parse_json(json_obj, "{s:o}", "bufs", &bufs_obj);
    json_object_foreach(bufs_obj, buf_id_str, buf_obj) {
        send_json("{s:s s:i}", "name", "get_buf", "id", atoi(buf_id_str));
    }
}
