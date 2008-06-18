/*
 * keybinding.c - Key bindings configuration management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/* XStringToKeysym() */
#include <X11/Xlib.h>

#include "structs.h"
#include "lua.h"
#include "window.h"

extern awesome_t globalconf;

DO_LUA_NEW(static, keybinding_t, keybinding, "keybinding", keybinding_ref)
DO_LUA_GC(keybinding_t, keybinding, "keybinding", keybinding_unref)

static void
__luaA_keystore(keybinding_t *key, const char *str)
{
    xcb_keycode_t kc;
    int ikc;

    if(!a_strlen(str))
        return;
    else if(a_strncmp(str, "#", 1))
        key->keysym = XStringToKeysym(str);
    else
    {
        ikc = atoi(str + 1);
        memcpy(&kc, &ikc, sizeof(KeyCode));
        key->keycode = kc;
    }
}

/** Define a global key binding. This key binding will always be available.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with modifier keys.
 * \lparam A key name.
 * \lparam A function to execute.
 * \lreturn The keybinding.
 */
static int
luaA_keybinding_new(lua_State *L)
{
    size_t i, len;
    keybinding_t *k;
    const char *key;

    /* arg 1 is key mod table */
    luaA_checktable(L, 1);
    /* arg 2 is key */
    key = luaL_checkstring(L, 2);
    /* arg 3 is cmd to run */
    luaA_checkfunction(L, 3);

    /* get the last arg as function */
    k = p_new(keybinding_t, 1);
    __luaA_keystore(k, key);
    k->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 1);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 1, i);
        k->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    return luaA_keybinding_userdata_new(L, k);
}

/** Add a global key binding. This key binding will always be available.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A keybinding.
 */
static int
luaA_keybinding_add(lua_State *L)
{
    keybinding_t *key, **k = luaA_checkudata(L, 1, "keybinding");

    /* Check that the keybinding has not been already added. */
    for(key = globalconf.keys; key; key = key->next)
        if(key == *k)
            luaL_error(L, "keybinding already added");

    keybinding_list_push(&globalconf.keys, *k);

    keybinding_ref(k);

    window_root_grabkey(*k);

    return 0;
}

/** Remove a global key binding.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A keybinding.
 */
static int
luaA_keybinding_remove(lua_State *L)
{
    keybinding_t **k = luaA_checkudata(L, 1, "keybinding");

    keybinding_list_detach(&globalconf.keys, *k);

    keybinding_unref(k);

    window_root_ungrabkey(*k);

    return 0;
}

/** Convert a keybinding to a printable string.
 * \return A string.
 */
static int
luaA_keybinding_tostring(lua_State *L)
{
    keybinding_t **p = luaA_checkudata(L, 1, "keybinding");
    lua_pushfstring(L, "[keybinding udata(%p)]", *p);
    return 1;
}

const struct luaL_reg awesome_keybinding_methods[] =
{
    { "new", luaA_keybinding_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_keybinding_meta[] =
{
    {"add", luaA_keybinding_add },
    {"remove", luaA_keybinding_remove },
    {"__tostring", luaA_keybinding_tostring },
    {"__gc", luaA_keybinding_gc },
    { NULL, NULL },
};

