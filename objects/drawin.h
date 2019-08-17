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
#include "objects/drawable.h"

struct drawin_t;

struct drawin_impl
{
    xcb_window_t (*get_xcb_window)(struct drawin_t *drawin);
    drawin_t *(*get_drawin_by_window)(xcb_window_t win);

    /* Drawin private functions */
    void (*drawin_allocate)(struct drawin_t *drawin);
    void (*drawin_cleanup)(struct drawin_t *drawin);
    void (*drawin_moveresize)(struct drawin_t *drawin);
    void (*drawin_refresh)(struct drawin_t *drawin, int16_t x, int16_t y,
            uint16_t w, uint16_t h);
    void (*drawin_map)(struct drawin_t *drawin);
    void (*drawin_unmap)(struct drawin_t *drawin);
    double (*drawin_get_opacity)(struct drawin_t *drawin);
    void (*drawin_set_cursor)(struct drawin_t *drawin, const char *cursor);
    void (*drawin_systray_kickout)(struct drawin_t *drawin);

    cairo_surface_t *(*drawin_get_shape_bounding)(struct drawin_t *drawin);
    cairo_surface_t *(*drawin_get_shape_clip)(struct drawin_t *drawin);
    cairo_surface_t *(*drawin_get_shape_input)(struct drawin_t *drawin);
    void (*drawin_set_shape_bounding)(struct drawin_t *drawin,
            cairo_surface_t *surface);
    void (*drawin_set_shape_clip)(struct drawin_t *drawin,
            cairo_surface_t *surface);
    void (*drawin_set_shape_input)(struct drawin_t *drawin,
            cairo_surface_t *surface);
};

/** Drawin type */
struct drawin_t
{
    LUA_OBJECT_HEADER
    /* XXX This data should only be cast from drawable_impl functions */
    void *impl_data;
    /** Opacity */
    double opacity;
    /** Strut */
    strut_t strut;
    /** Button bindings */
    button_array_t buttons;
    /** Do we have pending border changes? */
    bool border_need_update;
    /** Border color */
    color_t border_color;
    /** Border width */
    uint16_t border_width;
    /** The window type */
    window_type_t type;
    /** The border width callback */
    void (*border_width_callback)(void *, uint16_t old, uint16_t new);
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
    /** Do we have a pending geometry change that still needs to be applied? */
    bool geometry_dirty;
};

ARRAY_FUNCS(drawin_t *, drawin, DO_NOTHING)

void drawin_apply_moveresize(drawin_t *w);
void luaA_drawin_systray_kickout(lua_State *);

void drawin_class_setup(lua_State *);

lua_class_t drawin_class;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
