#ifndef DMPLUA_H
#define DMPLUA_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "buf.h"

lua_State *l;

char *make_patch(const char *old, const char *new);
int apply_patch(buf_t *buf, char *patch_text);

#endif
