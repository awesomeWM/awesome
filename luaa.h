/*
 * luaa.h - Lua configuration management header
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include <basedir.h>

#include "draw.h"
#include "common/lualib.h"
#include "common/luaclass.h"

#define luaA_deprecate(L, repl) \
    do { \
        luaA_warn(L, "%s: This function is deprecated and will be removed, see %s", \
                  __FUNCTION__, repl); \
        lua_pushlstring(L, __FUNCTION__, sizeof(__FUNCTION__)); \
        signal_object_emit(L, &global_signals, "debug::deprecation", 1); \
    } while(0)

#define luaA_checkscreen(screen) \
    do { \
        if(screen < 0 || screen >= globalconf.screens.len) \
            luaL_error(L, "invalid screen number: %d", screen + 1); \
    } while(0)

/** Print a warning about some Lua code.
 * This is less mean than luaL_error() which setjmp via lua_error() and kills
 * everything. This only warn, it's up to you to then do what's should be done.
 * \param L The Lua VM state.
 * \param fmt The warning message.
 */
static inline void __attribute__ ((format(printf, 2, 3)))
luaA_warn(lua_State *L, const char *fmt, ...)
{
    va_list ap;
    luaL_where(L, 1);
    fprintf(stderr, "%sW: ", lua_tostring(L, -1));
    lua_pop(L, 1);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static inline int
luaA_typerror(lua_State *L, int narg, const char *tname)
{
    const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                      tname, luaL_typename(L, narg));
    return luaL_argerror(L, narg, msg);
}

static inline void
luaA_getuservalue(lua_State *L, int idx)
{
#if LUA_VERSION_NUM >= 502
    lua_getuservalue(L, idx);
#else
    lua_getfenv(L, idx);
#endif
} 

static inline void
luaA_setuservalue(lua_State *L, int idx)
{
#if LUA_VERSION_NUM >= 502
    lua_setuservalue(L, idx);
#else
    lua_setfenv(L, idx);
#endif
} 

static inline size_t
luaA_rawlen(lua_State *L, int idx)
{
#if LUA_VERSION_NUM >= 502
    return lua_rawlen(L, idx);
#else
    return lua_objlen(L, idx);
#endif
}

static inline void
luaA_registerlib(lua_State *L, const char *libname, const luaL_Reg *l)
{
#if LUA_VERSION_NUM >= 502
    if (libname)
    {
        lua_newtable(L);
        luaL_setfuncs(L, l, 0);
        lua_pushvalue(L, -1);
        lua_setglobal(L, libname);
    }
    else
        luaL_setfuncs(L, l, 0);
#else
    luaL_register(L, libname, l);
#endif
}

static inline bool
luaA_checkboolean(lua_State *L, int n)
{
    if(!lua_isboolean(L, n))
        luaA_typerror(L, n, "boolean");
    return lua_toboolean(L, n);
}

static inline lua_Number
luaA_getopt_number(lua_State *L, int idx, const char *name, lua_Number def)
{
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1))
        def = luaL_optnumber(L, -1, def);
    lua_pop(L, 1);
    return def;
}

/** Push a area type to a table on stack.
 * \param L The Lua VM state.
 * \param geometry The area geometry to push.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_pusharea(lua_State *L, area_t geometry)
{
    lua_createtable(L, 0, 4);
    lua_pushnumber(L, geometry.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, geometry.y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, geometry.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, geometry.height);
    lua_setfield(L, -2, "height");
    return 1;
}

/** Register an Lua object.
 * \param L The Lua stack.
 * \param idx Index of the object in the stack.
 * \param ref A int address: it will be filled with the int
 * registered. If the address points to an already registered object, it will
 * be unregistered.
 * \return Always 0.
 */
static inline int
luaA_register(lua_State *L, int idx, int *ref)
{
    lua_pushvalue(L, idx);
    if(*ref != LUA_REFNIL)
        luaL_unref(L, LUA_REGISTRYINDEX, *ref);
    *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Unregister a Lua object.
 * \param L The Lua stack.
 * \param ref A reference to an Lua object.
 */
static inline void
luaA_unregister(lua_State *L, int *ref)
{
    luaL_unref(L, LUA_REGISTRYINDEX, *ref);
    *ref = LUA_REFNIL;
}

/** Register a function.
 * \param L The Lua stack.
 * \param idx Index of the function in the stack.
 * \param fct A int address: it will be filled with the int
 * registered. If the address points to an already registered function, it will
 * be unregistered.
 * \return luaA_register value.
 */
static inline int
luaA_registerfct(lua_State *L, int idx, int *fct)
{
    luaA_checkfunction(L, idx);
    return luaA_register(L, idx, fct);
}

void luaA_init(xdgHandle *);
bool luaA_parserc(xdgHandle *, const char *, bool);

/** Global signals */
signal_array_t global_signals;

int luaA_class_index_miss_property(lua_State *, lua_object_t *);
int luaA_class_newindex_miss_property(lua_State *, lua_object_t *);
int luaA_default_index(lua_State *);
int luaA_default_newindex(lua_State *);

void luaA_systray_invalidate(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
