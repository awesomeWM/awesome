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

DO_ARRAY(lua_class_t *, lua_class, DO_NOTHING)

static lua_class_array_t luaA_classes;

/** Convert a object to a udata if possible.
 * \param L The Lua VM state.
 * \param ud The index.
 * \param class The wanted class.
 * \return A pointer to the object, NULL otherwise.
 */
void *
luaA_toudata(lua_State *L, int ud, lua_class_t *class)
{
    void *p = lua_touserdata(L, ud);
    if(p) /* value is a userdata? */
        if(lua_getmetatable(L, ud)) /* does it have a metatable? */
        {
            lua_pushlightuserdata(L, class);
            lua_rawget(L, LUA_REGISTRYINDEX);
            if(!lua_rawequal(L, -1, -2)) /* does it have the correct mt? */
                p = NULL;
            lua_pop(L, 2); /* remove both metatables */
        }
    return p;
}

/** Check for a udata class.
 * \param L The Lua VM state.
 * \param ud The object index on the stack.
 * \param class The wanted class.
 */
void *
luaA_checkudata(lua_State *L, int ud, lua_class_t *class)
{
    void *p = luaA_toudata(L, ud, class);
    if(!p)
        luaL_typerror(L, ud, class->name);
    return p;
}

/** Get an object lua_class.
 * \param L The Lua VM state.
 * \param idx The index of the object on the stack.
 */
lua_class_t *
luaA_class_get(lua_State *L, int idx)
{
    int type = lua_type(L, idx);

    if(type == LUA_TUSERDATA)
        foreach(class, luaA_classes)
            if(luaA_toudata(L, idx, *class))
                return *class;

    return NULL;
}

/** Enhanced version of lua_typename that recognizes setup Lua classes.
 * \param L The Lua VM state.
 * \param idx The index of the object on the stack.
 */
const char *
luaA_typename(lua_State *L, int idx)
{
    int type = lua_type(L, idx);

    if(type == LUA_TUSERDATA)
    {
        lua_class_t *lua_class = luaA_class_get(L, idx);
        if(lua_class)
            return lua_class->name;
    }

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
                 lua_class_propfunc_t index_miss_property,
                 lua_class_propfunc_t newindex_miss_property,
                 const struct luaL_reg methods[],
                 const struct luaL_reg meta[])
{
    /* Create the metatable */
    lua_newtable(L);
    /* Register it with class pointer as key in the registry */
    lua_pushlightuserdata(L, class);
    /* Duplicate the metatable */
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, -1);           /* dup metatable                      2 */
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable      1 */

    luaL_register(L, NULL, meta);                                      /* 1 */
    luaL_register(L, name, methods);                                   /* 2 */
    lua_pushvalue(L, -1);           /* dup self as metatable              3 */
    lua_setmetatable(L, -2);        /* set self as metatable              2 */
    lua_pop(L, 2);

    class->allocator = allocator;
    class->name = name;
    class->index_miss_property = index_miss_property;
    class->newindex_miss_property = newindex_miss_property;

    lua_class_array_append(&luaA_classes, class);
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
    signal_object_emit(L, &lua_class->signals, name, nargs);
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
    if(prop)
    {
        if(prop->index)
            return prop->index(L, luaA_checkudata(L, 1, class));
    }
    else
    {
        if(class->index_miss_property)
            return class->index_miss_property(L, luaA_checkudata(L, 1, class));
    }

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
    if(prop)
    {
        if(prop->newindex)
            return prop->newindex(L, luaA_checkudata(L, 1, class));
    }
    else
    {
        if(class->newindex_miss_property)
            return class->newindex_miss_property(L, luaA_checkudata(L, 1, class));
    }

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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
