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
#include "draw.h"
#include "common/tokenize.h"

typedef widget_t *(widget_constructor_t)(widget_t *);
typedef void (widget_destructor_t)(widget_t *);
typedef struct widget_node widget_node_t;

/** Widget */
struct widget_t
{
    LUA_OBJECT_HEADER
    /** Widget type is constructor */
    widget_constructor_t *type;
    /** Widget destructor */
    widget_destructor_t *destructor;
    /** Extents function */
    area_t (*extents)(lua_State *, widget_t *);
    /** Draw function */
    void (*draw)(widget_t *, draw_context_t *, area_t, wibox_t *);
    /** Index function */
    int (*index)(lua_State *, awesome_token_t);
    /** Newindex function */
    int (*newindex)(lua_State *, awesome_token_t);
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_array_t buttons;
    /** True if the widget is visible */
    bool isvisible;
};

struct widget_node
{
    /** The widget object */
    widget_t *widget;
    /** The geometry where the widget was drawn */
    area_t geometry;
};
DO_ARRAY(widget_node_t, widget_node, DO_NOTHING)

widget_t *widget_getbycoords(orientation_t, widget_node_array_t *, int, int, int16_t *, int16_t *);
void widget_render(wibox_t *);

void widget_invalidate_bywidget(widget_t *);
void widget_invalidate_bytype(widget_constructor_t *);

lua_class_t widget_class;
void widget_class_setup(lua_State *);

widget_constructor_t widget_textbox;
widget_constructor_t widget_systray;
widget_constructor_t widget_imagebox;

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
