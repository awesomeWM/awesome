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

#define LUAA_OBJECT_REGISTRY_KEY "awesome.object.registry"

void luaA_object_setup(lua_State *);
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

static inline void
luaA_object_registry_push(lua_State *L)
{
    lua_pushliteral(L, LUAA_OBJECT_REGISTRY_KEY);
    lua_rawget(L, LUA_REGISTRYINDEX);
}

/** Reference an object and return a pointer to it.
 * That only works with userdata, table, thread or function.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 * \return The object reference, or NULL if not referencable.
 */
static inline void *
luaA_object_ref(lua_State *L, int oud)
{
    luaA_object_registry_push(L);
    void *p = luaA_object_incref(L, -1, oud < 0 ? oud - 1 : oud);
    lua_pop(L, 1);
    return p;
}

/** Unreference an object and return a pointer to it.
 * That only works with userdata, table, thread or function.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 */
static inline void
luaA_object_unref(lua_State *L, void *pointer)
{
    luaA_object_registry_push(L);
    luaA_object_decref(L, -1, pointer);
    lua_pop(L, 1);
}

/** Push a referenced object onto the stack.
 * \param L The Lua VM state.
 * \param pointer The object to push.
 * \return The number of element pushed on stack.
 */
static inline int
luaA_object_push(lua_State *L, void *pointer)
{
    luaA_object_registry_push(L);
    lua_pushlightuserdata(L, pointer);
    lua_rawget(L, -2);
    lua_remove(L, -2);
    return 1;
}

#define LUA_OBJECT_HEADER

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
        return luaA_object_push(L, item);                                      \
    }                                                                          \
                                                                               \
    static inline type *                                                       \
    prefix##_ref(lua_State *L, int ud)                                         \
    {                                                                          \
        return luaA_object_ref(L, ud);                                         \
    }                                                                          \
                                                                               \
    static inline void                                                         \
    prefix##_unref(lua_State *L, type *item)                                   \
    {                                                                          \
        luaA_object_unref(L, item);                                            \
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
    return 0;
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
