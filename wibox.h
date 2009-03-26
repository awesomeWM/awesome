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

void wibox_refresh(void);
void wibox_update_positions(void);

int luaA_wibox_new(lua_State *);
int luaA_wibox_userdata_new(lua_State *, wibox_t *);
void luaA_wibox_invalidate_byitem(lua_State *, const void *);

void wibox_position_update(wibox_t *);
wibox_t * wibox_getbywin(xcb_window_t);
void wibox_detach(wibox_t *);
void wibox_attach(wibox_t *, screen_t *);
void wibox_delete(wibox_t **);

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

DO_RCNT(wibox_t, wibox, wibox_delete)
ARRAY_FUNCS(wibox_t *, wibox, wibox_unref)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
