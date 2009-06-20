/*
 * lualib.h - useful functions and type for Lua
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

#ifndef AWESOME_COMMON_LUALIB
#define AWESOME_COMMON_LUALIB

#include <lauxlib.h>
#include "common/util.h"

#define luaA_checkfunction(L, n) \
    do { \
        if(!lua_isfunction(L, n)) \
            luaL_typerror(L, n, "function"); \
    } while(0)

/** Convert s stack index to positive.
 * \param L The Lua VM state.
 * \param ud The index.
 * \return A positive index.
 */
static inline int
luaA_absindex(lua_State *L, int ud)
{
    return (ud > 0 || ud <= LUA_REGISTRYINDEX) ? ud : lua_gettop(L) + ud + 1;
}

/** Execute an Lua function on top of the stack.
 * \param L The Lua stack.
 * \param nargs The number of arguments for the Lua function.
 * \param nret The number of returned value from the Lua function.
 * \return True on no error, false otherwise.
 */
static inline bool
luaA_dofunction(lua_State *L, int nargs, int nret)
{
    if(nargs)
        lua_insert(L, - (nargs + 1));
    if(lua_pcall(L, nargs, nret, 0))
    {
        warn("error running function: %s",
             lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
