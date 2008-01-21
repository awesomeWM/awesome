/*
 * focustitle.c - focus title widget
 *
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
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

#include <stdio.h>
#include "widget.h"
#include "layout.h"
#include "tag.h"
#include "focus.h"
#include "xutil.h"
#include "screen.h"
#include "common/util.h"

extern AwesomeConf globalconf;

typedef struct
{
    Alignment align;
    XColor fg;
    XColor bg;
} Data;

static int
focustitle_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    Data *d = widget->data;
    Client *sel = focus_get_current_client(widget->statusbar->screen);

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 0,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    widget->area.width = widget->statusbar->width - used;
    widget->area.height = widget->statusbar->height;

    if(sel)
    {
        draw_text(ctx, widget->area,
                  d->align,
                  widget->font->height / 2, widget->font, sel->name,
                  d->fg, d->bg);
        if(sel->isfloating || sel->ismax)
            draw_circle(ctx, widget->area.x, widget->area.y,
                        (widget->font->height + 2) / 4,
                        sel->ismax, d->fg);
    }
    else
        draw_rectangle(ctx, widget->area, True, d->bg);

    return widget->area.width;
}

Widget *
focustitle_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *buf;
    int phys_screen = get_phys_screen(statusbar->screen);

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = focustitle_draw;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        d->fg = initxcolor(globalconf.display, phys_screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_selected[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(globalconf.display, phys_screen, buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_selected[ColBG];

    d->align = draw_get_align(cfg_getstr(config, "align"));

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    /* Set cache property */
    w->cache.flags = WIDGET_CACHE_CLIENTS | WIDGET_CACHE_TAGS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
