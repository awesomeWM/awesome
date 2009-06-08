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
