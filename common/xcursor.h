/*
 * xcursor.h - X cursors management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_COMMON_XCURSORS_H
#define AWESOME_COMMON_XCURSORS_H

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#include "common/array.h"

typedef struct cursor_cache_entry_t cursor_cache_entry_t;
ARRAY_TYPE(cursor_cache_entry_t, cursors)

xcb_cursor_t xcursor_new(cursors_array_t *, xcb_cursor_context_t *, const char *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
