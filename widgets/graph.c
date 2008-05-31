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

#include "widget.h"
#include "screen.h"
#include "common/draw.h"

extern awesome_t globalconf;

typedef struct
{
    /* general layout */

    char **data_title;                  /** Data title of the data sections */
    float *max;                         /** Represents a full graph */
    int width;                          /** Width of the widget */
    float height;                       /** Height of graph (0.0-1.0; 1.0 = height of statusbar) */
    int box_height;                     /** Height of the innerbox in pixels */
    int size;                           /** Size of lines-array (also innerbox-lenght) */
    xcolor_t bg;                        /** Background color */
    xcolor_t bordercolor;               /** Border color */
    position_t grow;                    /** grow: Left or Right */

    bool *scale;                        /** Scale the graph */

    /* markers... */
    int *index;                         /** Index of current (new) value */
    int *max_index;                     /** Index of the actual maximum value */
    float *current_max;                 /** Pointer to current maximum value itself */

    /* all data is stored here */
    int data_items;                     /** Number of data-input items */
    int **lines;                        /** Keeps the calculated values (line-length); */
    float **values;                     /** Actual values */

    /* additional data + a pointer to **lines accordingly */
    int **fillbottom;                   /** Data array pointer (point to some *lines) */
    int **fillbottom_index;             /** Points to some index[i] */
    int fillbottom_total;               /** Total of them */
    bool *fillbottom_vertical_grad;     /** Create a vertical color gradient */
    xcolor_t *fillbottom_color;         /** Color of them */
    xcolor_t **fillbottom_pcolor_center;/** Color at middle of graph */
    xcolor_t **fillbottom_pcolor_end;   /** Color at end of graph */
    int **filltop;                      /** Data array pointer (points to some *lines) */
    int **filltop_index;                /** Points to some index[i] */
    int filltop_total;                  /** Total of them */
    bool *filltop_vertical_grad;        /** Create a vertical color gradient */
    xcolor_t *filltop_color;            /** Color of them */
    xcolor_t **filltop_pcolor_center;   /** Color at center of graph */
    xcolor_t **filltop_pcolor_end;      /** Color at end of graph */
    int **drawline;                     /** Data array pointer (points to some *lines) */
    int **drawline_index;               /** Points to some index[i] */
    int drawline_total;                 /** Total of them */
    bool *drawline_vertical_grad;       /** Create a vertical color gradient */
    xcolor_t *drawline_color;           /** Color of them */
    xcolor_t **drawline_pcolor_center;  /** Color at middle of graph */
    xcolor_t **drawline_pcolor_end;     /** Color at end of graph */

    int *draw_from;                     /** Preparation/tmp array for draw_graph(); */
    int *draw_to;                       /** Preparation/tmp array for draw_graph(); */
} Data;

/* the same as the progressbar_pcolor_set may use a common function */
static void
graph_pcolor_set(xcolor_t **ppcolor, char *new_color)
{
    bool flag = false;
    if(!*ppcolor)
    {
        flag = true; /* p_delete && restore to NULL, if xcolor_new unsuccessful */
        *ppcolor = p_new(xcolor_t, 1);
    }
    if(!(xcolor_new(globalconf.connection,
                    globalconf.default_screen,
                    new_color, *ppcolor))
       && flag)
        p_delete(ppcolor);
}

static void
graph_data_add(Data *d, const char *new_data_title)
{
    d->data_items++;

    /* memory (re-)allocating */
    p_realloc(&(d->values), d->data_items);
    p_realloc(&(d->lines), d->data_items);

    p_realloc(&(d->scale), d->data_items);

    p_realloc(&(d->index), d->data_items);
    p_realloc(&(d->max_index), d->data_items);
    p_realloc(&(d->current_max), d->data_items); /* \todo value itself - rename */

    p_realloc(&(d->max), d->data_items);

    p_realloc(&(d->data_title), d->data_items);

    p_realloc(&(d->fillbottom), d->data_items);
    p_realloc(&(d->fillbottom_index), d->data_items);
    p_realloc(&(d->filltop), d->data_items);
    p_realloc(&(d->filltop_index), d->data_items);
    p_realloc(&(d->drawline), d->data_items);
    p_realloc(&(d->drawline_index), d->data_items);

    p_realloc(&(d->fillbottom_color), d->data_items);
    p_realloc(&(d->fillbottom_color), d->data_items);
    p_realloc(&(d->fillbottom_pcolor_center), d->data_items);
    p_realloc(&(d->fillbottom_pcolor_end), d->data_items);
    p_realloc(&(d->fillbottom_vertical_grad), d->data_items);
    p_realloc(&(d->filltop_color), d->data_items);
    p_realloc(&(d->filltop_color), d->data_items);
    p_realloc(&(d->filltop_pcolor_center), d->data_items);
    p_realloc(&(d->filltop_pcolor_end), d->data_items);
    p_realloc(&(d->filltop_vertical_grad), d->data_items);
    p_realloc(&(d->drawline_color), d->data_items);
    p_realloc(&(d->drawline_color), d->data_items);
    p_realloc(&(d->drawline_pcolor_center), d->data_items);
    p_realloc(&(d->drawline_pcolor_end), d->data_items);
    p_realloc(&(d->drawline_vertical_grad), d->data_items);

    /* initialize values for new data section */
    d->index[d->data_items - 1] = 0;
    d->values[d->data_items - 1] = p_new(float, d->size);
    d->lines[d->data_items - 1] = p_new(int, d->size);

    d->scale[d->data_items - 1] = false;

    d->data_title[d->data_items - 1] = a_strdup(new_data_title);

    d->current_max[d->data_items - 1] =  0;
    d->max_index[d->data_items - 1] =  0;
    d->max[d->data_items - 1] = 100.0;

    /* \fixme: initialise fillbottom by default */
    d->fillbottom[d->fillbottom_total] = d->lines[d->data_items - 1];
    d->fillbottom_index[d->fillbottom_total] = &d->index[d->data_items - 1];
    d->fillbottom_color[d->fillbottom_total] = globalconf.colors.fg;
    d->fillbottom_pcolor_center[d->fillbottom_total] = NULL;
    d->fillbottom_pcolor_end[d->fillbottom_total] = NULL;
    d->fillbottom_vertical_grad[d->fillbottom_total] = true;
    d->fillbottom_total++;
}

static int
graph_draw(widget_node_t *w, statusbar_t *statusbar, int offset,
           int used __attribute__ ((unused)))
{
    int margin_top;
    int z, y, x, tmp, cur_index, test_index;
    Data *d = w->widget->data;
    area_t rectangle, pattern_area;
    draw_context_t *ctx = statusbar->ctx;

    if(!d->data_items)
        return 0;

    w->area.x = widget_calculate_offset(statusbar->width,
                                        d->width, offset,
                                        w->widget->align);
    w->area.y = 0;

    /* box = the graph inside the rectangle */
    if(!(d->box_height))
        d->box_height = (int) (statusbar->height * d->height + 0.5) - 2;

    margin_top = (int)((statusbar->height - (d->box_height + 2)) / 2 + 0.5) + w->area.y;

    /* draw background */
    rectangle.x = w->area.x + 1;
    rectangle.y = margin_top + 1;
    rectangle.width = d->size;
    rectangle.height = d->box_height;
    draw_rectangle(ctx, rectangle, 1.0, true, d->bg);

    /* for graph drawing */
    rectangle.y = margin_top + d->box_height + 1; /* bottom left corner as starting point */
    rectangle.width = d->size; /* rectangle.height is not used */

    draw_graph_setup(ctx); /* setup some drawing options */

    /* gradient begin either left or on the right of the rectangle */
    if(d->grow == Right)
        pattern_area.x = rectangle.x + rectangle.width;
    else
        pattern_area.x = rectangle.x;

    if(d->filltop_total)
    {
        pattern_area.y = rectangle.y - rectangle.height;

        /* draw style = top */
        for(z = 0; z < d->filltop_total; z++)
        {
            if(d->filltop_vertical_grad[z])
            {
                pattern_area.width = 0;
                pattern_area.height = rectangle.height;
            }
            else
            {
                pattern_area.height = 0;

                if(d->grow == Right)
                    pattern_area.width = -rectangle.width;
                else
                    pattern_area.width = rectangle.width;
            }

            cur_index = *(d->filltop_index[z]);

            for(y = 0; y < d->size; y++)
            {
                /* draw this filltop data thing [z]. But figure out the part
                 * what shall be visible. Therefore find the next smaller value
                 * on this index (draw_from) - might be 0 (then draw from start) */
                for(tmp = 0, x = 0; x < d->filltop_total; x++)
                {
                    if (x == z) /* no need to compare with itself */
                        continue;

                    /* current index's can be different (widget_tell might shift
                     * some with a different frequenzy), so calculate
                     * offset and compare accordingly finally */
                    test_index = cur_index + (*(d->filltop_index[x]) - *(d->filltop_index[z]));

                    if (test_index < 0)
                        test_index = d->size + test_index; /* text_index is minus, since < 0 */
                    else if(test_index >= d->size)
                        test_index -= d->size;

                    /* ... (test_)index to test for a smaller value found. */

                    /* if such a smaller value (to not overdraw!) is there, store into 'tmp' */
                    if(d->filltop[x][test_index] > tmp && d->filltop[x][test_index] < d->filltop[z][cur_index])
                        tmp = d->filltop[x][test_index];

                }
                /* reverse values (because drawing from top) */
                d->draw_from[cur_index] = d->box_height - tmp; /* i.e. no smaller value -> from top of box */
                d->draw_to[cur_index] = d->box_height - d->filltop[z][cur_index]; /* i.e. on full graph -> 0 = bottom */

                if (--cur_index < 0) /* next index to compare to other values */
                    cur_index = d->size - 1;
            }
            draw_graph(ctx, rectangle , d->draw_from, d->draw_to, *(d->filltop_index[z]), d->grow, pattern_area,
                       &(d->filltop_color[z]), d->filltop_pcolor_center[z], d->filltop_pcolor_end[z]);
        }
    }

    pattern_area.y = rectangle.y;

    if(d->fillbottom_total)
    {
        /* draw style = bottom */
        for(z = 0; z < d->fillbottom_total; z++)
        {
            if(d->fillbottom_vertical_grad[z])
            {
                pattern_area.width = 0;
                pattern_area.height = -rectangle.height;
            }
            else
            {
                pattern_area.height = 0;

                if(d->grow == Right)
                    pattern_area.width = -rectangle.width;
                else
                    pattern_area.width = rectangle.width;
            }

            cur_index = *(d->fillbottom_index[z]);

            for(y = 0; y < d->size; y++)
            {
                for(tmp = 0, x = 0; x < d->fillbottom_total; x++)
                {
                    if (x == z)
                        continue;

                    test_index = cur_index + (*(d->fillbottom_index[x]) - *(d->fillbottom_index[z]));

                    if (test_index < 0)
                        test_index = d->size + test_index;
                    else if(test_index >= d->size)
                        test_index -= d->size;

                    if(d->fillbottom[x][test_index] > tmp && d->fillbottom[x][test_index] < d->fillbottom[z][cur_index])
                        tmp = d->fillbottom[x][test_index];
                }
                d->draw_from[cur_index] = tmp;
                if (--cur_index < 0)
                    cur_index = d->size - 1;
            }
            draw_graph(ctx, rectangle, d->draw_from, d->fillbottom[z], *(d->fillbottom_index[z]), d->grow,
                       pattern_area, &(d->fillbottom_color[z]), d->fillbottom_pcolor_center[z], d->fillbottom_pcolor_end[z]);
        }
    }

    if(d->drawline_total)
    {
        /* draw style = line */
        for(z = 0; z < d->drawline_total; z++)
        {
            if(d->drawline_vertical_grad[z])
            {
                pattern_area.width = 0;
                pattern_area.height = -rectangle.height;
            }
            else
            {
                pattern_area.height = 0;
                if(d->grow == Right)
                    pattern_area.width = -rectangle.width;
                else
                    pattern_area.width = rectangle.width;
            }

            draw_graph_line(ctx, rectangle, d->drawline[z], *(d->drawline_index[z]), d->grow, pattern_area,
                            &(d->drawline_color[z]), d->drawline_pcolor_center[z], d->drawline_pcolor_end[z]);
        }
    }

    /* draw border (after line-drawing, what paints 0-values to the border) */
    rectangle.x = w->area.x;
    rectangle.y = margin_top;
    rectangle.width = d->size + 2;
    rectangle.height = d->box_height + 2;
    draw_rectangle(ctx, rectangle, 1.0, false, d->bordercolor);

    w->area.width = d->width;
    w->area.height = statusbar->height;
    return w->area.width;
}

static widget_tell_status_t
graph_tell(widget_t *widget, const char *property, const char *new_value)
{
    Data *d = widget->data;
    int i, u, fi;
    float value;
    char *title, *setting;
    char *new_val;
    bool found;

    if(!new_value)
        return WIDGET_ERROR_NOVALUE;

    /* following properties need a datasection */
    else if(!a_strcmp(property, "data")
            || !a_strcmp(property, "fg")
            || !a_strcmp(property, "fg_center")
            || !a_strcmp(property, "fg_end")
            || !a_strcmp(property, "vertical_gradient")
            || !a_strcmp(property, "scale")
            || !a_strcmp(property, "max")
            || !a_strcmp(property, "reverse"))
    {
        /* check if this section is defined already */
        new_val = a_strdup(new_value);
        title = strtok(new_val, " ");
        if(!(setting = strtok(NULL, " ")))
        {
            p_delete(&new_val);
            return WIDGET_ERROR_NOVALUE;
        }
        for(found = false, fi = 0; fi < d->data_items; fi++)
        {
            if(!a_strcmp(title, d->data_title[fi]))
            {
                found = true;
                break;
            }
        }
        /* no section found -> create one */
        if(!found)
        {
            graph_data_add(d, title);
            fi = d->data_items - 1;
        }

        /* change values accordingly... */
        if(!a_strcmp(property, "data"))
        {
            /* assign incoming value */
            value = MAX(atof(setting), 0);

            if(++d->index[fi] >= d->size) /* cycle inside the array */
                d->index[fi] = 0;

            if(d->scale[fi]) /* scale option is true */
            {
                d->values[fi][d->index[fi]] = value;

                if(value > d->current_max[fi]) /* a new maximum value found */
                {
                    d->max_index[fi] = d->index[fi];
                    d->current_max[fi] = value;

                    /* recalculate */
                    for (u = 0; u < d->size; u++)
                        d->lines[fi][u] = (int) (d->values[fi][u] * (d->box_height) / d->current_max[fi] + 0.5);
                }
                /* old max_index reached + current_max > normal, re-check/generate */
                else if(d->max_index[fi] == d->index[fi] && d->current_max[fi] > d->max[fi])
                {
                    /* find the new max */
                    for(u = 0; u < d->size; u++)
                        if(d->values[fi][u] > d->values[fi][d->max_index[fi]])
                            d->max_index[fi] = u;

                    d->current_max[fi] = MAX(d->values[fi][d->max_index[fi]], d->max[fi]);

                    /* recalculate */
                    for(u = 0; u < d->size; u++)
                        d->lines[fi][u] = (int) (d->values[fi][u] * d->box_height / d->current_max[fi] + 0.5);
                }
                else
                    d->lines[fi][d->index[fi]] = (int) (value * d->box_height / d->current_max[fi] + 0.5);
            }
            else /* scale option is false - limit to d->box_height */
            {
                if (value < d->max[fi])
                    d->lines[fi][d->index[fi]] = (int) (value * d->box_height / d->max[fi] + 0.5);
                else
                    d->lines[fi][d->index[fi]] = d->box_height;
            }
            p_delete(&new_val);
            return WIDGET_NOERROR;
        }
        /* TODO: using everything as strtobottom mixing e.g. FIX scale especially */
        else if(!a_strcmp(property, "fg"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &(d->fillbottom_color[fi]));
        else if(!a_strcmp(property, "fg_center"))
            graph_pcolor_set(&(d->fillbottom_pcolor_center[fi]), setting);
        else if(!a_strcmp(property, "fg_end"))
            graph_pcolor_set(&(d->fillbottom_pcolor_end[fi]), setting);
        else if(!a_strcmp(property, "vertical_gradient"))
            d->fillbottom_vertical_grad[fi] = a_strtobool(setting);
        else if(!a_strcmp(property, "scale"))
            d->scale[fi] = a_strtobool(setting);
        else if(!a_strcmp(property, "max"))
        {
            d->max[fi] = atof(setting);
            d->current_max[fi] = d->max[fi];
        }

        p_delete(&new_val);
        return WIDGET_NOERROR;
    }
    else if(!a_strcmp(property, "height"))
        d->height = atof(new_value);
    else if(!a_strcmp(property, "width"))
    {
        d->width = atoi(new_value);
        d->size = d->width - 2;
        /* re-allocate/initialise necessary values */
        for(i = 0; i < d->data_items; i++)
        {
            p_delete(&d->values[i]);
            p_delete(&d->lines[i]);
            d->values[i] = p_new(float, d->size);
            d->lines[i] = p_new(int, d->size);

            d->fillbottom[i] = d->lines[i];

            d->index[i] = 0;
            d->current_max[i] =  0;
            d->max_index[i] =  0;
        }
    }
    else if(!a_strcmp(property, "bg"))
    {
        if(!xcolor_new(globalconf.connection,
                           globalconf.default_screen,
                           new_value, &d->bg))
            return WIDGET_ERROR_FORMAT_COLOR;
    }
    else if(!a_strcmp(property, "bordercolor"))
    {
        if(!xcolor_new(globalconf.connection,
                           globalconf.default_screen,
                           new_value, &d->bordercolor))
            return WIDGET_ERROR_FORMAT_COLOR;
    }
    else if(!a_strcmp(property, "grow"))
        switch((d->grow = position_get_from_str(new_value)))
        {
          case Left:
          case Right:
            break;
          default:
            warn("error changing property %s of widget %s, must be 'left' or 'right'",
                 property, widget->name);
            return WIDGET_ERROR_CUSTOM;
        }
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
graph_new(alignment_t align)
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);

    w->draw = graph_draw;
    w->tell = graph_tell;
    w->align = align;
    d = w->data = p_new(Data, 1);

    d->width = 80;
    d->height = 0.80;
    d->size = d->width - 2;
    d->grow = Left;
    d->draw_from = p_new(int, d->size);
    d->draw_to = p_new(int, d->size);

    d->bg = globalconf.colors.bg;
    d->bordercolor = globalconf.colors.fg;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
