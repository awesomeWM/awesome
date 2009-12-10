/*
 * systray.c - systray widget
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "widget.h"
#include "screen.h"
#include "wibox.h"
#include "luaa.h"
#include "globalconf.h"
#include "common/xembed.h"
#include "common/atoms.h"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

typedef struct
{
    /* systray height */
    int height;
} systray_data_t;

static area_t
systray_extents(lua_State *L, widget_t *widget)
{
    int screen = luaL_optnumber(L, -1, 1) - 1;
    luaA_checkscreen(screen);

    area_t geometry;
    int phys_screen = screen_virttophys(screen), n = 0;
    systray_data_t *d = widget->data;

    for(int i = 0; i < globalconf.embedded.len; i++)
        if(globalconf.embedded.tab[i].phys_screen == phys_screen)
            n++;

    /** \todo use class hints */
    geometry.width  = d->height * n;
    geometry.height = d->height;
    geometry.x      = geometry.y = 0;

    return geometry;
}

static void
systray_draw(widget_t *widget, draw_context_t *ctx,
             area_t geometry, wibox_t *p)
{
    uint32_t orient;

    switch(p->position)
    {
      case Right:
      case Left:
        orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
        break;
      default:
        orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
        break;
    }

    systray_data_t *d = widget->data;
    d->height = p->geometry.height;

    /* set wibox orientation */
    /** \todo stop setting that property on each redraw */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.screens.tab[p->ctx.phys_screen].systray.window,
                        _NET_SYSTEM_TRAY_ORIENTATION, CARDINAL, 32, 1, &orient);
}

/** Delete a systray widget.
 * \param w The widget to destroy.
 */
static void
systray_destructor(widget_t *w)
{
    systray_data_t *d = w->data;
    p_delete(&d);
}

/** Initialize a systray widget.
 * \param w The widget to initialize.
 * \return The same widget.
 */
widget_t *
widget_systray(widget_t *w)
{
    w->draw = systray_draw;
    w->extents = systray_extents;
    w->destructor = systray_destructor;

    systray_data_t *d = w->data = p_new(systray_data_t, 1);
    d->height = 0;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
