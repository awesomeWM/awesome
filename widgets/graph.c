/*
 * graph.c - a graph widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007-2008 Marco Candrian <mac@calmar.ws>
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

#include "util.h"
#include "draw.h"
#include "widget.h"
#include "xutil.h"
#include "screen.h"

extern AwesomeConf globalconf;

typedef struct
{
    int percent;            /* Percent 0 to 100 */
    int width;              /* Width of the widget */
    int padding_left;       /* Left padding */
    float height;           /* Height 0-1, where 1 is height of statusbar */
    XColor fg;              /* Foreground color */
    XColor bg;              /* Background color */
    XColor bordercolor;     /* Border color */

    int *lines;             /* keeps the calculated values (line-length); */
    int lines_index;        /* pointer to current val */
    int lines_size;         /* size of lines-array */
} Data;



static int
graph_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    int width, height; /* width/height (pixel) to draw the outer box */
    int margin_top, left_offset;
    Data *d = widget->data;

    width = d->lines_size + 2;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 d->width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    margin_top = (int) (widget->statusbar->height * (1 - d->height)) / 2 + 0.5 + widget->area.y;
    height = (int) (widget->statusbar->height * d->height + 0.5);
    left_offset = widget->area.x + d->padding_left;

    draw_rectangle(ctx,
        left_offset, margin_top,
        width, height,
        False, d->bordercolor);

    draw_rectangle(ctx,
        left_offset + 1, margin_top + 1,
        width - 2, height - 2,
        True, d->bg);

    /* computes line-length to draw and store into lines[] */
    d->lines[d->lines_index] = (int) (d->percent) * (height - 2 ) / 100 + 0.5;

    if(d->lines[d->lines_index] < 0)
        d->lines[d->lines_index] = 0;

    draw_graph(ctx,
               left_offset + 2, margin_top + height - 1,
               d->lines_size, d->lines, d->lines_index,
               d->fg);

    widget->area.width = d->width;
    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static void
graph_tell(Widget *widget, char *command)
{
    Data *d = widget->data;
    int percent;

    if(!command || d->width < 1)
        return;

    if(++d->lines_index >= d->lines_size) /* cycle inside the array */
        d->lines_index = 0;

    percent = atoi(command);
    if(percent <= 100 && percent >= 0)
        d->percent = percent;
}

Widget *
graph_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *color;
    int phys_screen = get_phys_screen(statusbar->screen);

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);

    w->draw = graph_draw;
    w->tell = graph_tell;
    d = w->data = p_new(Data, 1);

    d->height = cfg_getfloat(config, "height");
    d->width = cfg_getint(config, "width");
    d->padding_left = cfg_getint(config, "padding_left");

    d->lines_size = d->width - d->padding_left - 2;
    d->lines_index = -1; /* graph_tell will increment it to 0 (to begin with...) */

    if(d->lines_size < 1)
    {
        warn("graph widget needs: (width - padding_left) >= 3\n");
        return w;
    }

    d->lines = p_new(int, d->lines_size);

    if((color = cfg_getstr(config, "fg")))
        d->fg = initxcolor(phys_screen, color);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((color = cfg_getstr(config, "bg")))
        d->bg = initxcolor(phys_screen, color);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    if((color = cfg_getstr(config, "bordercolor")))
        d->bordercolor = initxcolor(phys_screen, color);
    else
        d->bordercolor = d->fg;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
