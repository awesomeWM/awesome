/*
 * iconbox.c - icon widget
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include "util.h"
#include "widget.h"

extern AwesomeConf globalconf;

static int
iconbox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int location, width;

    width = draw_get_image_width(widget->data);

    location = calculate_offset(vscreen.statusbar->width,
                                width,
                                offset,
                                widget->alignment);

    draw_image(ctx, location, 0, widget->data);

    return width;
}

static void
iconbox_tell(Widget *widget, char *command)
{
    if(widget->data)
        p_delete(&widget->data);
    widget->data = a_strdup(command);
    return;
}

Widget *
iconbox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;

    w = p_new(Widget, 1);
    common_new(w, statusbar, config);
    w->draw = iconbox_draw;
    w->tell = iconbox_tell;
    w->data = (void *) a_strdup(cfg_getstr(config, "image"));
    
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
