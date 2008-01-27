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

#include <cairo.h>
#include "common/draw.h"
#include "widget.h"
#include "screen.h"
#include "common/util.h"

extern AwesomeConf globalconf;

typedef struct
{
    /* general layout */
    float *max;                 /* Represents a full graph */
    int width;                  /* Width of the widget */
    float height;               /* Height of graph (0-1, where 1 is height of statusbar) */
    int box_height;             /* Height of the innerbox in pixels */
    int padding_left;           /* Left padding */
    int size;                   /* Size of lines-array (also innerbox-lenght) */
    XColor bg;                  /* Background color */
    XColor bordercolor;         /* Border color */

    /* markers... */
    int index;                  /* Index of current (new) value */
    int *max_index;             /* Index of the actual maximum value */
    float *current_max;         /* Pointer to current maximum value itself */

    /* all data is stored here */
    int data_items;             /* Number of data-input items */
    int **lines;                /* Keeps the calculated values (line-length); */
    float **values;             /* Actual values */

    /* additional data + a pointer to **lines accordingly */
    int **fillbottom;           /* datatypes holder (same as some **lines pointer) */
    int fillbottom_total;       /* total of them */
    XColor *fillbottom_color;   /* color of them */
    int **filltop;              /* datatypes holder */
    int filltop_total;          /* total of them */
    XColor *filltop_color;      /* color of them */
    int **drawline;             /* datatypes holder */
    int drawline_total;         /* total of them */
    XColor *drawline_color;     /* color of them */

    int *draw_from;             /* Preparation/tmp array for draw_graph(); */
    int *draw_to;               /* Preparation/tmp array for draw_graph(); */

} Data;

static int
graph_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    int margin_top, left_offset;
    int z, y, x, tmp;
    Data *d = widget->data;
    Area rectangle;
    cairo_surface_t *surface;
    cairo_t *cr;

    if(d->width < 1 || !d->data_items)
        return 0;

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
    rectangle.width = d->size + 2;
    rectangle.height = d->box_height + 2;
    draw_rectangle(ctx, rectangle, False, d->bordercolor);

    rectangle.x++;
    rectangle.y++;
    rectangle.width -= 2;
    rectangle.height -= 2;
    draw_rectangle(ctx, rectangle, True, d->bg);

    draw_graph_init(ctx, &surface, &cr); /* setup drawing surface etc */

    /* draw style = top */
    for(z = 0; z < d->filltop_total; z++)
    {
        for(y = 0; y < d->size; y++)
        {
            for(tmp = 0, x = 0; x < d->filltop_total; x++) /* find largest smaller value */
            {
                if (x == z)
                    continue;

                if(d->filltop[x][y] > tmp && d->filltop[x][y] < d->filltop[z][y])
                    tmp = d->filltop[x][y];
            }
            d->draw_from[y] = d->box_height - tmp;
            d->draw_to[y] = d->box_height - d->filltop[z][y];
        }
        draw_graph(cr,
                left_offset + 2, margin_top + d->box_height + 1,
                d->size, d->draw_from, d->draw_to, d->index,
                d->filltop_color[z]);
    }

    /* draw style = bottom */
    for(z = 0; z < d->fillbottom_total; z++)
    {
        for(y = 0; y < d->size; y++)
        {
            for(tmp = 0, x = 0; x < d->fillbottom_total; x++) /* find largest smaller value */
            {
                if (x == z)
                    continue;

                if(d->fillbottom[x][y] > tmp && d->fillbottom[x][y] < d->fillbottom[z][y])
                    tmp = d->fillbottom[x][y];
            }
            d->draw_from[y] = tmp;
        }

        draw_graph(cr,
                left_offset + 2, margin_top + d->box_height + 1,
                d->size, d->draw_from, d->fillbottom[z], d->index,
                d->fillbottom_color[z]);
    }

    /* draw style = line */
    for(z = 0; z < d->drawline_total; z++)
    {
        draw_graph_line(cr,
                left_offset + 2, margin_top + d->box_height + 1,
                d->size, d->drawline[z], d->index,
                d->drawline_color[z]);
    }

    draw_graph_end(surface, cr);

    widget->area.width = d->width;
    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static void
graph_tell(Widget *widget, char *command)
{
    Data *d = widget->data;
    int i, z;
    float *value;
    char *tok;

    if(!command || d->width < 1 || !(d->data_items > 0))
        return;

    value = p_new(float, d->data_items);

    for (i = 0, tok = strtok(command, ","); tok && i < d->data_items; tok = strtok(NULL, ","), i++)
        value[i] = MAX(atof(tok), 0);

    if(++d->index >= d->size) /* cycle inside the arrays (all-in-one) */
        d->index = 0;

    /* add according values and to-draw-line-lenghts to the according data_items */
    for(z = 0; z < d->data_items; z++)
    {
        if(d->values[z]) /* scale option is true */
        {
            d->values[z][d->index] = value[z];

            if(value[z] > d->current_max[z]) /* a new maximum value found */
            {
                d->max_index[z] = d->index;
                d->current_max[z] = value[z];

                /* recalculate */
                for (i = 0; i < d->size; i++)
                    d->lines[z][i] = (int) (d->values[z][i] * (d->box_height) / d->current_max[z] + 0.5);
            }
            else if(d->max_index[z] == d->index) /* old max_index reached, re-check/generate */
            {
                /* find the new max */
                for (i = 0; i < d->size; i++)
                    if (d->values[z][i] > d->values[z][d->max_index[z]])
                        d->max_index[z] = i;

                d->current_max[z] = MAX(d->values[z][d->max_index[z]], d->max[z]);

                /* recalculate */
                for (i = 0; i < d->size; i++)
                    d->lines[z][i] = (int) (d->values[z][i] * d->box_height / d->current_max[z] + 0.5);
            }
            else
                d->lines[z][d->index] = (int) (value[z] * d->box_height / d->current_max[z] + 0.5);

        }
        else /* scale option is false - limit to d->box_height */
        {
            if (value[z] < d->current_max[z])
                d->lines[z][d->index] = (int) (value[z] * d->box_height / d->current_max[z] + 0.5);
            else
                d->lines[z][d->index] = d->box_height;
        }
    }
}

Widget *
graph_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    cfg_t *cfg;
    char *color;
    int phys_screen = get_phys_screen(statusbar->screen);
    int i;
    char *type;
    XColor tmp_color;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);

    w->draw = graph_draw;
    w->tell = graph_tell;
    d = w->data = p_new(Data, 1);

    d->width = cfg_getint(config, "width");
    d->height = cfg_getfloat(config, "height");
    d->padding_left = cfg_getint(config, "padding_left");
    d->size = d->width - d->padding_left - 2;

    if(d->size < 1)
    {
        warn("graph widget needs: (width - padding_left) >= 3\n");
        return w;
    }

    if(!(d->data_items = cfg_size(config, "data")))
    {
        warn("graph widget needs at least one data section\n");
        return w;
    }

    d->draw_from = p_new(int, d->size);
    d->draw_to = p_new(int, d->size);

    d->fillbottom = p_new(int *, d->size);
    d->filltop = p_new(int *, d->size);
    d->drawline = p_new(int *, d->size);

    d->values = p_new(float *, d->data_items);
    d->lines = p_new(int *, d->data_items);

    d->filltop_color = p_new(XColor, d->data_items);
    d->fillbottom_color = p_new(XColor, d->data_items);
    d->drawline_color = p_new(XColor, d->data_items);

    d->max_index = p_new(int, d->data_items);

    d->current_max = p_new(float, d->data_items);
    d->max = p_new(float, d->data_items);

    tmp_color = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    for(i = 0; i < d->data_items; i++)
    {
        cfg = cfg_getnsec(config, "data", i);

        if((color = cfg_getstr(cfg, "fg")))
            tmp_color = draw_color_new(globalconf.display, phys_screen, color);
        else
            tmp_color = globalconf.screens[statusbar->screen].colors_normal[ColFG];

        if (cfg_getbool(cfg, "scale"))
            d->values[i] = p_new(float, d->size); /* not null -> scale = true */

        /* prevent: division by zero */
        d->current_max[i] = d->max[i] = cfg_getfloat(cfg, "max");
        if(!(d->max[i] > 0))
        {
            warn("all graph widget needs a 'max' value greater than zero\n");
            d->data_items = 0;
            return w;
        }

        d->lines[i] = p_new(int, d->size);

        /* filter each style-typ into it's own array (for easy looping later)*/

        if ((type = cfg_getstr(cfg, "style")))
        {
            if(!strncmp(type, "bottom", sizeof("bottom")))
            {
                d->fillbottom[d->fillbottom_total] = d->lines[i];
                d->fillbottom_color[d->fillbottom_total] = tmp_color;
                d->fillbottom_total++;
            }
            else if (!strncmp(type, "top", sizeof("top")))
            {
                d->filltop[d->filltop_total] = d->lines[i];
                d->filltop_color[d->filltop_total] = tmp_color;
                d->filltop_total++;
            }
            else if (!strncmp(type, "line", sizeof("line")))
            {
                d->drawline[d->drawline_total] = d->lines[i];
                d->drawline_color[d->drawline_total] = tmp_color;
                d->drawline_total++;
            }
        }
    }

    if((color = cfg_getstr(config, "bg")))
        d->bg = draw_color_new(globalconf.display, phys_screen, color);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    if((color = cfg_getstr(config, "bordercolor")))
        d->bordercolor = draw_color_new(globalconf.display, phys_screen, color);
    else
        d->bordercolor = tmp_color;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
