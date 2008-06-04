/*
 * lua.h - Lua configuration management header
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

#ifndef AWESOME_LUA_H
#define AWESOME_LUA_H

#include <lua.h>
#include <lauxlib.h>

#include "common/util.h"

/** Object types */
typedef enum
{
    AWESOME_TYPE_STATUSBAR = 1,
    AWESOME_TYPE_TITLEBAR
} awesome_type_t;

/** Type for Lua function */
typedef int luaA_function;

#define luaA_dostring(L, cmd) \
    do { \
        if(cmd) \
            if(luaL_dostring(L, cmd)) \
                warn("error executing Lua code: %s", \
                     lua_tostring(L, -1)); \
    } while(0)

#define luaA_dofunction(L, f, n) \
    do { \
        if(f) \
        { \
            lua_rawgeti(L, LUA_REGISTRYINDEX, f); \
            if(n) \
                lua_insert(L, - (n + 1)); \
            if(lua_pcall(L, n, 0, 0)) \
                warn("error running function: %s", \
                     lua_tostring(L, -1)); \
        } \
    } while(0)

#define luaA_checktable(L, n) \
    do { \
        if(!lua_istable(L, n)) \
            luaL_typerror(L, n, "table"); \
    } while(0)

#define luaA_checkfunction(L, n) \
    do { \
        if(!lua_isfunction(L, n)) \
            luaL_typerror(L, n, "function"); \
    } while(0)

#define luaA_checkscreen(screen) \
    do { \
        if(screen < 0 || screen >= globalconf.screens_info->nscreen) \
            luaL_error(L, "invalid screen number: %d\n", screen); \
    } while(0)

/** Check that an object is not a NULL reference.
 * \param L The Lua state.
 * \param ud The index of the object in the stack.
 * \param tname The type name.
 * \return A pointer to the object.
 */
static inline void *
luaA_checkudata(lua_State *L, int ud, const char *tname)
{
    void **p = luaL_checkudata(L, ud, tname);
    if(*p)
        return p;
    luaL_error(L, "invalid object");
    return NULL;
}

static inline lua_Number
luaA_getopt_number(lua_State *L, int idx, const char *name, lua_Number def)
{
    /* assume that table is first on stack */
    lua_getfield(L, idx, name);
    /* return luaL_optnumber result */
    return luaL_optnumber(L, -1, def);
}

static inline const char *
luaA_getopt_string(lua_State *L, int idx, const char *name, const char *def)
{
    /* assume that table is first on stack */
    lua_getfield(L, idx, name);
    /* return luaL_optnumber result */
    return luaL_optstring(L, -1, def);
}

static inline int
luaA_settype(lua_State *L, const char *type)
{
    luaL_getmetatable(L, type);
    lua_setmetatable(L, -2);
    return 1;
}

static inline char *
luaA_name_init(lua_State *L)
{
    lua_getfield(L, 1, "name");
    return a_strdup(luaL_checkstring(L, -1));
}

static inline bool
luaA_checkboolean(lua_State *L, int n)
{
    if(!lua_isboolean(L, n))
        luaL_typerror(L, n, "boolean");
    return lua_toboolean(L, n);
}

bool luaA_parserc(const char *);
void luaA_docmd(char *);
void luaA_pushpointer(void *, awesome_type_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
