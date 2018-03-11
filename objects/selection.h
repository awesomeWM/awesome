/*
 * selection.h - X11 selection interface
 *
 * Copyright © 2017 Uli Schlachter <psychon@znc.in>
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

#ifndef AWESOME_OBJECTS_SELECTION_H
#define AWESOME_OBJECTS_SELECTION_H

#include "common/luaobject.h"
#include <xcb/xcb.h>

typedef struct selection_t
{
    LUA_OBJECT_HEADER
    /** Atom identifying the selection we are watching */
    xcb_atom_t selection;
    /** Window used for transfers */
    xcb_window_t window;
    /** TODO */
    void *callback_function;
    /** TODO */
    bool incremental;
} selection_t;

void selection_class_setup(lua_State *);
bool selection_handle_notify(lua_State *, int, xcb_selection_notify_event_t *);
bool selection_handle_property(lua_State *, int);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
