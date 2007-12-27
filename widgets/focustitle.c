/*
 * focustitle.c - focus title widget
 *
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
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

#include <stdio.h>
#include <confuse.h>
#include "util.h"
#include "widget.h"
#include "layout.h"
#include "focus.h"

extern AwesomeConf globalconf;

static int
focustitle_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Client *sel = globalconf.focus->client;

    widget->location = widget_calculate_offset(vscreen.statusbar->width,
                                               0,
                                               offset,
                                               widget->alignment);

    if(sel)
    {
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, vscreen.font, sel->name,
                  vscreen.colors_selected[ColFG],
                  vscreen.colors_selected[ColBG]);
        if(sel->isfloating)
            draw_circle(ctx, widget->location, 0,
                        (vscreen.font->height + 2) / 4,
                        sel->ismax,
                        vscreen.colors_selected[ColFG]);
    }
    else
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, vscreen.font, NULL,
                  vscreen.colors_normal[ColFG],
                  vscreen.colors_normal[ColBG]);

    widget->width = vscreen.statusbar->width - used;

    return widget->width;
}

Widget *
focustitle_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = focustitle_draw;
    w->alignment = AlignFlex;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
