/*
 * widget.h - widget managing header
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
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

#define WIDGET_CACHE_CLIENTS        1<<0
#define WIDGET_CACHE_LAYOUTS        1<<1
#define WIDGET_CACHE_TAGS           1<<2
#define WIDGET_CACHE_ALL            (WIDGET_CACHE_CLIENTS | WIDGET_CACHE_LAYOUTS | WIDGET_CACHE_TAGS)

typedef widget_t *(WidgetConstructor)(alignment_t);

void widget_invalidate_cache(int, int);
int widget_calculate_offset(int, int, int, int);
void widget_common_new(widget_t *);
widget_t * widget_getbyname(const char *);
void widget_invalidate_statusbar_bywidget(widget_t *);
void widget_tell_managestatus(widget_t *, widget_tell_status_t, const char *);

WidgetConstructor taglist_new;
WidgetConstructor textbox_new;
WidgetConstructor iconbox_new;
WidgetConstructor progressbar_new;
WidgetConstructor graph_new;
WidgetConstructor tasklist_new;

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
