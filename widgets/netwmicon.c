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
#include "util.h"
#include "focus.h"
#include "tag.h"
#include "widget.h"
#include "rules.h"
#include "ewmh.h"

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
        return 0;

    for(r = globalconf.rules; r; r = r->next)
        if(r->icon && client_match_rule(sel, r))
        {
            area = draw_get_image_size(r->icon);
            widget->width = ((double) widget->statusbar->height / (double) area.height) * area.width;
            widget->location = widget_calculate_offset(widget->statusbar->width,
                                                       widget->width,
                                                       offset,
                                                       widget->alignment);
            draw_image(ctx, widget->location, 0, widget->statusbar->height, r->icon);

            return widget->width;
        }


    if(!(icon = ewmh_get_window_icon(sel->win)))
        return 0;

    widget->width = ((double) widget->statusbar->height / (double) icon->height) * icon->width;

    widget->location = widget_calculate_offset(widget->statusbar->width,
                                               widget->width,
                                               offset,
                                               widget->alignment);

    draw_image_from_argb_data(ctx,
                              widget->location, 0,
                              icon->width, icon->height,
                              widget->statusbar->height, icon->image);
    p_delete(&icon);

    return widget->width;
}

Widget *
netwmicon_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = netwmicon_draw;
    
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
