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

#ifndef AWESOME_OBJECTS_WIBOX_H
#define AWESOME_OBJECTS_WIBOX_H

#include "objects/widget.h"
#include "objects/window.h"
#include "common/luaobject.h"

/** Wibox type */
struct wibox_t
{
    WINDOW_OBJECT_HEADER
    /** Ontop */
    bool ontop;
    /** Visible */
    bool visible;
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
    /** The pixmap copied to the window object. */
    xcb_pixmap_t pixmap;
    /** The graphic context. */
    xcb_gcontext_t gc;
    /** The window geometry. */
    area_t geometry;
    /** Draw context */
    draw_context_t ctx;
    /** Orientation */
    orientation_t orientation;
    /** The window's shape */
    struct
    {
        /** The window's content */
        image_t *clip;
        /** The window's content and border */
        image_t *bounding;
    } shape;
    /** Has wibox an attached systray **/
    bool has_systray;
};

void wibox_unref_simplified(wibox_t **);

ARRAY_FUNCS(wibox_t *, wibox, wibox_unref_simplified)
void wibox_widget_node_array_wipe(lua_State *, int);

void wibox_refresh(void);

void luaA_wibox_invalidate_byitem(lua_State *, const void *);

wibox_t * wibox_getbywin(xcb_window_t);

void wibox_refresh_pixmap_partial(wibox_t *, int16_t, int16_t, uint16_t, uint16_t);

void wibox_class_setup(lua_State *);

lua_class_t wibox_class;

void wibox_clear_mouse_over(wibox_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
