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

#include <ev.h>

#include <lua.h>
#include <lauxlib.h>

#include <basedir.h>

#include <stdio.h>

#include "draw.h"
#include "common/util.h"
#include "common/luaobject.h"

#define luaA_deprecate(L, repl) \
    luaA_warn(L, "%s: This function is deprecated and will be removed, see %s", \
              __FUNCTION__, repl)

#define DO_LUA_TOSTRING(type, prefix, lua_type) \
    static int \
    luaA_##prefix##_tostring(lua_State *L) \
    { \
        type *p = luaL_checkudata(L, 1, lua_type); \
        lua_pushfstring(L, lua_type ": %p", p); \
        return 1; \
    }

#define luaA_dostring(L, cmd) \
    do { \
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
        if(screen < 0 || screen >= globalconf.screens.len) \
            luaL_error(L, "invalid screen number: %d", screen + 1); \
    } while(0)

/** Dump the Lua stack. Useful for debugging.
 * \param L The Lua VM state.
 */
static inline void
luaA_dumpstack(lua_State *L)
{
    fprintf(stderr, "-------- Lua stack dump ---------\n");
    for(int i = lua_gettop(L); i; i--)
    {
        int t = lua_type(L, i);
        switch (t)
        {
          case LUA_TSTRING:
            fprintf(stderr, "%d: string: `%s'\n", i, lua_tostring(L, i));
            break;
          case LUA_TBOOLEAN:
            fprintf(stderr, "%d: bool:   %s\n", i, lua_toboolean(L, i) ? "true" : "false");
            break;
          case LUA_TNUMBER:
            fprintf(stderr, "%d: number: %g\n", i, lua_tonumber(L, i));
            break;
          case LUA_TNIL:
            fprintf(stderr, "%d: nil\n", i);
            break;
          default:
            fprintf(stderr, "%d: %s\t#%d\t%p\n", i, lua_typename(L, t),
                    (int) lua_objlen(L, i),
                    lua_topointer(L, i));
            break;
        }
    }
    fprintf(stderr, "------- Lua stack dump end ------\n");
}

/** Convert a object to a udata if possible.
 * \param L The Lua VM state.
 * \param ud The index.
 * \param tname The type name.
 * \return A pointer to the object, NULL otherwise.
 */
static inline void *
luaA_toudata(lua_State *L, int ud, const char *tname)
{
    void *p = lua_touserdata(L, ud);
    if(p) /* value is a userdata? */
        if(lua_getmetatable(L, ud)) /* does it have a metatable? */
        {
            lua_getfield(L, LUA_REGISTRYINDEX, tname); /* get correct metatable */
            if(!lua_rawequal(L, -1, -2)) /* does it have the correct mt? */
                p = NULL;
            lua_pop(L, 2); /* remove both metatables */
        }
    return p;
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
    lua_Number n = luaL_optnumber(L, -1, def);
    lua_pop(L, 1);
    return n;
}

static inline const char *
luaA_getopt_lstring(lua_State *L, int idx, const char *name, const char *def, size_t *len)
{
    lua_getfield(L, idx, name);
    const char *s = luaL_optlstring(L, -1, def, len);
    lua_pop(L, 1);
    return s;
}

static inline bool
luaA_getopt_boolean(lua_State *L, int idx, const char *name, bool def)
{
    lua_getfield(L, idx, name);
    bool b = luaA_optboolean(L, -1, def);
    lua_pop(L, 1);
    return b;
}

/** Push a area type to a table on stack.
 * \param L The Lua VM state.
 * \param geometry The area geometry to push.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_pusharea(lua_State *L, area_t geometry)
{
    lua_newtable(L);
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

static inline int
luaA_usemetatable(lua_State *L, int idxobj, int idxfield)
{
    lua_getmetatable(L, idxobj);
    lua_pushvalue(L, idxfield);
    lua_rawget(L, -2);
    if(!lua_isnil(L, -1))
    {
        lua_remove(L, -2);
        return 1;
    }
    lua_pop(L, 2);

    return 0;
}

/** Register an Lua object.
 * \param L The Lua stack.
 * \param idx Index of the object in the stack.
 * \param ref A luaA_ref address: it will be filled with the luaA_ref
 * registered. If the adresse point to an already registered object, it will
 * be unregistered.
 * \return Always 0.
 */
static inline int
luaA_register(lua_State *L, int idx, luaA_ref *ref)
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
luaA_unregister(lua_State *L, luaA_ref *ref)
{
    luaL_unref(L, LUA_REGISTRYINDEX, *ref);
    *ref = LUA_REFNIL;
}

/** Register a function.
 * \param L The Lua stack.
 * \param idx Index of the function in the stack.
 * \param fct A luaA_ref address: it will be filled with the luaA_ref
 * registered. If the adresse point to an already registered function, it will
 * be unregistered.
 * \return luaA_register value.
 */
static inline int
luaA_registerfct(lua_State *L, int idx, luaA_ref *fct)
{
    luaA_checkfunction(L, idx);
    return luaA_register(L, idx, fct);
}

/** Execute an Lua function.
 * \param L The Lua stack.
 * \param f The Lua function to execute.
 * \param nargs The number of arguments for the Lua function.
 * \param nret The number of returned value from the Lua function.
 * \return True on no error, false otherwise.
 */
static inline bool
luaA_dofunction(lua_State *L, luaA_ref f, int nargs, int nret)
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

/** Get an optional padding table from a Lua table.
 * \param L The Lua VM state.
 * \param idx The table index on the stack.
 * \param dpadding The default padding value to use.
 */
static inline padding_t
luaA_getopt_padding(lua_State *L, int idx, padding_t *dpadding)
{
    padding_t padding;

    luaA_checktable(L, 2);

    padding.right = luaA_getopt_number(L, 2, "right", dpadding->right);
    padding.left = luaA_getopt_number(L, 2, "left", dpadding->left);
    padding.top = luaA_getopt_number(L, 2, "top", dpadding->top);
    padding.bottom = luaA_getopt_number(L, 2, "bottom", dpadding->bottom);

    return padding;
}


/** Push a padding structure into a table on the Lua stack.
 * \param L The Lua VM state.
 * \param padding The padding struct pointer.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_pushpadding(lua_State *L, padding_t *padding)
{
    lua_newtable(L);
    lua_pushnumber(L, padding->right);
    lua_setfield(L, -2, "right");
    lua_pushnumber(L, padding->left);
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, padding->top);
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, padding->bottom);
    lua_setfield(L, -2, "bottom");
    return 1;
}

void luaA_init(xdgHandle);
bool luaA_parserc(xdgHandle, const char *, bool);
void luaA_on_timer(EV_P_ ev_timer *, int);
int luaA_pushcolor(lua_State *, const xcolor_t *);
bool luaA_hasitem(lua_State *, const void *);
void luaA_table2wtable(lua_State *);
int luaA_next(lua_State *, int);
bool luaA_isloop(lua_State *, int);

#define hooks_property(c, prop) \
    do { \
        if(globalconf.hooks.property != LUA_REFNIL) \
        { \
            client_push(globalconf.L, c); \
            lua_pushliteral(globalconf.L, prop); \
            luaA_dofunction(globalconf.L, globalconf.hooks.property, 2, 0); \
        } \
    } while(0);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
