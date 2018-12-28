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

static inline void free_string(char **c)
{
    p_delete(c);
}

DO_ARRAY(char*, string, free_string)

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
    fprintf(stderr, "%s%sW: ", a_current_time_str(), lua_tostring(L, -1));
    lua_pop(L, 1);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");

#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
#endif
}

static inline int
luaA_typerror(lua_State *L, int narg, const char *tname)
{
    const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                      tname, luaL_typename(L, narg));
#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    lua_concat(L, 2);
#endif
    return luaL_argerror(L, narg, msg);
}

static inline int
luaA_rangerror(lua_State *L, int narg, double min, double max)
{
    const char *msg = lua_pushfstring(L, "value in [%f, %f] expected, got %f",
                                      min, max, (double) lua_tonumber(L, narg));
#if LUA_VERSION_NUM >= 502
    luaL_traceback(L, L, NULL, 2);
    lua_concat(L, 2);
#endif
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

static inline lua_Number
luaA_checknumber_range(lua_State *L, int n, lua_Number min, lua_Number max)
{
    lua_Number result = lua_tonumber(L, n);
    if (result < min || result > max)
        luaA_rangerror(L, n, min, max);
    return result;
}

static inline lua_Number
luaA_optnumber_range(lua_State *L, int narg, lua_Number def, lua_Number min, lua_Number max)
{
    if (lua_isnoneornil(L, narg))
        return def;
    return luaA_checknumber_range(L, narg, min, max);
}

static inline lua_Number
luaA_getopt_number_range(lua_State *L, int idx, const char *name, lua_Number def, lua_Number min, lua_Number max)
{
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1))
        def = luaA_optnumber_range(L, -1, def, min, max);
    lua_pop(L, 1);
    return def;
}

static inline int
luaA_checkinteger(lua_State *L, int n)
{
    lua_Number d = lua_tonumber(L, n);
    if (d != (int)d)
        luaA_typerror(L, n, "integer");
    return (int)d;
}

static inline lua_Integer
luaA_optinteger (lua_State *L, int narg, lua_Integer def)
{
    return luaL_opt(L, luaA_checkinteger, narg, def);
}

static inline int
luaA_getopt_integer(lua_State *L, int idx, const char *name, lua_Integer def)
{
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1))
        def = luaA_optinteger(L, -1, def);
    lua_pop(L, 1);
    return def;
}

static inline int
luaA_checkinteger_range(lua_State *L, int n, lua_Number min, lua_Number max)
{
    int result = luaA_checkinteger(L, n);
    if (result < min || result > max)
        luaA_rangerror(L, n, min, max);
    return result;
}

static inline lua_Integer
luaA_optinteger_range(lua_State *L, int narg, lua_Integer def, lua_Number min, lua_Number max)
{
    if (lua_isnoneornil(L, narg))
        return def;
    return luaA_checkinteger_range(L, narg, min, max);
}

static inline int
luaA_getopt_integer_range(lua_State *L, int idx, const char *name, lua_Integer def, lua_Number min, lua_Number max)
{
    lua_getfield(L, idx, name);
    if (lua_isnil(L, -1) || lua_isnumber(L, -1))
        def = luaA_optinteger_range(L, -1, def, min, max);
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
    lua_pushinteger(L, geometry.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, geometry.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, geometry.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, geometry.height);
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

typedef bool luaA_config_callback(const char *);

void luaA_init(xdgHandle *, string_array_t *);
const char *luaA_find_config(xdgHandle *, const char *, luaA_config_callback *);
bool luaA_parserc(xdgHandle *, const char *);

/** Global signals */
signal_array_t global_signals;

int luaA_class_index_miss_property(lua_State *, lua_object_t *);
int luaA_class_newindex_miss_property(lua_State *, lua_object_t *);
int luaA_default_index(lua_State *);
int luaA_default_newindex(lua_State *);
void luaA_emit_startup(void);

void luaA_systray_invalidate(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
