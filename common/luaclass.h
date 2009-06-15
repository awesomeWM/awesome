/*
 * luaclass.h - useful functions for handling Lua classes
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

#ifndef AWESOME_COMMON_LUACLASS
#define AWESOME_COMMON_LUACLASS

#include "common/signal.h"

#define LUA_OBJECT_HEADER \
        signal_array_t signals;

/** Generic type for all objects.
 * All Lua objects can be casted to this type.
 */
typedef struct
{
    LUA_OBJECT_HEADER
} lua_object_t;

typedef lua_object_t *(*lua_class_allocator_t)(lua_State *);

typedef struct
{
    /** Class name */
    const char *name;
    signal_array_t signals;
    /** Allocator for creating new objects of that class */
    lua_class_allocator_t allocator;
} lua_class_t;

int luaA_type(lua_State *, int);
const char * luaA_typename(lua_State *, int);

void luaA_class_add_signal(lua_State *, lua_class_t *, const char *, int);
void luaA_class_remove_signal(lua_State *, lua_class_t *, const char *, int);
void luaA_class_emit_signal(lua_State *, lua_class_t *, const char *, int);

void luaA_openlib(lua_State *, const char *, const struct luaL_reg[], const struct luaL_reg[]);
void luaA_class_setup(lua_State *, lua_class_t *, const char *, lua_class_allocator_t,
                      const struct luaL_reg[], const struct luaL_reg[]);

#define LUA_CLASS_FUNCS(prefix, lua_class) \
    static inline int                                                          \
    luaA_##prefix##_class_add_signal(lua_State *L)                             \
    {                                                                          \
        luaA_class_add_signal(L, &(lua_class), luaL_checkstring(L, 1), 2);     \
        return 0;                                                              \
    }                                                                          \
                                                                               \
    static inline int                                                          \
    luaA_##prefix##_class_remove_signal(lua_State *L)                          \
    {                                                                          \
        luaA_class_remove_signal(L, &(lua_class),                              \
                                 luaL_checkstring(L, 1), 2);                   \
        return 0;                                                              \
    }                                                                          \
                                                                               \
    static inline int                                                          \
    luaA_##prefix##_class_emit_signal(lua_State *L)                            \
    {                                                                          \
        luaA_class_emit_signal(L, &(lua_class), luaL_checkstring(L, 1),        \
                              lua_gettop(L) - 1);                              \
        return 0;                                                              \
    }

#define LUA_CLASS_METHODS(class) \
    { "add_signal", luaA_##class##_class_add_signal }, \
    { "remove_signal", luaA_##class##_class_remove_signal }, \
    { "emit_signal", luaA_##class##_class_emit_signal },

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
