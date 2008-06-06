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

typedef enum
{
    Top_Style = 1,
    Bottom_Style,
    Line_Style
} draw_style_t;

typedef struct
{
    /* general layout */

    char **data_title;                  /** Data title of the data sections */
    float *max_value;                   /** Represents a full graph */
    int width;                          /** Width of the widget */
    float height;                       /** Height of graph (0.0-1.0; 1.0 = height of bar) */
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
    draw_style_t *draw_style;           /** Draw style of according index */

    /* all data is stored here */
    int data_items;                     /** Number of data-input items */
    int **lines;                        /** Keeps the calculated values (line-length); */
    float **values;                     /** Actual values */

    bool *vertical_gradient;            /** Create a vertical color gradient */

    xcolor_t *color_start;              /** Color of them */
    xcolor_t **pcolor_center;           /** Color at middle of graph */
    xcolor_t **pcolor_end;              /** Color at end of graph */

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

    /* memory (re-)allocating + initialising */
    p_realloc(&(d->values), d->data_items);
    d->values[d->data_items - 1] = p_new(float, d->size);

    p_realloc(&(d->lines), d->data_items);
    d->lines[d->data_items - 1] = p_new(int, d->size);

    p_realloc(&(d->scale), d->data_items);
    d->scale[d->data_items - 1] = false;

    p_realloc(&(d->index), d->data_items);
    d->index[d->data_items - 1] = 0;

    p_realloc(&(d->max_index), d->data_items);
    d->max_index[d->data_items - 1] =  0;

    p_realloc(&(d->current_max), d->data_items);
    d->current_max[d->data_items - 1] =  0;

    p_realloc(&(d->max_value), d->data_items);
    d->max_value[d->data_items - 1] = 100.0;

    p_realloc(&(d->data_title), d->data_items);
    d->data_title[d->data_items - 1] = a_strdup(new_data_title);

    p_realloc(&(d->color_start), d->data_items);
    d->color_start[d->data_items - 1] = globalconf.colors.fg;

    p_realloc(&(d->pcolor_center), d->data_items);
    d->pcolor_center[d->data_items - 1] = NULL;

    p_realloc(&(d->pcolor_end), d->data_items);
    d->pcolor_end[d->data_items - 1] = NULL;

    p_realloc(&(d->vertical_gradient), d->data_items);
    d->vertical_gradient[d->data_items - 1] = true;

    p_realloc(&(d->draw_style), d->data_items);
    d->draw_style[d->data_items - 1] = Bottom_Style;
}

static int
graph_draw(draw_context_t *ctx,
           int screen __attribute__ ((unused)),
           widget_node_t *w,
           int offset,
           int used __attribute__ ((unused)),
           void *p __attribute__ ((unused)))
{
    int margin_top;
    int z, y, x, tmp, cur_index, test_index;
    Data *d = w->widget->data;
    area_t rectangle, pattern_area;

    if(!d->data_items)
        return 0;

    w->area.x = widget_calculate_offset(ctx->width,
                                        d->width, offset,
                                        w->widget->align);
    w->area.y = 0;

    /* box = the graph inside the rectangle */
    if(!(d->box_height))
        d->box_height = (int) (ctx->height * d->height + 0.5) - 2;

    margin_top = (int)((ctx->height - (d->box_height + 2)) / 2 + 0.5) + w->area.y;

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

    pattern_area.y = rectangle.y - rectangle.height;

    /* draw style = top */
    for(z = 0; z < d->data_items; z++)
    {
        if(d->draw_style[z] != Top_Style)
            continue;

        if(d->vertical_gradient[z])
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

        cur_index = d->index[z];

        for(y = 0; y < d->size; y++)
        {
            /* draw this filltop data thing [z]. But figure out the part
             * what shall be visible. Therefore find the next smaller value
             * on this index (draw_from) - might be 0 (then draw from start) */
            for(tmp = 0, x = 0; x < d->data_items; x++)
            {
                if(d->draw_style[x] != Top_Style)
                    continue;
                if (x == z) /* no need to compare with itself */
                    continue;

                /* current index's can be different (widget_tell might shift
                 * some with a different frequenzy), so calculate
                 * offset and compare accordingly finally */
                test_index = cur_index + (d->index[x] - d->index[z]);

                if (test_index < 0)
                    test_index = d->size + test_index; /* text_index is minus, since < 0 */
                else if(test_index >= d->size)
                    test_index -= d->size;

                /* ... (test_)index to test for a smaller value found. */

                /* if such a smaller value (to not overdraw!) is there, store into 'tmp' */
                if(d->lines[x][test_index] > tmp && d->lines[x][test_index] < d->lines[z][cur_index])
                    tmp = d->lines[x][test_index];

            }
            /* reverse values (because drawing from top) */
            d->draw_from[cur_index] = d->box_height - tmp; /* i.e. no smaller value -> from top of box */
            d->draw_to[cur_index] = d->box_height - d->lines[z][cur_index]; /* i.e. on full graph -> 0 = bottom */

            if (--cur_index < 0) /* next index to compare to other values */
                cur_index = d->size - 1;
        }
        draw_graph(ctx, rectangle , d->draw_from, d->draw_to, d->index[z], d->grow, pattern_area,
                   &(d->color_start[z]), d->pcolor_center[z], d->pcolor_end[z]);
    }

    pattern_area.y = rectangle.y;

    /* draw style = bottom */
    for(z = 0; z < d->data_items; z++)
    {
        if(d->draw_style[z] != Bottom_Style)
            continue;

        if(d->vertical_gradient[z])
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

        cur_index = d->index[z];

        for(y = 0; y < d->size; y++)
        {
            for(tmp = 0, x = 0; x < d->data_items; x++)
            {
                if(d->draw_style[x] != Bottom_Style)
                    continue;
                if (x == z)
                    continue;

                test_index = cur_index + (d->index[x] - d->index[z]);

                if (test_index < 0)
                    test_index = d->size + test_index;
                else if(test_index >= d->size)
                    test_index -= d->size;

                if(d->lines[x][test_index] > tmp && d->lines[x][test_index] < d->lines[z][cur_index])
                    tmp = d->lines[x][test_index];
            }
            d->draw_from[cur_index] = tmp;
            if (--cur_index < 0)
                cur_index = d->size - 1;
        }
        draw_graph(ctx, rectangle, d->draw_from, d->lines[z], d->index[z], d->grow, pattern_area,
                   &(d->color_start[z]), d->pcolor_center[z], d->pcolor_end[z]);
    }

    /* draw style = line */
    for(z = 0; z < d->data_items; z++)
    {
        if(d->draw_style[z] != Line_Style)
            continue;

        if(d->vertical_gradient[z])
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

        draw_graph_line(ctx, rectangle, d->lines[z], d->index[z], d->grow, pattern_area,
                &(d->color_start[z]), d->pcolor_center[z], d->pcolor_end[z]);
    }

    /* draw border (after line-drawing, what paints 0-values to the border) */
    rectangle.x = w->area.x;
    rectangle.y = margin_top;
    rectangle.width = d->size + 2;
    rectangle.height = d->box_height + 2;
    draw_rectangle(ctx, rectangle, 1.0, false, d->bordercolor);

    w->area.width = d->width;
    w->area.height = ctx->height;
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
            || !a_strcmp(property, "max_value")
            || !a_strcmp(property, "draw_style"))
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
                else if(d->max_index[fi] == d->index[fi] && d->current_max[fi] > d->max_value[fi])
                {
                    /* find the new max */
                    for(u = 0; u < d->size; u++)
                        if(d->values[fi][u] > d->values[fi][d->max_index[fi]])
                            d->max_index[fi] = u;

                    d->current_max[fi] = MAX(d->values[fi][d->max_index[fi]], d->max_value[fi]);

                    /* recalculate */
                    for(u = 0; u < d->size; u++)
                        d->lines[fi][u] = (int) (d->values[fi][u] * d->box_height / d->current_max[fi] + 0.5);
                }
                else
                    d->lines[fi][d->index[fi]] = (int) (value * d->box_height / d->current_max[fi] + 0.5);
            }
            else /* scale option is false - limit to d->box_height */
            {
                if (value < d->max_value[fi])
                    d->lines[fi][d->index[fi]] = (int) (value * d->box_height / d->max_value[fi] + 0.5);
                else
                    d->lines[fi][d->index[fi]] = d->box_height;
            }
            p_delete(&new_val);
            return WIDGET_NOERROR;
        }
        else if(!a_strcmp(property, "fg"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &(d->color_start[fi]));
        else if(!a_strcmp(property, "fg_center"))
            graph_pcolor_set(&(d->pcolor_center[fi]), setting);
        else if(!a_strcmp(property, "fg_end"))
            graph_pcolor_set(&(d->pcolor_end[fi]), setting);
        else if(!a_strcmp(property, "vertical_gradient"))
            d->vertical_gradient[fi] = a_strtobool(setting);
        else if(!a_strcmp(property, "scale"))
            d->scale[fi] = a_strtobool(setting);
        else if(!a_strcmp(property, "max_value"))
        {
            d->max_value[fi] = atof(setting);
            d->current_max[fi] = d->max_value[fi];
        }
        else if(!a_strcmp(property, "draw_style"))
        {
            if(!a_strcmp(setting, "bottom"))
                d->draw_style[fi] = Bottom_Style;
            else if(!a_strcmp(setting, "line"))
                d->draw_style[fi] = Line_Style;
            else if(!a_strcmp(setting, "top"))
                d->draw_style[fi] = Top_Style;
            else
            {
                warn("'error changing property %s of widget %s, must be 'bottom', 'top' or 'line'",
                     property, widget->name);
                p_delete(&new_val);
                return WIDGET_ERROR_CUSTOM;
            }
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
