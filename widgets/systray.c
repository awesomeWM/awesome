/*
 * systray.c - systray widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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
#include "common/xembed.h"
#include "common/atoms.h"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

extern awesome_t globalconf;

static int
systray_draw(draw_context_t *ctx,
             int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used __attribute__ ((unused)),
             void *p,
             awesome_type_t type)
{
    uint32_t orient;
    /* p is always a statusbar, titlebars are forbidden */
    statusbar_t *sb = (statusbar_t *) p;

    /* we are on a statusbar */
    assert(type == AWESOME_TYPE_STATUSBAR);

    w->area.height = ctx->height;

    if(ctx->width - used > 0)
    {
        int i = 0;
        xembed_window_t *em;
        for(em = globalconf.embedded; em; em = em->next)
            if(em->phys_screen == sb->phys_screen)
                i++;
        /** \todo use clas hints */
        w->area.width = MIN(i * ctx->height, ctx->width - used);
    }
    else
        w->area.width = 0;

    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    switch(sb->position)
    {
      case Right:
      case Left:
        orient = _NET_SYSTEM_TRAY_ORIENTATION_VERT;
        break;
      default:
        orient = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
        break;
    }

    /* set statusbar orientation */
    /** \todo stop setting that property on each redraw */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.screens[sb->phys_screen].systray.window,
                        _NET_SYSTEM_TRAY_ORIENTATION, CARDINAL, 32, 1, &orient);

    return w->area.width;
}

widget_t *
systray_new(alignment_t align)
{
    widget_t *w;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = systray_draw;
    w->cache_flags = WIDGET_CACHE_EMBEDDED;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
