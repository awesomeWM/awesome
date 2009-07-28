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

#include "common/luaobject.h"

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

    /* Not referencable. */
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
    int count = lua_tonumber(L, -1) + 1;
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
luaA_object_decref(lua_State *L, int tud, void *pointer)
{
    if(!pointer)
        return;

    /* First, refcount-- */
    /* Get the metatable */
    lua_getmetatable(L, tud);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, pointer);
    /* Get the number of references */
    lua_rawget(L, -2);
    /* Get the number of references and decrement it */
    int count = lua_tonumber(L, -1) - 1;
    lua_pop(L, 1);
    /* Push the pointer (key) */
    lua_pushlightuserdata(L, pointer);
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
        lua_pushlightuserdata(L, pointer);
        /* Push nil as value */
        lua_pushnil(L);
        /* table[pointer] = nil */
        lua_rawset(L, tud < 0 ? tud - 2 : tud);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
