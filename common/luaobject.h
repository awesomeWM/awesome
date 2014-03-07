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

#include "common/luaclass.h"
#include "luaa.h"

#define LUAA_OBJECT_REGISTRY_KEY "awesome.object.registry"

int luaA_settype(lua_State *, lua_class_t *);
void luaA_object_setup(lua_State *);
void * luaA_object_incref(lua_State *, int, int);
void luaA_object_decref(lua_State *, int, const void *);

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
    luaA_getuservalue(L, ud);
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
    luaA_getuservalue(L, ud);
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
luaA_object_push_item(lua_State *L, int ud, const void *pointer)
{
    /* Get env table of the object */
    luaA_getuservalue(L, ud);
    /* Push key */
    lua_pushlightuserdata(L, (void *) pointer);
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
 * \return The object reference, or NULL if not referenceable.
 */
static inline void *
luaA_object_ref(lua_State *L, int oud)
{
    luaA_object_registry_push(L);
    void *p = luaA_object_incref(L, -1, oud < 0 ? oud - 1 : oud);
    lua_pop(L, 1);
    return p;
}

/** Reference an object and return a pointer to it checking its type.
 * That only works with userdata.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 * \param class The class of object expected
 * \return The object reference, or NULL if not referenceable.
 */
static inline void *
luaA_object_ref_class(lua_State *L, int oud, lua_class_t *class)
{
    luaA_checkudata(L, oud, class);
    return luaA_object_ref(L, oud);
}

/** Unreference an object and return a pointer to it.
 * That only works with userdata, table, thread or function.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 */
static inline void
luaA_object_unref(lua_State *L, const void *pointer)
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
luaA_object_push(lua_State *L, const void *pointer)
{
    luaA_object_registry_push(L);
    lua_pushlightuserdata(L, (void *) pointer);
    lua_rawget(L, -2);
    lua_remove(L, -2);
    return 1;
}

void signal_object_emit(lua_State *, signal_array_t *, const char *, int);

void luaA_object_connect_signal(lua_State *, int, const char *, lua_CFunction);
void luaA_object_disconnect_signal(lua_State *, int, const char *, lua_CFunction);
void luaA_object_connect_signal_from_stack(lua_State *, int, const char *, int);
void luaA_object_disconnect_signal_from_stack(lua_State *, int, const char *, int);
void luaA_object_emit_signal(lua_State *, int, const char *, int);

int luaA_object_connect_signal_simple(lua_State *);
int luaA_object_disconnect_signal_simple(lua_State *);
int luaA_object_emit_signal_simple(lua_State *);

#define LUA_OBJECT_FUNCS(lua_class, type, prefix)                              \
    LUA_CLASS_FUNCS(prefix, lua_class)                                         \
    static inline type *                                                       \
    prefix##_new(lua_State *L)                                                 \
    {                                                                          \
        type *p = lua_newuserdata(L, sizeof(type));                            \
        p_clear(p, 1);                                                         \
        (lua_class).instances++;                                               \
        luaA_settype(L, &(lua_class));                                         \
        lua_newtable(L);                                                       \
        lua_newtable(L);                                                       \
        lua_setmetatable(L, -2);                                               \
        luaA_setuservalue(L, -2);                                              \
        lua_pushvalue(L, -1);                                                  \
        p->signals.inherits_from = &(lua_class).signals;                       \
        luaA_class_emit_signal(L, &(lua_class), "new", 1);                     \
        return p;                                                              \
    }

#define OBJECT_EXPORT_PROPERTY(pfx, type, field) \
    fieldtypeof(type, field) \
    pfx##_get_##field(type *object) \
    { \
        return object->field; \
    }

#define LUA_OBJECT_EXPORT_PROPERTY(pfx, type, field, pusher) \
    static int \
    luaA_##pfx##_get_##field(lua_State *L, type *object) \
    { \
        pusher(L, object->field); \
        return 1; \
    }

int luaA_object_tostring(lua_State *);

#define LUA_OBJECT_META(prefix) \
    { "__tostring", luaA_object_tostring }, \
    { "connect_signal", luaA_object_connect_signal_simple }, \
    { "disconnect_signal", luaA_object_disconnect_signal_simple }, \
    { "emit_signal", luaA_object_emit_signal_simple },

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
