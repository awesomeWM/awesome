/*
 * wibox.h - wibox functions header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_WIBOX_H
#define AWESOME_WIBOX_H

#include "widget.h"
#include "strut.h"
#include "common/luaobject.h"

/** Wibox types */
typedef enum
{
    WIBOX_TYPE_NORMAL = 0,
    WIBOX_TYPE_TITLEBAR
} wibox_type_t;

/** Wibox type */
struct wibox_t
{
    LUA_OBJECT_HEADER
    /** Ontop */
    bool ontop;
    /** Visible */
    bool visible;
    /** Position */
    position_t position;
    /** Wibox type */
    wibox_type_t type;
    /** Alignment */
    alignment_t align;
    /** Screen */
    screen_t *screen;
    /** Widget list */
    widget_node_array_t widgets;
    void *widgets_table;
    /** Widget the mouse is over */
    widget_t *mouse_over;
    /** Need update */
    bool need_update;
    /** Need shape update */
    bool need_shape_update;
    /** Cursor */
    char *cursor;
    /** Background image */
    image_t *bg_image;
    /* Banned? used for titlebars */
    bool isbanned;
    /** Button bindings */
    button_array_t buttons;
    /** The window object. */
    xcb_window_t window;
    /** The pixmap copied to the window object. */
    xcb_pixmap_t pixmap;
    /** The graphic context. */
    xcb_gcontext_t gc;
    /** The window geometry. */
    area_t geometry;
    /** The window border width */
    uint16_t border_width;
    /** The window border color */
    xcolor_t border_color;
    /** Draw context */
    draw_context_t ctx;
    /** Orientation */
    orientation_t orientation;
    /** Opacity */
    double opacity;
    /** Strut */
    strut_t strut;
    /** The window's shape */
    struct
    {
        /** The window's content */
        image_t *clip;
        /** The window's content and border */
        image_t *bounding;
    } shape;
};

void wibox_unref_simplified(wibox_t **);

ARRAY_FUNCS(wibox_t *, wibox, wibox_unref_simplified)

void wibox_refresh(void);

void luaA_wibox_invalidate_byitem(lua_State *, const void *);

wibox_t * wibox_getbywin(xcb_window_t);

void wibox_moveresize(lua_State *, int, area_t);
void wibox_refresh_pixmap_partial(wibox_t *, int16_t, int16_t, uint16_t, uint16_t);
void wibox_init(wibox_t *, int);
void wibox_wipe(wibox_t *);
void wibox_set_opacity(lua_State *, int, double);
void wibox_set_orientation(lua_State *, int, orientation_t);

void wibox_class_setup(lua_State *);

lua_class_t wibox_class;

void wibox_clear_mouse_over(wibox_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
