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

#include "key.h"

/** Define a global key binding. This key binding will always be available.
 * (DEPRECATED, see key).
 * \param L The Lua VM state.
 * \luastack
 * \lparam A table with modifier keys.
 * \lparam A key name.
 * \lparam A function to execute on key press.
 * \lparam A function to execute on key release.
 * \lreturn The key.
 */
static int
luaA_keybinding_new(lua_State *L)
{
    luaA_deprecate(L, "key");
    return luaA_key_new(L);
}

const struct luaL_reg awesome_keybinding_methods[] =
{
    { "__call", luaA_keybinding_new },
    { NULL, NULL }
};
