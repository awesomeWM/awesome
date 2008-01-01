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
#include <string.h>
#include "util.h"
#include "draw.h"
#include "widget.h"
#include "xutil.h"

extern AwesomeConf globalconf;

typedef struct
{
    int *percent;   /* 0-100 */
    int width;      /* width of the bars */
    int lpadding;   /* padding on the left of the bars */
    int gap;        /* pixels between bars */
    int bars;       /* number of bars */
    float height;   /* height 0-1 (where 1 = height of statusbar */
    XColor *fg;
    XColor *bg;
    XColor *bcolor; /* border color */
} Data;

static int
progressbar_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    int i, width, pwidth, margin_top, pb_height, left_offset;

    Data *d = widget->data;

    if (!(d->bars))
        return 0;

    margin_top = (int) (widget->statusbar->height * (1 - d->height)) / 2 + 0.5;
    pb_height = (int) (widget->statusbar->height * d->height - (d->gap * (d->bars - 1))) / d->bars + 0.5; 

    width = d->width - d->lpadding;

    widget->location = widget_calculate_offset(widget->statusbar->width,
            d->width,
            offset,
            widget->alignment);

    left_offset = widget->location + d->lpadding;

    for (i = 0; i < d->bars; i++)
    {
        pwidth = (int) d->percent[i] ? ((width - 2) * d->percent[i]) / 100 : 0;

        draw_rectangle(ctx,
                left_offset, margin_top,
                width, pb_height,
                False, d->bcolor[i]);

        if(pwidth > 0)
            draw_rectangle(ctx,
                    left_offset + 1, margin_top + 1,
                    pwidth, pb_height - 2,
                    True, d->fg[i]);

        if(width - 2 - pwidth > 0) /* not filled area */
            draw_rectangle(ctx,
                    left_offset + 1 + pwidth, margin_top + 1,
                    width - 2 - pwidth, pb_height - 2,
                    True, d->bg[i]);

        margin_top += (pb_height + d->gap);
    }

    widget->width = d->width;
    return widget->width;
}

static void
progressbar_tell(Widget *widget, char *command)
{
    Data *d = widget->data;
    int i, percent;
    char * tok;

    if(!command)
        return;

    if(!d->bars)
        return;

    i = 0;
    for (tok = strtok(command, ", "); tok != NULL; tok = strtok(NULL, ", "))
    {
        percent = atoi(tok);
        if(percent <= 100 && percent >= 0)
            d->percent[i] = percent;
        if (i > d->bars )
            break;
        i++;
    }
}

Widget *
progressbar_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *color;
    int i, bars; 
    cfg_t *cfg;


    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);
    d->width = cfg_getint(config, "width");

    bars = cfg_size(config, "bar");
    if (!(bars))
    {
        d->bars = 0;
        warn("A progressbar-widget needs a: bar {} in the .awesomerc\n");
        return w;
    }
    d->bars = bars;
    d->bg = p_new(XColor, bars);
    d->fg = p_new(XColor, bars);
    d->bcolor = p_new(XColor, bars);
    d->percent = p_new(int, bars);

    for(i = 0; i < bars; i++)
    {
        cfg = cfg_getnsec(config, "bar", i);

        if((color = cfg_getstr(cfg, "fg")))
            d->fg[i] = initxcolor(statusbar->screen, color);
        else
            d->fg[i] = globalconf.screens[statusbar->screen].colors_normal[ColFG];

        if((color = cfg_getstr(cfg, "bg")))
            d->bg[i] = initxcolor(statusbar->screen, color);
        else
            d->bg[i] = globalconf.screens[statusbar->screen].colors_normal[ColBG];

        if((color = cfg_getstr(cfg, "bcolor")))
            d->bcolor[i] = initxcolor(statusbar->screen, color);
        else
            d->bcolor[i] = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    } 


    d->height = cfg_getfloat(config, "height");
    d->gap = cfg_getint(config, "gap");
    d->lpadding = cfg_getint(config, "lpadding");

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
