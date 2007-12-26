/*
 * layoutinfo.c - layout info widget
 *
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include <confuse.h>
#include "widget.h"
#include "util.h"
#include "layout.h"

extern AwesomeConf globalconf;

static int
layoutinfo_draw(Widget *widget,
                DrawCtx *ctx,
                int offset,
                int used __attribute__ ((unused)))
{
    int location = widget_calculate_offset(widget->statusbar->width,
                                           widget->statusbar->height,
                                           offset,
                                           widget->alignment);

    draw_image(ctx, location, 0, widget->statusbar->height,
               get_current_layout(widget->statusbar->screen)->image);

    return widget->statusbar->height;
}

Widget *
layoutinfo_new(Statusbar *statusbar, cfg_t* config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = (void*) layoutinfo_draw;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
