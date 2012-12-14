#include <stdlib.h>
#include <xlocale.h>

#include <jansson.h>

#include "log.h"
#include "net.h"
#include "proto_handlers.h"


void on_room_info(json_t *json_obj) {
    int rv;
    const char *buf_id_str;
    json_t *bufs_obj;
    json_t *buf_obj;
    json_error_t json_err;
    rv = json_unpack_ex(json_obj, &json_err, 0, "{s:o}", "bufs", &bufs_obj);
    if (rv != 0) {
        log_json_err(&json_err);
        die("Avenge me, Othello! Shiiiiiiiiiiiiit!");
    }
    json_object_foreach(bufs_obj, buf_id_str, buf_obj) {
        /* TODO: check return value */
        send_json("{s:s s:i}", "name", "get_buf", "id", atoi(buf_id_str));
    }
}
