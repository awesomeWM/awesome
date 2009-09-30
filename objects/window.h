/*
 * window.h - window object header
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

#ifndef AWESOME_OBJECTS_WINDOW_H
#define AWESOME_OBJECTS_WINDOW_H

#include "common/luaclass.h"

#define WINDOW_OBJECT_HEADER \
    LUA_OBJECT_HEADER \
    /** The X window number */ \
    xcb_window_t window; \
    /** Opacity */ \
    double opacity;

/** Window structure */
typedef struct
{
    WINDOW_OBJECT_HEADER
} window_t;

lua_class_t window_class;

void window_class_setup(lua_State *);

void window_set_opacity(lua_State *, int, double);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
