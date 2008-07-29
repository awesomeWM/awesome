/*
 * widget.h - widget managing header
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

#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include "structs.h"

#define WIDGET_CACHE_CLIENTS        (1<<0)
#define WIDGET_CACHE_TAGS           (1<<2)
#define WIDGET_CACHE_EMBEDDED       (1<<3)

void widget_invalidate_cache(int, int);
int widget_calculate_offset(int, int, int, int);
void widget_common_new(widget_t *);
void widget_render(widget_node_t *, draw_context_t *, xcb_gcontext_t, xcb_drawable_t, int, position_t, int, int, void *, awesome_type_t);

int luaA_widget_userdata_new(lua_State *, widget_t *);

void widget_invalidate_bywidget(widget_t *);

widget_constructor_t taglist_new;
widget_constructor_t textbox_new;
widget_constructor_t progressbar_new;
widget_constructor_t graph_new;
widget_constructor_t tasklist_new;
widget_constructor_t systray_new;
widget_constructor_t appicon_new;

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
