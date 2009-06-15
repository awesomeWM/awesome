/*
 * luaclass.c - useful functions for handling Lua classes
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

#include "common/luaclass.h"
#include "common/luaobject.h"

typedef struct
{
    int id;
    lua_class_t *class;
} lua_class_id_t;

DO_ARRAY(lua_class_id_t, lua_class_id, DO_NOTHING)

static lua_class_id_array_t luaA_classes;

void
luaA_openlib(lua_State *L, const char *name,
             const struct luaL_reg methods[],
             const struct luaL_reg meta[])
{
    luaL_newmetatable(L, name);                                        /* 1 */
    lua_pushvalue(L, -1);           /* dup metatable                      2 */
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable      1 */

    luaL_register(L, NULL, meta);                                      /* 1 */
    luaL_register(L, name, methods);                                   /* 2 */
    lua_pushvalue(L, -1);           /* dup self as metatable              3 */
    lua_setmetatable(L, -2);        /* set self as metatable              2 */
    lua_pop(L, 2);
}

void
luaA_class_setup(lua_State *L, lua_class_t *class,
                 const char *name,
                 lua_class_allocator_t allocator,
                 const struct luaL_reg methods[],
                 const struct luaL_reg meta[])
{
    static int class_type_counter = LUA_TTHREAD + 1;

    luaA_openlib(L, name, methods, meta);

    class->allocator = allocator;
    class->name = name;

    lua_class_id_array_append(&luaA_classes, (lua_class_id_t)
                              {
                                  .id = ++class_type_counter,
                                  .class = class,
                              });
}

void
luaA_class_add_signal(lua_State *L, lua_class_t *lua_class,
                      const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    signal_add(&lua_class->signals, name, luaA_object_ref(L, ud));
}

void
luaA_class_remove_signal(lua_State *L, lua_class_t *lua_class,
                         const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    void *ref = (void *) lua_topointer(L, ud);
    signal_remove(&lua_class->signals, name, ref);
    luaA_object_unref(L, (void *) ref);
    lua_remove(L, ud);
}

void
luaA_class_emit_signal(lua_State *L, lua_class_t *lua_class,
                       const char *name, int nargs)
{
    signal_t *sigfound = signal_array_getbyid(&lua_class->signals,
                                              a_strhash((const unsigned char *) name));
    if(sigfound)
        foreach(ref, sigfound->sigfuncs)
        {
            for(int i = 0; i < nargs; i++)
                lua_pushvalue(L, - nargs);
            luaA_object_push(L, (void *) *ref);
            luaA_dofunction(L, nargs, 0);
        }
    lua_pop(L, nargs);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
