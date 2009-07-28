/*
 * luaobject.h - useful functions for handling Lua objects
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

#ifndef AWESOME_COMMON_LUAOBJECT
#define AWESOME_COMMON_LUAOBJECT

#include <lauxlib.h>
#include "common/array.h"

static inline int
luaA_settype(lua_State *L, const char *type)
{
    luaL_getmetatable(L, type);
    lua_setmetatable(L, -2);
    return 1;
}

void * luaA_object_incref(lua_State *, int, int);
void luaA_object_decref(lua_State *, int, void *);

/** Store an item in the environment table of an object.
 * \param L The Lua VM state.
 * \param ud The index of the object on the stack.
 * \param iud The index of the item on the stack.
 * \return The item reference.
 */
static inline void *
luaA_object_ref_item(lua_State *L, int ud, int iud)
{
    /* Get the env table from the object */
    lua_getfenv(L, ud);
    void *pointer = luaA_object_incref(L, -1, iud < 0 ? iud - 1 : iud);
    /* Remove env table */
    lua_pop(L, 1);
    return pointer;
}

/** Unref an item from the environment table of an object.
 * \param L The Lua VM state.
 * \param ud The index of the object on the stack.
 * \param ref item.
 */
static inline void
luaA_object_unref_item(lua_State *L, int ud, void *pointer)
{
    /* Get the env table from the object */
    lua_getfenv(L, ud);
    /* Decrement */
    luaA_object_decref(L, -1, pointer);
    /* Remove env table */
    lua_pop(L, 1);
}

/** Push an object item on the stack.
 * \param L The Lua VM state.
 * \param ud The object index on the stack.
 * \param pointer The item pointer.
 * \return The number of element pushed on stack.
 */
static inline int
luaA_object_push_item(lua_State *L, int ud, void *pointer)
{
    /* Get env table of the object */
    lua_getfenv(L, ud);
    /* Push key */
    lua_pushlightuserdata(L, pointer);
    /* Get env.pointer */
    lua_rawget(L, -2);
    /* Remove env table */
    lua_remove(L, -2);
    return 1;
}

DO_ARRAY(int, int, DO_NOTHING)

#define LUA_OBJECT_HEADER \
        int_array_t refs;

/** Generic type for all objects.
 * All Lua objects can be casted to this type.
 */
typedef struct
{
    LUA_OBJECT_HEADER
} lua_object_t;

#define LUA_OBJECT_FUNCS(type, prefix, lua_type)                               \
    static inline type *                                                       \
    prefix##_new(lua_State *L)                                                 \
    {                                                                          \
        type *p = lua_newuserdata(L, sizeof(type));                            \
        p_clear(p, 1);                                                         \
        luaA_settype(L, lua_type);                                             \
        lua_newtable(L);                                                       \
        lua_newtable(L);                                                       \
        lua_setmetatable(L, -2);                                               \
        lua_setfenv(L, -2);                                                    \
        return p;                                                              \
    }                                                                          \
                                                                               \
    static inline int                                                          \
    prefix##_push(lua_State *L, type *item)                                    \
    {                                                                          \
        if(item)                                                               \
        {                                                                      \
            assert(item->refs.len);                                            \
            lua_rawgeti(L, LUA_REGISTRYINDEX, item->refs.tab[0]);              \
        }                                                                      \
        else                                                                   \
            lua_pushnil(L);                                                    \
        return 1;                                                              \
    }                                                                          \
                                                                               \
    static inline type *                                                       \
    prefix##_ref(lua_State *L, int ud)                                         \
    {                                                                          \
        if(lua_isnil(L, ud))                                                   \
            return NULL;                                                       \
        type *item = luaL_checkudata(L, ud, lua_type);                         \
        lua_pushvalue(L, ud);                                                  \
        int_array_append(&item->refs, luaL_ref(L, LUA_REGISTRYINDEX));         \
        lua_remove(L, ud);                                                     \
        return item;                                                           \
    }                                                                          \
                                                                               \
    static inline void                                                         \
    prefix##_unref(lua_State *L, type *item)                                   \
    {                                                                          \
        if(item)                                                               \
        {                                                                      \
            assert(item->refs.len);                                            \
            luaL_unref(L, LUA_REGISTRYINDEX, item->refs.tab[0]);               \
            int_array_take(&item->refs, 0);                                    \
        }                                                                      \
    }                                                                          \
                                                                               \
    static inline int                                                          \
    prefix##_push_item(lua_State *L, type *item, void *ref)                    \
    {                                                                          \
        prefix##_push(L, item);                                                \
        luaA_object_push_item(L, -1, ref);                                     \
        lua_remove(L, -2);                                                     \
        return 1;                                                              \
    }


/** Garbage collect a Lua object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_object_gc(lua_State *L)
{
    lua_object_t *item = lua_touserdata(L, 1);
    int_array_wipe(&item->refs);
    return 0;
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
