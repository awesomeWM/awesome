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

#include <lua.h>
#include <lauxlib.h>

#include "common/util.h"

/** Lua function to call on dofunction() error */
lua_CFunction lualib_dofunction_on_error;

void luaA_checkfunction(lua_State *, int);
void luaA_checktable(lua_State *, int);

/** Dump the Lua stack. Useful for debugging.
 * \param L The Lua VM state.
 */
void luaA_dumpstack(lua_State *);

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

static inline int
luaA_dofunction_error(lua_State *L)
{
    if(lualib_dofunction_on_error)
        return lualib_dofunction_on_error(L);
    return 0;
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
    /* Move function before arguments */
    lua_insert(L, - nargs - 1);
    /* Push error handling function */
    lua_pushcfunction(L, luaA_dofunction_error);
    /* Move error handling function before args and function */
    lua_insert(L, - nargs - 2);
    int error_func_pos = lua_gettop(L) - nargs - 1;
    if(lua_pcall(L, nargs, nret, - nargs - 2))
    {
        warn("%s", lua_tostring(L, -1));
        /* Remove error function and error string */
        lua_pop(L, 2);
        return false;
    }
    /* Remove error function */
    lua_remove(L, error_func_pos);
    return true;
}

/** Call a registered function. Its arguments are the complete stack contents.
 * \param L The Lua VM state.
 * \param handler The function to call.
 * \return The number of elements pushed on stack.
 */
static inline
int luaA_call_handler(lua_State *L, int handler)
{
    /* This is based on luaA_dofunction, but allows multiple return values */
    assert(handler != LUA_REFNIL);

    int nargs = lua_gettop(L);

    /* Push error handling function and move it before args */
    lua_pushcfunction(L, luaA_dofunction_error);
    lua_insert(L, - nargs - 1);
    int error_func_pos = 1;

    /* push function and move it before args */
    lua_rawgeti(L, LUA_REGISTRYINDEX, handler);
    lua_insert(L, - nargs - 1);

    if(lua_pcall(L, nargs, LUA_MULTRET, error_func_pos))
    {
        warn("%s", lua_tostring(L, -1));
        /* Remove error function and error string */
        lua_pop(L, 2);
        return 0;
    }
    /* Remove error function */
    lua_remove(L, error_func_pos);
    return lua_gettop(L);
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
