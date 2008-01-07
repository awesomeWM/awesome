/*
 * layoutinfo.c - layout info widget
 *
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Aldo Cortesi <aldo@nullcube.com>
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

#include "widget.h"
#include "util.h"
#include "tag.h"

extern AwesomeConf globalconf;

static int
layoutinfo_draw(Widget *widget,
                DrawCtx *ctx,
                int offset,
                int used __attribute__ ((unused)))
{
    Tag **curtags = get_current_tags(widget->statusbar->screen);
    Area area = draw_get_image_size(curtags[0]->layout->image);

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->statusbar->height,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    widget->area.width = ((double) widget->statusbar->height / (double) area.height) * area.width;;
    widget->area.height = widget->statusbar->height;

    draw_image(ctx, widget->area.x, widget->area.y,
               widget->statusbar->height,
               curtags[0]->layout->image);

    p_delete(&curtags);

    return widget->area.width;
}

Widget *
layoutinfo_new(Statusbar *statusbar, cfg_t* config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = layoutinfo_draw;

    /* Set cache property */
    w->cache.flags = WIDGET_CACHE_LAYOUTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
