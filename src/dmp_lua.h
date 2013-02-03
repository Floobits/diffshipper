#ifndef DMPLUA_H
#define DMPLUA_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "buf.h"


lua_State *init_lua_state();

int apply_patch(lua_State *l, buf_t *buf, char *patch_text);
char *make_patch(lua_State *l, const char *old, const char *new);

#endif
