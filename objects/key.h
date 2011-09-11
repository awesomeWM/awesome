/*
 * key.h - Keybinding helpers
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

#ifndef AWESOME_OBJECTS_KEY_H
#define AWESOME_OBJECTS_KEY_H

#include "common/luaobject.h"

typedef struct keyb_t
{
    LUA_OBJECT_HEADER
    /** Key modifier */
    uint16_t modifiers;
    /** Keysym */
    xcb_keysym_t keysym;
    /** Keycode */
    xcb_keycode_t keycode;
} keyb_t;

lua_class_t key_class;
LUA_OBJECT_FUNCS(key_class, keyb_t, key)
DO_ARRAY(keyb_t *, key, DO_NOTHING)

void key_class_setup(lua_State *);

void luaA_key_array_set(lua_State *, int, int, key_array_t *);
int luaA_key_array_get(lua_State *, int, key_array_t *);

int luaA_pushmodifiers(lua_State *, uint16_t);
uint16_t luaA_tomodifiers(lua_State *L, int ud);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
