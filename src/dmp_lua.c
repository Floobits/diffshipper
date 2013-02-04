#include <string.h>

#include "lualib.h"
#include "lua.h"
#include "lauxlib.h"

#include "buf.h"
#include "dmp_lua.h"
#include "log.h"
#include "util.h"

lua_State *init_lua_state() {
    int rv;
    lua_State *l = luaL_newstate();
    luaL_openlibs(l);
    luaL_openlib(l, "bitop", bitop_lib, 0);
    rv = luaL_loadfile(l, "src/lua/init.lua");
    if (rv) {
        const char* lua_err = lua_tostring(l, -1);
        die("couldn't load init.lua: %s", lua_err);
    }
    rv = lua_pcall(l, 0, 0, 0);
    if (rv) {
        const char* lua_err = lua_tostring(l, -1);
        die("couldn't require diff_match_patch: %s", lua_err);
    }
    log_debug("Loaded lua successfully");
}


int apply_patch(buf_t *buf, char *patch_text) {
    const char *new_text;
    int clean_patch;
    lua_getglobal(l_ap, "apply_patch");
    lua_pushstring(l_ap, buf->buf);
    lua_pushstring(l_ap, patch_text);
    if (lua_pcall(l_ap, 2, 2, 0))
        die("error calling lua: %s", lua_tostring(l_ap, -1));

    clean_patch = lua_toboolean(l_ap, -2);
    new_text = lua_tostring(l_ap, -1);

    log_debug("clean patch: %i. new text: %s", clean_patch, new_text);
    lua_settop(l_ap, 0);

    free(buf->buf);
    buf->buf = strdup(new_text);
    buf->len = strlen(buf->buf);
    free(buf->md5);
    buf->md5 = md5(buf->buf, buf->len);

    return clean_patch;
};


char *make_patch(const char *old, const char *new) {
    char *patch_text;

    lua_getglobal(l_mp, "make_patch");
    lua_pushstring(l_mp, old);
    lua_pushstring(l_mp, new);

    if (lua_pcall(l_mp, 2, 1, 0))
        die("error calling lua: %s", lua_tostring(l_mp, -1));

    patch_text = strdup(lua_tostring(l_mp, -1));
    log_debug("patch text is %s", patch_text);
    lua_settop(l_mp, 0);
    return patch_text;
};
