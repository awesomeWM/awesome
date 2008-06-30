/*
 * keybinding.h - Keybinding helpers
 *
 * Copyright © 2008 Pierre Habouzit <madcoder@debian.org>
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

#ifndef AWESOME_KEYBINDING_H
#define AWESOME_KEYBINDING_H

#include <xcb/xcb.h>
#include "lua.h"
#include "common/refcount.h"
#include "common/array.h"

/** Keys bindings */
typedef struct keybinding_t keybinding_t;
ARRAY_TYPE(struct keybinding_t *, keybinding);

struct keybinding_t
{
    /** Ref count */
    int refcount;
    /** Key modifier */
    unsigned long mod;
    /** Keysym */
    xcb_keysym_t keysym;
    /** Keycode */
    xcb_keycode_t keycode;
    /** Lua function to execute. */
    luaA_function fct;
};

void keybinding_delete(keybinding_t **);
DO_RCNT(keybinding_t, keybinding, keybinding_delete)
ARRAY_FUNCS(keybinding_t *, keybinding, keybinding_unref)
keybinding_t *keybinding_find(const xcb_key_press_event_t *);

#endif
