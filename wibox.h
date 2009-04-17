/*
 * wibox.h - wibox functions header
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

#ifndef AWESOME_STATUSBAR_H
#define AWESOME_STATUSBAR_H

#include "widget.h"
#include "swindow.h"

/** Wibox types */
typedef enum
{
    WIBOX_TYPE_NORMAL = 0,
    WIBOX_TYPE_TITLEBAR
} wibox_type_t;

/** Wibox type */
struct wibox_t
{
    /** Lua references */
    luaA_ref_array_t refs;
    /** Ontop */
    bool ontop;
    /** Visible */
    bool isvisible;
    /** Position */
    position_t position;
    /** Wibox type */
    wibox_type_t type;
    /** Window */
    simple_window_t sw;
    /** Alignment */
    alignment_t align;
    /** Screen */
    screen_t *screen;
    /** Widget list */
    widget_node_array_t widgets;
    luaA_ref widgets_table;
    /** Widget the mouse is over */
    widget_t *mouse_over;
    /** Mouse over event handler */
    luaA_ref mouse_enter, mouse_leave;
    /** Need update */
    bool need_update;
    /** Cursor */
    char *cursor;
    /** Background image */
    image_t *bg_image;
    /* Banned? used for titlebars */
    bool isbanned;
    /** Button bindings */
    button_array_t buttons;
};

void wibox_unref_simplified(wibox_t **);

DO_ARRAY(wibox_t *, wibox, wibox_unref_simplified)

void wibox_refresh(void);
void wibox_update_positions(void);

void luaA_wibox_invalidate_byitem(lua_State *, const void *);

void wibox_position_update(wibox_t *);
wibox_t * wibox_getbywin(xcb_window_t);
void wibox_detach(wibox_t *);

static inline void
wibox_moveresize(wibox_t *wibox, area_t geometry)
{
    if(wibox->sw.window && !wibox->isbanned)
        simplewindow_moveresize(&wibox->sw, geometry);
    else if(wibox->sw.window && wibox->isbanned)
    {
        area_t real_geom = geometry;
        geometry.x = -geometry.width;
        geometry.y = -geometry.height;
        simplewindow_moveresize(&wibox->sw, geometry);
        wibox->sw.geometry = real_geom;
    }
    else
        wibox->sw.geometry = geometry;
    wibox->need_update = true;
}

LUA_OBJECT_FUNCS(wibox_t, wibox, "wibox")

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
