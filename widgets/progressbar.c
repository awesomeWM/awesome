/*
 * progressbar.c - progress bar widget
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
#include "draw.h"
#include "widget.h"
#include "xutil.h"

extern AwesomeConf globalconf;

typedef struct
{
    int percent;
    int width;
    XColor fg;
    XColor bg;
} Data;

static int
progressbar_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int location, width, pwidth, margin, height;
    Data *d = widget->data;

    height = vscreen.statusbar->height / 2;
    margin = (vscreen.statusbar->height - height) / 2 - 1;

    width = d->width - (margin * 2);

    location = widget_calculate_offset(vscreen.statusbar->width,
                                       d->width,
                                       offset,
                                       widget->alignment) + margin;

    pwidth = d->percent ? (width * d->percent) / 100 : 0;

    draw_rectangle(ctx,
                   location + 1, margin,
                   width, height,
                   False, d->fg);

    if(pwidth > 0)
        draw_rectangle(ctx,
                       location + 1, margin + 1,
                       pwidth, height - 2,
                       True, d->fg);

    if(width - pwidth - 2 > 0)
        draw_rectangle(ctx,
                       location + pwidth + 2, margin + 1,
                       width - pwidth - 2, height - 2,
                       True, d->bg);

    return d->width;
}

static void
progressbar_tell(Widget *widget, char *command)
{
    Data *d = widget->data;
    int percent;
    
    if(!command)
        return;

    percent = atoi(command);

    if(percent <= 100 && percent >= 0)
        d->percent = percent;
}

Widget *
progressbar_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *color;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);
    d->width = cfg_getint(config, "width");
    d->percent = 0;

    if((color = cfg_getstr(config, "fg")))
        d->fg = initxcolor(statusbar->screen, color);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];
    
    if((color = cfg_getstr(config, "bg")))
        d->bg = initxcolor(statusbar->screen, color);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
