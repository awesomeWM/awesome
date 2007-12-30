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
#include "tag.h"
#include "focus.h"
#include "xutil.h"

extern AwesomeConf globalconf;

typedef struct
{
    XColor fg;
    XColor bg;
} Data;

static int
focustitle_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Data *d = widget->data;
    Client *sel = focus_get_current_client(widget->statusbar->screen);

    widget->location = widget_calculate_offset(vscreen.statusbar->width,
                                               0,
                                               offset,
                                               widget->alignment);

    if(sel)
    {
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, widget->font->height / 2, widget->font, sel->name,
                  d->fg, d->bg);
        if(sel->isfloating)
            draw_circle(ctx, widget->location, 0,
                        (widget->font->height + 2) / 4,
                        sel->ismax, d->fg);
    }
    else
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, widget->font->height / 2, widget->font, NULL,
                  d->fg, d->bg);

    widget->width = vscreen.statusbar->width - used;

    return widget->width;
}

Widget *
focustitle_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *buf;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = focustitle_draw;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        d->fg = initxcolor(statusbar->screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_selected[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(statusbar->screen, buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_selected[ColBG];

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
