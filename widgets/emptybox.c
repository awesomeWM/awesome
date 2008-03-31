/*
 * emptybox.c - empty widget
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

#include "widget.h"
#include "common/configopts.h"

extern AwesomeConf globalconf;

typedef struct
{
    style_t style;
} Data;

static int
emptybox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    Data *d = widget->data;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_rectangle(ctx, widget->area, 1.0, True, d->style.bg);

    return widget->area.width;
}

Widget *
emptybox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = emptybox_draw;

    w->data = d = p_new(Data, 1);

    draw_style_init(globalconf.display, statusbar->phys_screen,
                    cfg_getsec(config, "style"),
                    &d->style,
                    &globalconf.screens[statusbar->screen].styles.normal);

    w->area.width = cfg_getint(config, "width");
    w->area.height = statusbar->height;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
