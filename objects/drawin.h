/*
 * drawin.h - drawin functions header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2010 Uli Schlachter <psychon@znc.in>
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

#ifndef AWESOME_OBJECTS_DRAWIN_H
#define AWESOME_OBJECTS_DRAWIN_H

#include "objects/window.h"
#include "common/luaobject.h"
#include "objects/drawable.h"
#include "draw.h"

/** Drawin type */
struct drawin_t
{
    WINDOW_OBJECT_HEADER
    /** Ontop */
    bool ontop;
    /** Visible */
    bool visible;
    /** Cursor */
    char *cursor;
    /** The drawable for this drawin. */
    drawable_t *drawable;
    /** The window geometry. */
    area_t geometry;
};

void drawin_unref_simplified(drawin_t **);

ARRAY_FUNCS(drawin_t *, drawin, drawin_unref_simplified)

drawin_t * drawin_getbywin(xcb_window_t);

void drawin_refresh_pixmap_partial(drawin_t *, int16_t, int16_t, uint16_t, uint16_t);

void drawin_class_setup(lua_State *);

lua_class_t drawin_class;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
