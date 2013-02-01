#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "buf.h"
#include "dmp_lua.h"
#include "log.h"
#include "util.h"

const char* make_patch(const char* old, const char* new) {
    int rv;
    lua_getglobal(l, "make_patch");
    lua_pushstring(l, old);
    lua_pushstring(l, new);
    rv = lua_pcall(l, 2, 1, 0);
    if (rv) {
        die("error calling lua: %s", lua_tostring(l, -1));
    }
    const char *patch_text = strdup(lua_tostring(l, -1));
    log_debug("patch text is %s", patch_text);
    lua_settop(l, 0);
    return patch_text;
};


int apply_patch(buf_t *buf, char *patch_text) {
    const char *new_text;
    int clean_patch;
    lua_getglobal(l, "apply_patch");
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
