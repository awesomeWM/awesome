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
                        lua_class_propfunc_t cb_new,
                        lua_class_propfunc_t cb_index,
                        lua_class_propfunc_t cb_newindex)
{
    lua_class_property_array_insert(&lua_class->properties, (lua_class_property_t)
                                    {
                                        .id = token,
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

/** Try to use the metatable of an object.
 * \param L The Lua VM state.
 * \param idxobj The index of the object.
 * \param idxfield The index of the field (attribute) to get.
 * \return The number of element pushed on stack.
 */
int
luaA_usemetatable(lua_State *L, int idxobj, int idxfield)
{
    /* Get metatable of the object. */
    lua_getmetatable(L, idxobj);
    /* Get the field */
    lua_pushvalue(L, idxfield);
    lua_rawget(L, -2);
    /* Do we have a field like that? */
    if(!lua_isnil(L, -1))
    {
        /* Yes, so return it! */
        lua_remove(L, -2);
        return 1;
    }
    /* No, so remove everything. */
    lua_pop(L, 2);

    return 0;
}

static lua_class_property_t *
lua_class_property_array_getbyid(lua_class_property_array_t *arr,
                                 awesome_token_t id)
{
    lua_class_property_t lookup_prop = { .id = id };
    return lua_class_property_array_lookup(arr, &lookup_prop);
}

/** Get the class of an object.
 * \param L The Lua VM state.
 * \param idx The index of the object on the stack.
 * \return The class if found, NULL otherwise.
 */
static lua_class_t *
luaA_class_get(lua_State *L, int idx)
{
    int type = luaA_type(L, 1);

    /* Find the class. */
    lua_class_t *class = NULL;

    foreach(classid, luaA_classes)
        if(classid->id == type)
        {
            class = classid->class;
            break;
        }

    return class;
}

/** Get a property of a object.
 * \param L The Lua VM state.
 * \param lua_class The Lua class.
 * \param fieldidx The index of the field name.
 * \return The object property if found, NULL otherwise.
 */
static lua_class_property_t *
luaA_class_property_get(lua_State *L, lua_class_t *lua_class, int fieldidx)
{
    /* Lookup the property using token */
    size_t len;
    const char *attr = luaL_checklstring(L, fieldidx, &len);
    awesome_token_t token = a_tokenize(attr, len);

    return lua_class_property_array_getbyid(&lua_class->properties, token);
}

/** Generic index meta function for objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_class_index(lua_State *L)
{
    /* Try to use metatable first. */
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    lua_class_t *class = luaA_class_get(L, 1);

    lua_class_property_t *prop = luaA_class_property_get(L, class, 2);

    /* Property does exist and has an index callback */
    if(prop && prop->index)
        return prop->index(L, luaL_checkudata(L, 1, class->name));

    return 0;
}

/** Generic newindex meta function for objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_class_newindex(lua_State *L)
{
    /* Try to use metatable first. */
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    lua_class_t *class = luaA_class_get(L, 1);

    lua_class_property_t *prop = luaA_class_property_get(L, class, 2);

    /* Property does exist and has a newindex callback */
    if(prop && prop->newindex)
        return prop->newindex(L, luaL_checkudata(L, 1, class->name));

    return 0;
}

/** Generic constructor function for objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_class_new(lua_State *L, lua_class_t *lua_class)
{
    /* Check we have a table that should contains some properties */
    luaA_checktable(L, 2);

    /* Create a new object */
    void *object = lua_class->allocator(L);

    /* Push the first key before iterating */
    lua_pushnil(L);
    /* Iterate over the property keys */
    while(lua_next(L, 2))
    {
        /* Check that the key is a string.
         * We cannot call tostring blindly or Lua will convert a key that is a
         * number TO A STRING, confusing lua_next() */
        if(lua_isstring(L, -2))
        {
            /* Lookup the property using token */
            size_t len;
            const char *attr = lua_tolstring(L, -2, &len);
            lua_class_property_t *prop =
                lua_class_property_array_getbyid(&lua_class->properties,
                                                 a_tokenize(attr, len));

            if(prop && prop->new)
                prop->new(L, object);
        }
        /* Remove value */
        lua_pop(L, 1);
    }

    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
