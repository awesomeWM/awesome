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
    float max;              /* Represents a full graph */
    int width;              /* Width of the widget */
    int padding_left;       /* Left padding */
    float height;           /* Height 0-1, where 1 is height of statusbar */
    XColor fg;              /* Foreground color */
    XColor bg;              /* Background color */
    XColor bordercolor;     /* Border color */
    int *lines;             /* Keeps the calculated values (line-length); */
    int lines_index;        /* Pointer to current value */
    int lines_size;         /* Size of lines-array (also innerbox-lenght) */
    int box_height;         /* Height of the innerbox */
    float *line_values;     /* Actual values */
    float current_max;      /* Curent maximum value */
    int line_max_index;     /* Index of the current maximum value */
} Data;

static int
graph_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    int margin_top, left_offset;
    Data *d = widget->data;
    Area rectangle;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 d->width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    margin_top = (int) (widget->statusbar->height * (1 - d->height)) / 2 + 0.5 + widget->area.y;
    left_offset = widget->area.x + d->padding_left;

    if(!(d->box_height))
        d->box_height = (int) (widget->statusbar->height * d->height + 0.5) - 2;

    rectangle.x = left_offset;
    rectangle.y = margin_top;
    rectangle.width = d->lines_size + 2;
    rectangle.height = d->box_height + 2;
    draw_rectangle(ctx, rectangle, False, d->bordercolor);

    rectangle.x++;
    rectangle.y++;
    rectangle.width -= 2;
    rectangle.height -= 2;
    draw_rectangle(ctx, rectangle, True, d->bg);

    if(d->lines[d->lines_index] < 0)
        d->lines[d->lines_index] = 0;

    draw_graph(ctx,
               left_offset + 2, margin_top + d->box_height + 1,
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
    int i;
    float value;

    if(!command || d->width < 1)
        return;

    if(++d->lines_index >= d->lines_size) /* cycle inside the array */
        d->lines_index = 0;

    value = MAX(atof(command), 0); /* TODO: may allow min-option and values */

    if(d->line_values) /* scale option is true */
    {
        d->line_values[d->lines_index] = value;

        if(value > d->current_max) /* a new maximum value found */
        {
            d->line_max_index = d->lines_index; 
            d->current_max = value;

            /* recalculate */
            for (i = 0; i < d->lines_size; i++) 
                d->lines[i] = (int) (d->line_values[i] * (d->box_height) / d->current_max + 0.5);
        }
        else if(d->line_max_index == d->lines_index) /* old max_index reached, re-check/generate */
        {
            /* find the new max */
            for (i = 0; i < d->lines_size; i++)
                if (d->line_values[i] > d->line_values[d->line_max_index])
                    d->line_max_index = i;

            d->current_max = MAX(d->line_values[d->line_max_index], d->max);

            /* recalculate */
            for (i = 0; i < d->lines_size; i++) 
                d->lines[i] = (int) (d->line_values[i] * d->box_height / d->current_max + 0.5);
        }
        else
            d->lines[d->lines_index] = (int) (value * d->box_height / d->current_max + 0.5);

    }
    else /* scale option is false */
    {
        if (value < d->current_max)
            d->lines[d->lines_index] = (int) (value * d->box_height / d->current_max + 0.5);
        else
            d->lines[d->lines_index] = d->box_height;
    }
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

    d->width = cfg_getint(config, "width");
    d->height = cfg_getfloat(config, "height");
    d->padding_left = cfg_getint(config, "padding_left");
    d->lines_size = d->width - d->padding_left - 2;

    if(d->lines_size < 1)
    {
        warn("graph widget needs: (width - padding_left) >= 3\n");
        return w;
    }

    d->lines = p_new(int, d->lines_size);

    if (cfg_getbool(config, "scale"))
        d->line_values = p_new(float, d->lines_size);

    /* prevent: division by zero; with a MIN option one day, check for div/0's */
    d->current_max = d->max = MAX(cfg_getfloat(config, "max"), 0.0001);
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
