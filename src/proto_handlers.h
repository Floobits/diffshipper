#ifndef PROTO_HANDLERS_H
#define PROTO_HANDLERS_H

#include <jansson.h>

void on_get_buf(json_t *json_obj);
void on_msg(json_t *json_obj);
void on_patch(json_t *json_obj);
void on_room_info(json_t *json_obj);

#endif
