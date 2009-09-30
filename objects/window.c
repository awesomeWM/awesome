/*
 * window.c - window object
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#include "luaa.h"
#include "xwindow.h"
#include "objects/window.h"
#include "common/luaobject.h"

LUA_CLASS_FUNCS(window, window_class)

/** Set a window opacity.
 * \param L The Lua VM state.
 * \param idx The index of the window on the stack.
 * \param opacity The opacity value.
 */
void
window_set_opacity(lua_State *L, int idx, double opacity)
{
    window_t *window = luaA_checkudata(L, idx, &window_class);

    if(window->opacity != opacity)
    {
        window->opacity = opacity;
        xwindow_set_opacity(window->window, opacity);
        luaA_object_emit_signal(L, idx, "property::opacity", 0);
    }
}

/** Set a window opacity.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_set_opacity(lua_State *L, window_t *window)
{
    if(lua_isnil(L, -1))
        window_set_opacity(L, -3, -1);
    else
    {
        double d = luaL_checknumber(L, -1);
        if(d >= 0 && d <= 1)
            window_set_opacity(L, -3, d);
    }
    return 0;
}

/** Get the window opacity.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_get_opacity(lua_State *L, window_t *window)
{
    if(window->opacity >= 0)
    {
        lua_pushnumber(L, window->opacity);
        return 1;
    }
    return 0;
}

LUA_OBJECT_EXPORT_PROPERTY(window, window_t, window, lua_pushnumber)

void
window_class_setup(lua_State *L)
{
    static const struct luaL_reg window_methods[] =
    {
        { NULL, NULL }
    };

    static const struct luaL_reg window_meta[] =
    {
        { NULL, NULL }
    };

    luaA_class_setup(L, &window_class, "window", NULL,
                     NULL, NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     window_methods, window_meta);

    luaA_class_add_property(&window_class, A_TK_WINDOW,
                            NULL,
                            (lua_class_propfunc_t) luaA_window_get_window,
                            NULL);
    luaA_class_add_property(&window_class, A_TK_OPACITY,
                            (lua_class_propfunc_t) luaA_window_set_opacity,
                            (lua_class_propfunc_t) luaA_window_get_opacity,
                            (lua_class_propfunc_t) luaA_window_set_opacity);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
