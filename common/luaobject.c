/*
 * luaobject.c - useful functions for handling Lua objects
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

/** Handling of signals.
 *
 * This can not be used as a standalone class, but is instead referenced
 * explicitely in the classes, where it can be used. In the respective classes,
 * it then can be used via `classname:connect_signal(...)` etc.
 * @classmod signals
 */

#include "common/luaobject.h"
#include "common/backtrace.h"

/** Setup the object system at startup.
 * \param L The Lua VM state.
 */
void
luaA_object_setup(lua_State *L)
{
    /* Push identification string */
    lua_pushliteral(L, LUAA_OBJECT_REGISTRY_KEY);
    /* Create an empty table */
    lua_newtable(L);
    /* Create an empty metatable */
    lua_newtable(L);
    /* Set this empty table as the registry metatable.
     * It's used to store the number of reference on stored objects. */
    lua_setmetatable(L, -2);
    /* Register table inside registry */
    lua_rawset(L, LUA_REGISTRYINDEX);
}

/** Increment a object reference in its store table.
 * \param L The Lua VM state.
 * \param tud The table index on the stack.
 * \param oud The object index on the stack.
 * \return A pointer to the object.
 */
void *
luaA_object_incref(lua_State *L, int tud, int oud)
{
    /* Get pointer value of the item */
    void *pointer = (void *) lua_topointer(L, oud);

    /* Not reference able. */
    if(!pointer)
    {
        lua_remove(L, oud);
        return NULL;
    }

    /* Push the pointer (key) */
    lua_pushlightuserdata(L, pointer);
    /* Push the data (value) */
    lua_pushvalue(L, oud < 0 ? oud - 1 : oud);
    /* table.lightudata = data */
    lua_rawset(L, tud < 0 ? tud - 2 : tud);

    /* refcount++ */

    /* Get the metatable */
    lua_getmetatable(L, tud);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, pointer);
    /* Get the number of references */
    lua_rawget(L, -2);
    /* Get the number of references and increment it */
    int count = lua_tointeger(L, -1) + 1;
    lua_pop(L, 1);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, pointer);
    /* Push count (value) */
    lua_pushinteger(L, count);
    /* Set metatable[pointer] = count */
    lua_rawset(L, -3);
    /* Pop metatable */
    lua_pop(L, 1);

    /* Remove referenced item */
    lua_remove(L, oud);

    return pointer;
}

/** Decrement a object reference in its store table.
 * \param L The Lua VM state.
 * \param tud The table index on the stack.
 * \param oud The object index on the stack.
 * \return A pointer to the object.
 */
void
luaA_object_decref(lua_State *L, int tud, const void *pointer)
{
    if(!pointer)
        return;

    /* First, refcount-- */
    /* Get the metatable */
    lua_getmetatable(L, tud);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, (void *) pointer);
    /* Get the number of references */
    lua_rawget(L, -2);
    /* Get the number of references and decrement it */
    int count = lua_tointeger(L, -1) - 1;
    /* Did we find the item in our table? (tointeger(nil)-1) is -1 */
    if (count < 0)
    {
        buffer_t buf;
        backtrace_get(&buf);
        warn("BUG: Reference not found: %d %p\n%s", tud, pointer, buf.s);

        /* Pop reference count and metatable */
        lua_pop(L, 2);
        return;
    }
    lua_pop(L, 1);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, (void *) pointer);
    /* Hasn't the ref reached 0? */
    if(count)
        lua_pushinteger(L, count);
    else
        /* Yup, delete it, set nil as value */
        lua_pushnil(L);
    /* Set meta[pointer] = count/nil */
    lua_rawset(L, -3);
    /* Pop metatable */
    lua_pop(L, 1);

    /* Wait, no more ref? */
    if(!count)
    {
        /* Yes? So remove it from table */
        lua_pushlightuserdata(L, (void *) pointer);
        /* Push nil as value */
        lua_pushnil(L);
        /* table[pointer] = nil */
        lua_rawset(L, tud < 0 ? tud - 2 : tud);
    }
}

int
luaA_settype(lua_State *L, lua_class_t *lua_class)
{
    lua_pushlightuserdata(L, lua_class);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_setmetatable(L, -2);
    return 1;
}

/** Add a signal.
 * @tparam string name A signal name.
 * @tparam func func A function to call when the signal is emitted.
 * @function connect_signal
 */
void
luaA_object_connect_signal(lua_State *L, int oud,
                           const char *name, lua_CFunction fn)
{
    lua_pushcfunction(L, fn);
    luaA_object_connect_signal_from_stack(L, oud, name, -1);
}

/** Remove a signal.
 * @tparam string name A signal name.
 * @tparam func func A function to remove.
 * @function disconnect_signal
 */
void
luaA_object_disconnect_signal(lua_State *L, int oud,
                              const char *name, lua_CFunction fn)
{
    lua_pushcfunction(L, fn);
    luaA_object_disconnect_signal_from_stack(L, oud, name, -1);
}

/** Add a signal to an object.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 * \param name The name of the signal.
 * \param ud The index of function to call when signal is emitted.
 */
void
luaA_object_connect_signal_from_stack(lua_State *L, int oud,
                                      const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    lua_object_t *obj = lua_touserdata(L, oud);
    signal_connect(&obj->signals, name, luaA_object_ref_item(L, oud, ud));
}

/** Remove a signal to an object.
 * \param L The Lua VM state.
 * \param oud The object index on the stack.
 * \param name The name of the signal.
 * \param ud The index of function to call when signal is emitted.
 */
void
luaA_object_disconnect_signal_from_stack(lua_State *L, int oud,
                                         const char *name, int ud)
{
    luaA_checkfunction(L, ud);
    lua_object_t *obj = lua_touserdata(L, oud);
    void *ref = (void *) lua_topointer(L, ud);
    if (signal_disconnect(&obj->signals, name, ref))
        luaA_object_unref_item(L, oud, ref);
    lua_remove(L, ud);
}

void
signal_object_emit(lua_State *L, signal_array_t *arr, const char *name, int nargs)
{
    signal_t *sigfound = signal_array_getbyid(arr,
                                              a_strhash((const unsigned char *) name));

    if(sigfound)
    {
        int nbfunc = sigfound->sigfuncs.len;
        luaL_checkstack(L, nbfunc + nargs + 1, "too much signal");
        /* Push all functions and then execute, because this list can change
         * while executing funcs. */
        foreach(func, sigfound->sigfuncs)
            luaA_object_push(L, *func);

        debug("Emitting class signal '%s' (%d funcs)", name, nbfunc);
        for(int i = 0; i < nbfunc; i++)
        {
            /* push all args */
            for(int j = 0; j < nargs; j++)
                lua_pushvalue(L, - nargs - nbfunc + i);
            /* push first function */
            lua_pushvalue(L, - nargs - nbfunc + i);
            /* remove this first function */
            lua_remove(L, - nargs - nbfunc - 1 + i);
            luaA_dofunction(L, nargs, 0);
        }
    }

    /* remove args */
    lua_pop(L, nargs);
}

/** Emit a signal.
 * @tparam string name A signal name.
 * @param[opt] ... Various arguments.
 * @function emit_signal
 */
void
luaA_object_emit_signal(lua_State *L, int oud,
                        const char *name, int nargs)
{
    int oud_abs = luaA_absindex(L, oud);
    lua_class_t *lua_class = luaA_class_get(L, oud);
    lua_object_t *obj = luaA_toudata(L, oud, lua_class);
    if(!obj) {
        luaA_warn(L, "Trying to emit signal '%s' on non-object", name);
        return;
    }
    else if(lua_class->checker && !lua_class->checker(obj)) {
        luaA_warn(L, "Trying to emit signal '%s' on invalid object", name);
        return;
    }
    signal_t *sigfound = signal_array_getbyid(&obj->signals,
                                              a_strhash((const unsigned char *) name));
    if(sigfound)
    {
        int nbfunc = sigfound->sigfuncs.len;
        luaL_checkstack(L, nbfunc + nargs + 2, "too much signal");

        /* if( name != "refresh" ) { */
            const char *objname = lua_tostring(L, oud_abs);
            debug("Emitting signal '%s' on %s (%d funcs)", name, objname, nbfunc);
            luaA_dumpstack(L);
        /* } */

        /* Push all functions and then execute, because this list can change
         * while executing funcs. */
        foreach(func, sigfound->sigfuncs)
            luaA_object_push_item(L, oud_abs, *func);

        /* lua_pushvalue(L, oud_abs); */
        /* lua_class = luaA_class_get(L, oud_abs); */
        /* luaA_object_tostring_idx(L, oud_abs); */
        /* const char *objname = lua_tostring(L, -1); */
        /* lua_pop(L, 1); */

        for(int i = 0; i < nbfunc; i++)
        {
            /* push object */
            lua_pushvalue(L, oud_abs);
            /* push all args */
            for(int j = 0; j < nargs; j++)
                lua_pushvalue(L, - nargs - nbfunc - 1 + i);
            /* push first function */
            lua_pushvalue(L, - nargs - nbfunc - 1 + i);
            /* remove this first function */
            lua_remove(L, - nargs - nbfunc - 2 + i);
            luaA_dofunction(L, nargs + 1, 0);
        }
    }

    /* Then emit signal on the class */
    lua_pushvalue(L, oud);
    lua_insert(L, - nargs - 1);
    luaA_class_emit_signal(L, luaA_class_get(L, - nargs - 1), name, nargs + 1);
}

int
luaA_object_connect_signal_simple(lua_State *L)
{
    luaA_object_connect_signal_from_stack(L, 1, luaL_checkstring(L, 2), 3);
    return 0;
}

int
luaA_object_disconnect_signal_simple(lua_State *L)
{
    luaA_object_disconnect_signal_from_stack(L, 1, luaL_checkstring(L, 2), 3);
    return 0;
}

int
luaA_object_emit_signal_simple(lua_State *L)
{
    luaA_object_emit_signal(L, 1, luaL_checkstring(L, 2), lua_gettop(L) - 2);
    return 0;
}

int
luaA_object_tostring(lua_State *L)
{
    lua_class_t *lua_class = luaA_class_get(L, 1);
    lua_object_t *object = luaA_checkudata(L, 1, lua_class);
    int offset = 0;

    for(; lua_class; lua_class = lua_class->parent)
    {
        if(offset)
        {
            lua_pushliteral(L, "/");
            lua_insert(L, -++offset);
        }
        lua_pushstring(L, NONULL(lua_class->name));
        lua_insert(L, -++offset);

        if (lua_class->tostring) {
            int k, n;

            lua_pushliteral(L, "(");
            n = 2 + lua_class->tostring(L, object);
            lua_pushliteral(L, ")");

            for (k = 0; k < n; k++)
                lua_insert(L, -offset);
            offset += n;
        }
    }

    lua_pushfstring(L, ": %p", object);

    lua_concat(L, offset + 1);

    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
