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

struct lua_class_property
{
    /** ID matching the property */
    awesome_token_t id;
    /** Name of the property */
    const char *name;
    /** Callback function called when the property is found in object creation. */
    lua_class_propfunc_t new;
    /** Callback function called when the property is found in object __index. */
    lua_class_propfunc_t index;
    /** Callback function called when the property is found in object __newindex. */
    lua_class_propfunc_t newindex;
};

typedef struct
{
    int id;
    lua_class_t *class;
} lua_class_id_t;

DO_ARRAY(lua_class_id_t, lua_class_id, DO_NOTHING)

static lua_class_id_array_t luaA_classes;

/* This has to be initialized to the highest natural type of Lua */
#define LUA_HIGHEST_TYPE LUA_TTHREAD

/** Enhanced version of lua_type that recognizes setup Lua classes.
 * \param L The Lua VM state.
 * \param idx The index of the object on the stack.
 */
int
luaA_type(lua_State *L, int idx)
{
    int type = lua_type(L, idx);

    if(type == LUA_TUSERDATA)
        foreach(class, luaA_classes)
            if(luaA_toudata(L, idx, class->class->name))
                return class->id;

    return type;
}

/** Enhanced version of lua_typename that recognizes setup Lua classes.
 * \param L The Lua VM state.
 * \param idx The index of the object on the stack.
 */
const char *
luaA_typename(lua_State *L, int idx)
{
    int type = luaA_type(L, idx);

    if(type > LUA_HIGHEST_TYPE)
        foreach(class, luaA_classes)
            if(class->id == type)
                return class->class->name;

    return lua_typename(L, type);
}

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

static int
lua_class_property_cmp(const void *a, const void *b)
{
    const lua_class_property_t *x = a, *y = b;
    return x->id > y->id ? 1 : (x->id < y->id ? -1 : 0);
}

BARRAY_FUNCS(lua_class_property_t, lua_class_property, DO_NOTHING, lua_class_property_cmp)

void
luaA_class_add_property(lua_class_t *lua_class,
                        awesome_token_t token,
                        const char *name,
                        lua_class_propfunc_t cb_new,
                        lua_class_propfunc_t cb_index,
                        lua_class_propfunc_t cb_newindex)
{
    lua_class_property_array_insert(&lua_class->properties, (lua_class_property_t)
                                    {
                                        .id = token,
                                        .name = name,
                                        .new = cb_new,
                                        .index = cb_index,
                                        .newindex = cb_newindex
                                    });
}

void
luaA_class_setup(lua_State *L, lua_class_t *class,
                 const char *name,
                 lua_class_allocator_t allocator,
                 const struct luaL_reg methods[],
                 const struct luaL_reg meta[])
{
    static int class_type_counter = LUA_HIGHEST_TYPE;

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
