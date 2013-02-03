#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "buf.h"
#include "dmp_lua.h"
#include "log.h"
#include "util.h"

#include "dmp_lua_str.h"


lua_State *init_lua_state() {
    int rv;
    lua_State *l = luaL_newstate();
    luaL_openlibs(l);
    rv = luaL_loadbuffer(l, lua_diff_match_patch_str, lua_diff_match_patch_str_size, "diff_match_patch");
    if (rv) {
        const char* lua_err = lua_tostring(l, -1);
        die("couldn't load diff_match_patch.lua: %s", lua_err);
    }
    rv = lua_pcall(l, 0, 0, 0);
    if (rv) {
        const char* lua_err = lua_tostring(l, -1);
        die("couldn't require diff_match_patch: %s", lua_err);
    }
    log_debug("Loaded lua successfully");
    return l;
}


int apply_patch(lua_State *l, buf_t *buf, char *patch_text) {
    const char *new_text;
    int clean_patch;
    lua_getglobal(l, "floo_apply_patch");
    lua_pushstring(l, buf->buf);
    lua_pushstring(l, patch_text);
    if (lua_pcall(l, 2, 2, 0))
        die("error calling lua: %s", lua_tostring(l, -1));

    clean_patch = lua_toboolean(l, -2);
    new_text = lua_tostring(l, -1);

    log_debug("clean patch: %i. new text: %s", clean_patch, new_text);
    lua_settop(l, 0);

    free(buf->buf);
    buf->buf = strdup(new_text);
    buf->len = strlen(buf->buf);
    free(buf->md5);
    buf->md5 = md5(buf->buf, buf->len);

    return clean_patch;
};


char *make_patch(lua_State *l, const char *old, const char *new) {
    char *patch_text;

    lua_getglobal(l, "floo_make_patch");
    lua_pushstring(l, old);
    lua_pushstring(l, new);

    if (lua_pcall(l, 2, 1, 0))
        die("error calling lua: %s", lua_tostring(l, -1));

    patch_text = strdup(lua_tostring(l, -1));
    log_debug("patch text is %s", patch_text);
    lua_settop(l, 0);
    return patch_text;
};
