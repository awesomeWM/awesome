/*
 * widget.h - widget managing header
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

#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include "button.h"
#include "common/tokenize.h"

typedef widget_t *(widget_constructor_t)(widget_t *);
typedef void (widget_destructor_t)(widget_t *);

/** Widget */
struct widget_t
{
    /** Lua references */
    luaA_ref_array_t refs;
    /** Widget type is constructor */
    widget_constructor_t *type;
    /** Widget destructor */
    widget_destructor_t *destructor;
    /** Geometry function */
    area_t (*geometry)(widget_t *, screen_t *, int, int);
    /** Draw function */
    void (*draw)(widget_t *, draw_context_t *, area_t, wibox_t *);
    /** Index function */
    int (*index)(lua_State *, awesome_token_t);
    /** Newindex function */
    int (*newindex)(lua_State *, awesome_token_t);
    /** Mouse over event handler */
    luaA_ref mouse_enter, mouse_leave;
    /** Alignement */
    alignment_t align;
    /** Supported alignment */
    alignment_t align_supported;
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_array_t buttons;
    /** True if the widget is visible */
    bool isvisible;
};

LUA_OBJECT_FUNCS(widget_t, widget, "widget");

struct widget_node_t
{
    /** The widget object */
    widget_t *widget;
    /** The geometry where the widget was drawn */
    area_t geometry;
};

widget_t *widget_getbycoords(orientation_t, widget_node_array_t *, int, int, int16_t *, int16_t *);
void widget_render(wibox_t *);

void luaA_table2widgets(lua_State *, widget_node_array_t *);

void widget_invalidate_bywidget(widget_t *);
void widget_invalidate_bytype(screen_t *, widget_constructor_t *);

widget_constructor_t widget_textbox;
widget_constructor_t widget_progressbar;
widget_constructor_t widget_graph;
widget_constructor_t widget_systray;
widget_constructor_t widget_imagebox;

void widget_node_delete(widget_node_t *);
ARRAY_FUNCS(widget_node_t, widget_node, widget_node_delete)

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
