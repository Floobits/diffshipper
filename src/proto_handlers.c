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
    int rv;
    json_error_t json_err;
    buf_t buf;
    rv = json_unpack_ex(json_obj, &json_err, 0, "{s:s s:s s:s}", "buf", &(buf.buf), "md5", &(buf.md5), "path", &(buf.path));
    if (rv != 0) {
        log_json_err(&json_err);
        die("Avenge me, Othello! Shiiiiiiiiiiiiit!");
    }
    rv = set_buf(&buf);
    if (rv)
        die("error setting buffer");
}


void on_msg(json_t *json_obj) {
    int rv;
    json_error_t json_err;
    char *username;
    char *msg;

    rv = json_unpack_ex(json_obj, &json_err, 0, "{s:s s:s}", "username", &username, "data", &msg);
    if (rv != 0) {
        log_json_err(&json_err);
        return;
    }

    log_msg("Message from user %s: %s", username, msg);

    free(username);
    free(msg);
}


void on_patch(json_t *json_obj) {
    int rv;
    json_error_t json_err;
    int buf_id;
    int user_id;
    char *username;
    char *md5_before;
    char *md5_after;
    char *patch_str;
    char *path;
    rv = json_unpack_ex(
        json_obj, &json_err, 0, "{s:i s:i s:s s:s s:s s:s s:s}",
        "id", &buf_id,
        "user_id", &user_id,
        "username", &username,
        "patch", &patch_str,
        "path", &path,
        "md5_before", &md5_before,
        "md5_after", &md5_after
    );
    if (rv != 0) {
        log_json_err(&json_err);
        return;
    }
    ignore_path(path);
    apply_patch(buf_id, patch_str);
}


void on_room_info(json_t *json_obj) {
    int rv;
    json_error_t json_err;
    const char *buf_id_str;
    json_t *bufs_obj;
    json_t *buf_obj;
    rv = json_unpack_ex(json_obj, &json_err, 0, "{s:o}", "bufs", &bufs_obj);
    if (rv != 0) {
        log_json_err(&json_err);
        die("Avenge me, Othello! Shiiiiiiiiiiiiit!");
    }
    json_object_foreach(bufs_obj, buf_id_str, buf_obj) {
        send_json("{s:s s:i}", "name", "get_buf", "id", atoi(buf_id_str));
    }
}
