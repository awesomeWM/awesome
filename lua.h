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

#include <ev.h>
#include <lua.h>
#include <lauxlib.h>

#include "common/util.h"
#include "common/draw.h"

/** Object types */
typedef enum
{
    AWESOME_TYPE_STATUSBAR = 1,
    AWESOME_TYPE_TITLEBAR
} awesome_type_t;

/** Type for Lua function */
typedef int luaA_function;

#define DO_LUA_NEW(decl, type, prefix, lua_type, type_ref) \
    decl int \
    luaA_##prefix##_userdata_new(lua_State *L, type *p) \
    { \
        type **pp = lua_newuserdata(L, sizeof(type *)); \
        *pp = p; \
        type_ref(pp); \
        return luaA_settype(L, lua_type); \
    }

#define DO_LUA_GC(type, prefix, lua_type, type_unref) \
    static int \
    luaA_##prefix##_gc(lua_State *L) \
    { \
        type **p = luaA_checkudata(L, 1, lua_type); \
        type_unref(p); \
        *p = NULL; \
        return 0; \
    }

#define DO_LUA_EQ(type, prefix, lua_type) \
    static int \
    luaA_##prefix##_eq(lua_State *L) \
    { \
        type **p1 = luaA_checkudata(L, 1, lua_type); \
        type **p2 = luaA_checkudata(L, 2, lua_type); \
        lua_pushboolean(L, (*p1 == *p2)); \
        return 1; \
    }

#define luaA_dostring(L, cmd) \
    do { \
        if(a_strlen(cmd)) \
            if(luaL_dostring(L, cmd)) \
                warn("error executing Lua code: %s", \
                     lua_tostring(L, -1)); \
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
            luaL_error(L, "invalid screen number: %d", screen); \
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

static inline bool
luaA_checkboolean(lua_State *L, int n)
{
    if(!lua_isboolean(L, n))
        luaL_typerror(L, n, "boolean");
    return lua_toboolean(L, n);
}

static inline bool
luaA_optboolean(lua_State *L, int idx, bool def)
{
    return luaL_opt(L, luaA_checkboolean, idx, def);
}

static inline lua_Number
luaA_getopt_number(lua_State *L, int idx, const char *name, lua_Number def)
{
    lua_getfield(L, idx, name);
    return luaL_optnumber(L, -1, def);
}

static inline const char *
luaA_getopt_lstring(lua_State *L, int idx, const char *name, const char *def, size_t *len)
{
    lua_getfield(L, idx, name);
    return luaL_optlstring(L, -1, def, len);
}

static inline const char *
luaA_getopt_string(lua_State *L, int idx, const char *name, const char *def)
{
    return luaA_getopt_lstring(L, idx, name, def, NULL);
}

static inline bool
luaA_getopt_boolean(lua_State *L, int idx, const char *name, bool def)
{
    lua_getfield(L, idx, name);
    return luaA_optboolean(L, -1, def);
}

static inline int
luaA_settype(lua_State *L, const char *type)
{
    luaL_getmetatable(L, type);
    lua_setmetatable(L, -2);
    return 1;
}

static inline int
luaA_usemetatable(lua_State *L, int idxobj, int idxfield)
{
    lua_getmetatable(L, idxobj);
    lua_pushvalue(L, idxfield);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1))
    {
        lua_remove(L, -2);
        return 1;
    }
    lua_pop(L, 2);

    return 0;
}

/** Register a function.
 * \param L The Lua stack.
 * \param fct A luaA_function address: it will be filled with the luaA_function
 * registered. If the adresse point to an already registered function, it will
 * be unregistered.
 * \return Always 0.
 */
static inline int
luaA_registerfct(lua_State *L, luaA_function *fct)
{
    luaA_checkfunction(L, -1);
    if(*fct != LUA_REFNIL)
        luaL_unref(L, LUA_REGISTRYINDEX, *fct);
    *fct = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Execute an Lua function.
 * \param L The Lua stack.
 * \param f The Lua function to execute.
 * \param nargs The number of arguments for the Lua function.
 * \param nret The number of returned value from the Lua function.
 * \return True on no error, false otherwise.
 */
static inline bool
luaA_dofunction(lua_State *L, luaA_function f, int nargs, int nret)
{
    if(f != LUA_REFNIL)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, f);
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
    return false;
}

int luaA_otable_index(lua_State *);

/** Create a new object table with a metatable.
 * This is useful to compare table with objects (udata) as keys.
 * \param L The Lua stack.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_otable_new(lua_State *L)
{
    /* Our object */
    lua_newtable(L);
    /* The meta table */
    lua_newtable(L);
    lua_pushcfunction(L, luaA_otable_index);
    /* Register index into the metatable */
    lua_setfield(L, -2, "__index");
    /* Set the meta table */
    lua_setmetatable(L, -2);
    return 1;
}

void luaA_init(void);
void luaA_parserc(const char *);
void luaA_pushpointer(lua_State *, void *, awesome_type_t);
void luaA_cs_init(void);
void luaA_cs_cleanup(void);
void luaA_on_timer(EV_P_ ev_timer *w, int revents);
void luaA_pushcolor(lua_State *, const xcolor_t *);

static inline int
luaA_generic_pairs(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
    lua_pushvalue(L, 1);  /* state, */
    lua_pushnil(L);  /* and initial value */
    return 3;
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
