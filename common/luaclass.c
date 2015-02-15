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
#include "luaa.h"

struct lua_class_property
{
    /** Name of the property */
    const char *name;
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
    if(p && lua_getmetatable(L, ud)) /* does it have a metatable? */
    {
        /* Get the lua_class_t that matches this metatable */
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_class_t *metatable_class = lua_touserdata(L, -1);

        /* remove lightuserdata (lua_class pointer) */
        lua_pop(L, 1);

        /* Now, check that the class given in argument is the same as the
         * metatable's object, or one of its parent (inheritance) */
        for(; metatable_class; metatable_class = metatable_class->parent)
            if(metatable_class == class)
                return p;
    }
    return NULL;
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
        luaA_typerror(L, ud, class->name);
    else if(class->checker && !class->checker(p))
        luaL_error(L, "invalid object");
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

    if(type == LUA_TUSERDATA && lua_getmetatable(L, idx))
    {
        /* Use the metatable has key to get the class from registry */
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_class_t *class = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return class;
    }

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
             const struct luaL_Reg methods[],
             const struct luaL_Reg meta[])
{
    luaL_newmetatable(L, name);                                        /* 1 */
    lua_pushvalue(L, -1);           /* dup metatable                      2 */
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable      1 */

    luaA_registerlib(L, NULL, meta);                                   /* 1 */
    luaA_registerlib(L, name, methods);                                /* 2 */
    lua_pushvalue(L, -1);           /* dup self as metatable              3 */
    lua_setmetatable(L, -2);        /* set self as metatable              2 */
    lua_pop(L, 2);
}

static int
lua_class_property_cmp(const void *a, const void *b)
{
    const lua_class_property_t *x = a, *y = b;
    return a_strcmp(x->name, y->name);
}

BARRAY_FUNCS(lua_class_property_t, lua_class_property, DO_NOTHING, lua_class_property_cmp)

void
luaA_class_add_property(lua_class_t *lua_class,
                        const char *name,
                        lua_class_propfunc_t cb_new,
                        lua_class_propfunc_t cb_index,
                        lua_class_propfunc_t cb_newindex)
{
    lua_class_property_array_insert(&lua_class->properties, (lua_class_property_t)
                                    {
                                        .name = name,
                                        .new = cb_new,
                                        .index = cb_index,
                                        .newindex = cb_newindex
                                    });
}

/** Garbage collect a Lua object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_class_gc(lua_State *L)
{
    lua_object_t *item = lua_touserdata(L, 1);
    signal_array_wipe(&item->signals);
    /* Get the object class */
    lua_class_t *class = luaA_class_get(L, 1);
    class->instances--;
    /* Call the collector function of the class, and all its parent classes */
    for(; class; class = class->parent)
        if(class->collector)
            class->collector(item);
    return 0;
}

/** Setup a new Lua class.
 * \param L The Lua VM state.
 * \param name The class name.
 * \param parent The parent class (inheritance).
 * \param allocator The allocator function used when creating a new object.
 * \param Collector The collector function used when garbage collecting an
 * object.
 * \param checker The check function to call when using luaA_checkudata().
 * \param index_miss_property Function to call when an object of this class
 * receive a __index request on an unknown property.
 * \param newindex_miss_property Function to call when an object of this class
 * receive a __newindex request on an unknown property.
 * \param methods The methods to set on the class table.
 * \param meta The meta-methods to set on the class objects.
 */
void
luaA_class_setup(lua_State *L, lua_class_t *class,
                 const char *name,
                 lua_class_t *parent,
                 lua_class_allocator_t allocator,
                 lua_class_collector_t collector,
                 lua_class_checker_t checker,
                 lua_class_propfunc_t index_miss_property,
                 lua_class_propfunc_t newindex_miss_property,
                 const struct luaL_Reg methods[],
                 const struct luaL_Reg meta[])
{
    /* Create the object metatable */
    lua_newtable(L);
    /* Register it with class pointer as key in the registry
     * class-pointer -> metatable */
    lua_pushlightuserdata(L, class);
    /* Duplicate the object metatable */
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
    /* Now register class pointer with metatable as key in the registry
     * metatable -> class-pointer */
    lua_pushvalue(L, -1);
    lua_pushlightuserdata(L, class);
    lua_rawset(L, LUA_REGISTRYINDEX);

    /* Duplicate objects metatable */
    lua_pushvalue(L, -1);
    /* Set garbage collector in the metatable */
    lua_pushcfunction(L, luaA_class_gc);
    lua_setfield(L, -2, "__gc");

    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable      1 */

    luaA_registerlib(L, NULL, meta);                                   /* 1 */
    luaA_registerlib(L, name, methods);                                /* 2 */
    lua_pushvalue(L, -1);           /* dup self as metatable              3 */
    lua_setmetatable(L, -2);        /* set self as metatable              2 */
    lua_pop(L, 2);

    class->collector = collector;
    class->allocator = allocator;
    class->name = name;
    class->index_miss_property = index_miss_property;
    class->newindex_miss_property = newindex_miss_property;
    class->checker = checker;
    class->parent = parent;
    class->instances = 0;

    signal_add(&class->signals, "new");

    if (parent)
        class->signals.inherits_from = &parent->signals;

    lua_class_array_append(&luaA_classes, class);
}

void
luaA_class_connect_signal(lua_State *L, lua_class_t *lua_class, const char *name, lua_CFunction fn)
{
    lua_pushcfunction(L, fn);
    luaA_class_connect_signal_from_stack(L, lua_class, name, -1);
}

void
luaA_class_connect_signal_from_stack(lua_State *L, lua_class_t *lua_class,
                                     const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    signal_connect(&lua_class->signals, name, luaA_object_ref(L, ud));
}

void
luaA_class_disconnect_signal_from_stack(lua_State *L, lua_class_t *lua_class,
                                        const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    void *ref = (void *) lua_topointer(L, ud);
    signal_disconnect(&lua_class->signals, name, ref);
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
    lua_class_t *class = luaA_class_get(L, idxobj);

    for(; class; class = class->parent)
    {
        /* Push the class */
        lua_pushlightuserdata(L, class);
        /* Get its metatable from registry */
        lua_rawget(L, LUA_REGISTRYINDEX);
        /* Push the field */
        lua_pushvalue(L, idxfield);
        /* Get the field in the metatable */
        lua_rawget(L, -2);
        /* Do we have a field like that? */
        if(!lua_isnil(L, -1))
        {
            /* Yes, so remove the metatable and return it! */
            lua_remove(L, -2);
            return 1;
        }
        /* No, so remove the metatable and its value */
        lua_pop(L, 2);
    }

    return 0;
}

static lua_class_property_t *
lua_class_property_array_getbyname(lua_class_property_array_t *arr,
                                   const char *name)
{
    lua_class_property_t lookup_prop = { .name = name };
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
    const char *attr = luaL_checkstring(L, fieldidx);

    /* Look for the property in the class; if not found, go in the parent class. */
    for(; lua_class; lua_class = lua_class->parent)
    {
        lua_class_property_t *prop =
            lua_class_property_array_getbyname(&lua_class->properties, attr);

        if(prop)
            return prop;
    }

    return NULL;
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

    /* Is this the special 'valid' property? This is the only property
     * accessible for invalid objects and thus needs special handling. */
    const char *attr = luaL_checkstring(L, 2);
    if (A_STREQ(attr, "valid"))
    {
        void *p = luaA_toudata(L, 1, class);
        if (class->checker)
            lua_pushboolean(L, p != NULL && class->checker(p));
        else
            lua_pushboolean(L, p != NULL);
        return 1;
    }

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
            lua_class_property_t *prop = luaA_class_property_get(L, lua_class, -2);

            if(prop && prop->new)
                prop->new(L, object);
        }
        /* Remove value */
        lua_pop(L, 1);
    }

    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
