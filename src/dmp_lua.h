#ifndef DMPLUA_H
#define DMPLUA_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "buf.h"

#define LUA_BITLIBNAME "bit"

LUALIB_API int luaopen_bit(lua_State *L);

static const luaL_Reg bitop_lib[] = {
  {LUA_BITLIBNAME, luaopen_bit},
  {NULL, NULL},
};

lua_State *l_ap;
lua_State *l_mp;

lua_State *init_lua_state();

int apply_patch(lua_State *l, buf_t *buf, char *patch_text);
char *make_patch(lua_State *l, const char *old, const char *new);

#endif
