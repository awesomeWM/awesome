/*
 * mouse.h - mouse managing header
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_MOUSE_H
#define AWESOME_MOUSE_H

#include "structs.h"

/** Mouse buttons bindings */
struct button_t
{
    /** Ref count */
    int refcount;
    /** Key modifiers */
    unsigned long mod;
    /** Mouse button number */
    unsigned int button;
    /** Lua function to execute on press. */
    luaA_ref press;
    /** Lua function to execute on release. */
    luaA_ref release;
};

void button_delete(button_t **);

DO_RCNT(button_t, button, button_delete)
ARRAY_FUNCS(button_t *, button, button_unref)

int luaA_client_mouse_resize(lua_State *);
int luaA_client_mouse_move(lua_State *);

int luaA_button_userdata_new(lua_State *, button_t *);

int luaA_button_array_get(lua_State *, button_array_t *);
void luaA_button_array_set(lua_State *, int idx, button_array_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
