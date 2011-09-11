/*
 * strut.c - strut management
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

#include "strut.h"
#include "luaa.h"

/** Push a strut type to a table on stack.
 * \param L The Lua VM state.
 * \param strut The strut to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushstrut(lua_State *L, strut_t strut)
{
    lua_createtable(L, 4, 0);
    lua_pushnumber(L, strut.left);
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, strut.right);
    lua_setfield(L, -2, "right");
    lua_pushnumber(L, strut.top);
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, strut.bottom);
    lua_setfield(L, -2, "bottom");
    return 1;
}

/** Convert a table to a strut_t structure.
 * \param L The Lua VM state.
 * \param idx The index of the table on the stack.
 * \param strut The strut to fill. Current values will be used as default.
 */
void
luaA_tostrut(lua_State *L, int idx, strut_t *strut)
{
    luaA_checktable(L, idx);
    strut->left = luaA_getopt_number(L, idx, "left", strut->left);
    strut->right = luaA_getopt_number(L, idx, "right", strut->right);
    strut->top = luaA_getopt_number(L, idx, "top", strut->top);
    strut->bottom = luaA_getopt_number(L, idx, "bottom", strut->bottom);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
