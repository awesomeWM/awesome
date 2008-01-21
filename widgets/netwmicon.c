/*
 * netwmicon.c - NET_WM_ICON widget
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

#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include "focus.h"
#include "tag.h"
#include "widget.h"
#include "rules.h"
#include "ewmh.h"
#include "common/util.h"

extern AwesomeConf globalconf;

static int
netwmicon_draw(Widget *widget, DrawCtx *ctx, int offset,
                    int used __attribute__ ((unused)))
{
    Area area;
    Rule* r;
    Client *sel = focus_get_current_client(widget->statusbar->screen);
    NetWMIcon *icon;

    if(!sel)
    {
        widget->area.width = 0;
        return 0;
    }

    widget->area.height = widget->statusbar->height;

    if((r = rule_matching_client(sel)) && r->icon)
    {
        area = draw_get_image_size(r->icon);
        widget->area.width = ((double) widget->statusbar->height / (double) area.height)
            * area.width;
        if(!widget->user_supplied_x)
            widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                     widget->area.width,
                                                     offset,
                                                     widget->alignment);

        if(!widget->user_supplied_y)
            widget->area.y = 0;
        draw_image(ctx, widget->area.x, widget->area.y,
                   widget->statusbar->height, r->icon);

        return widget->area.width;
    }

    if(!(icon = ewmh_get_window_icon(sel->win)))
    {
        widget->area.width = 0;
        return 0;
    }

    widget->area.width = ((double) widget->statusbar->height / (double) icon->height) * icon->width;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_image_from_argb_data(ctx,
                              widget->area.x, widget->area.y,
                              icon->width, icon->height,
                              widget->statusbar->height, icon->image);

    p_delete(&icon->image);
    p_delete(&icon);

    return widget->area.width;
}

Widget *
netwmicon_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = netwmicon_draw;
    
    /* Set cache property */
    w->cache.flags = WIDGET_CACHE_CLIENTS;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
