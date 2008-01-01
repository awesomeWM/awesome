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
    /** Percent 0 to 100 */
    int *percent;
    /** Width of the bars */
    int width;
    /** Left padding */
    int lpadding;
    /** Pixel between bars */
    int gap;
    /** Number of bars */
    int bars;
    /** Height 0-1, where 1 is height of statusbar */
    float height;
    /** Foreground color */
    XColor *fg;
    /** Background color */
    XColor *bg;
    /** Border color */
    XColor *bcolor;
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
    int i = 0, percent;
    char * tok;

    if(!command || !d->bars)
        return;

    for (tok = strtok(command, ","); tok && i < d->bars; tok = strtok(NULL, ","), i++)
    {
        percent = atoi(tok);
        if(percent <= 100 && percent >= 0)
            d->percent[i] = percent;
    }
}

Widget *
progressbar_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *color;
    int i; 
    cfg_t *cfg;


    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);
    d->width = cfg_getint(config, "width");

    if(!(d->bars = cfg_size(config, "bar")))
    {
        warn("progressbar widget needs at least one bar section\n");
        return w;
    }

    d->bg = p_new(XColor, d->bars);
    d->fg = p_new(XColor, d->bars);
    d->bcolor = p_new(XColor, d->bars);
    d->percent = p_new(int, d->bars);

    for(i = 0; i < d->bars; i++)
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
            d->bcolor[i] = d->fg[i];

    } 


    d->height = cfg_getfloat(config, "height");
    d->gap = cfg_getint(config, "gap");
    d->lpadding = cfg_getint(config, "lpadding");

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
