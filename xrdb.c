/*
 * xrdb.c - X Resources DataBase communication functions
 *
 * Copyright © 2015 Yauhen Kirylau <yawghen@gmail.com>
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

#include "xrdb.h"
#include "globalconf.h"

#include <string.h>

/* \brief get value from X Resources DataBase
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam string xrdb class, ie "URxvt" or ""
 * \lparam string xrdb name, ie "background" or "color0"
 * \lreturn string xrdb value or nil if not exists. \
 */
int luaA_xrdb_get_value(lua_State *L) {
    const char *resource_class = luaL_checkstring(L, 1);
    const char *resource_name = luaL_checkstring(L, 2);
    char *result = NULL;

    if (xcb_xrm_resource_get_string(globalconf.xrmdb, resource_name, resource_class, &result) < 0 ) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, result);
        p_delete(&result);
    }

    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
