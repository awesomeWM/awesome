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

typedef struct graph_t graph_t;

struct graph_t
{
    /** Grapht title of the data sections */
    char *title;
    /** Represents a full graph */
    float max_value;
    /** Scale the graph */
    bool scale;

    /* markers... */
    /** Index of current (new) value */
    int index;
    /** Index of the actual maximum value */
    int max_index;
    /** Pointer to current maximum value itself */
    float current_max;
    /** Draw style of according index */
    draw_style_t draw_style;
    /** Keeps the calculated values (line-length); */
    int *lines;
    /** Actual values */
    float *values;
    /** Color of them */
    xcolor_t color_start;
    /** Color at middle of graph */
    xcolor_t *pcolor_center;
    /** Color at end of graph */
    xcolor_t *pcolor_end;
    /** Create a vertical color gradient */
    bool vertical_gradient;
    /** Next and previous graph */
    graph_t *next, *prev;
};

DO_SLIST(graph_t, graph, p_delete)

typedef struct
{
    /** Width of the widget */
    int width;
    /** Height of graph (0.0-1.0; 1.0 = height of bar) */
    float height;
    /** Height of the innerbox in pixels */
    int box_height;
    /** Size of lines-array (also innerbox-length) */
    int size;
    /** Background color */
    xcolor_t bg;
    /** Border color */
    xcolor_t bordercolor;
    /** Grow: Left or Right */
    position_t grow;
    /** Preparation/tmp array for draw_graph(); */
    int *draw_from;
    /** Preparation/tmp array for draw_graph(); */
    int *draw_to;
    /** Graph list */
    graph_t *graphs;
} graph_data_t;

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

static graph_t *
graph_data_add(graph_data_t *d, const char *new_data_title)
{
    graph_t *graph = p_new(graph_t, 1);

    /* memory (re-)allocating + initialising */
    graph->values = p_new(float, d->size);
    graph->lines = p_new(int, d->size);
    graph->max_value = 100.0;
    graph->title = a_strdup(new_data_title);
    graph->color_start = globalconf.colors.fg;
    graph->vertical_gradient = true;
    graph->draw_style = Bottom_Style;

    graph_list_append(&d->graphs, graph);

    return graph;
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
    int y, tmp, cur_index, test_index;
    graph_data_t *d = w->widget->data;
    area_t rectangle, pattern_area;
    graph_t *graph, *graphtmp;

    if(!d->graphs)
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

    for(graph = d->graphs; graph; graph = graph->next)
    {
        if(graph->draw_style != Top_Style)
            continue;

        if(graph->vertical_gradient)
        {
            pattern_area.width = 0;
            pattern_area.height = rectangle.height;
        }
        else
        {
            pattern_area.height = 0;

            if(d->grow == Right)
                pattern_area.width = - rectangle.width;
            else
                pattern_area.width = rectangle.width;
        }

        cur_index = graph->index;

        for(y = 0; y < d->size; y++)
        {
            /* draw this filltop data thing. But figure out the part
             * what shall be visible. Therefore find the next smaller value
             * on this index (draw_from) - might be 0 (then draw from start) */
            tmp = 0;
            for(graphtmp = d->graphs; graphtmp; graphtmp = graphtmp->next)
            {
                /* no need to compare with itself */
                if(graphtmp == graph || graphtmp->draw_style != Top_Style)
                    continue;

                /* current index's can be different (widget_tell might shift
                 * some with a different frequenzy), so calculate
                 * offset and compare accordingly finally */
                test_index = cur_index + (graphtmp->index - graph->index);

                if(test_index < 0)
                    test_index = d->size + test_index; /* text_index is minus, since < 0 */
                else if(test_index >= d->size)
                    test_index -= d->size;

                /* if such a smaller value (to not overdraw!) is there, store into 'tmp' */
                if(graphtmp->lines[test_index] < graph->lines[cur_index])
                    tmp = MIN(graphtmp->lines[test_index], tmp);
            }
            /* reverse values (because drawing from top) */
            d->draw_from[cur_index] = d->box_height - tmp; /* i.e. no smaller value -> from top of box */
            d->draw_to[cur_index] = d->box_height - graph->lines[cur_index]; /* i.e. on full graph -> 0 = bottom */

            if (--cur_index < 0) /* next index to compare to other values */
                cur_index = d->size - 1;
        }
        draw_graph(ctx, rectangle , d->draw_from, d->draw_to, graph->index, d->grow, pattern_area,
                   &graph->color_start, graph->pcolor_center, graph->pcolor_end);
    }

    pattern_area.y = rectangle.y;

    /* draw style = bottom */
    for(graph = d->graphs; graph; graph = graph->next)
    {
        if(graph->draw_style != Bottom_Style)
            continue;

        if(graph->vertical_gradient)
        {
            pattern_area.width = 0;
            pattern_area.height = - rectangle.height;
        }
        else
        {
            pattern_area.height = 0;

            if(d->grow == Right)
                pattern_area.width = - rectangle.width;
            else
                pattern_area.width = rectangle.width;
        }

        cur_index = graph->index;

        for(y = 0; y < d->size; y++)
        {
            tmp = 0;
            for(graphtmp = d->graphs; graphtmp; graphtmp = graphtmp->next)
            {
                if(graph == graphtmp || graphtmp->draw_style != Bottom_Style)
                    continue;

                test_index = cur_index + (graphtmp->index - graph->index);

                if (test_index < 0)
                    test_index = d->size + test_index;
                else if(test_index >= d->size)
                    test_index -= d->size;

                if(graphtmp->lines[test_index] < graph->lines[cur_index])
                    tmp = MIN(graphtmp->lines[test_index], tmp);
            }
            d->draw_from[cur_index] = tmp;
            if(--cur_index < 0)
                cur_index = d->size - 1;
        }
        draw_graph(ctx, rectangle, d->draw_from, graph->lines, graph->index, d->grow, pattern_area,
                   &graph->color_start, graph->pcolor_center, graph->pcolor_end);
    }

    /* draw style = line */
    for(graph = d->graphs; graph; graph = graph->next)
    {
        if(graph->draw_style != Line_Style)
            continue;

        if(graph->vertical_gradient)
        {
            pattern_area.width = 0;
            pattern_area.height = -rectangle.height;
        }
        else
        {
            pattern_area.height = 0;
            if(d->grow == Right)
                pattern_area.width = - rectangle.width;
            else
                pattern_area.width = rectangle.width;
        }

        draw_graph_line(ctx, rectangle, graph->lines, graph->index, d->grow, pattern_area,
                &graph->color_start, graph->pcolor_center, graph->pcolor_end);
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
    graph_data_t *d = widget->data;
    graph_t *graph;
    int i;
    float value;
    char *title, *setting;
    char *new_val;

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
        for(graph = d->graphs; graph; graph = graph->next)
            if(!a_strcmp(title, graph->title))
                break;
        /* no section found -> create one */
        if(!graph)
            graph = graph_data_add(d, title);

        /* change values accordingly... */
        if(!a_strcmp(property, "data"))
        {
            /* assign incoming value */
            value = MAX(atof(setting), 0);

            if(++graph->index >= d->size) /* cycle inside the array */
                graph->index = 0;

            if(graph->scale) /* scale option is true */
            {
                graph->values[graph->index] = value;

                if(value > graph->current_max) /* a new maximum value found */
                {
                    graph->max_index = graph->index;
                    graph->current_max = value;

                    /* recalculate */
                    for (i = 0; i < d->size; i++)
                        graph->lines[i] = (int) (graph->values[i] * d->box_height
                                                 / graph->current_max + 0.5);
                }
                /* old max_index reached + current_max > normal, re-check/generate */
                else if(graph->max_index == graph->index
                        && graph->current_max > graph->max_value)
                {
                    /* find the new max */
                    for(i = 0; i < d->size; i++)
                        if(graph->values[i] > graph->values[graph->max_index])
                            graph->max_index = i;

                    graph->current_max = MAX(graph->values[graph->max_index], graph->max_value);

                    /* recalculate */
                    for(i = 0; i < d->size; i++)
                        graph->lines[i] = (int) (graph->values[i] * d->box_height
                                                 / graph->current_max + 0.5);
                }
                else
                    graph->lines[graph->index] = (int) (value * d->box_height
                                                        / graph->current_max + 0.5);
            }
            else /* scale option is false - limit to d->box_height */
            {
                if(value < graph->max_value)
                    graph->lines[graph->index] = (int) (value * d->box_height
                                                        / graph->max_value + 0.5);
                else
                    graph->lines[graph->index] = d->box_height;
            }
            p_delete(&new_val);
            return WIDGET_NOERROR;
        }
        else if(!a_strcmp(property, "fg"))
            xcolor_new(globalconf.connection, globalconf.default_screen, setting, &graph->color_start);
        else if(!a_strcmp(property, "fg_center"))
            graph_pcolor_set(&graph->pcolor_center, setting);
        else if(!a_strcmp(property, "fg_end"))
            graph_pcolor_set(&graph->pcolor_end, setting);
        else if(!a_strcmp(property, "vertical_gradient"))
            graph->vertical_gradient = a_strtobool(setting);
        else if(!a_strcmp(property, "scale"))
            graph->scale = a_strtobool(setting);
        else if(!a_strcmp(property, "max_value"))
        {
            graph->max_value = atof(setting);
            graph->current_max = graph->max_value;
        }
        else if(!a_strcmp(property, "draw_style"))
        {
            if(!a_strcmp(setting, "bottom"))
                graph->draw_style = Bottom_Style;
            else if(!a_strcmp(setting, "line"))
                graph->draw_style = Line_Style;
            else if(!a_strcmp(setting, "top"))
                graph->draw_style = Top_Style;
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
        for(graph = d->graphs; graph; graph = graph->next)
        {
            p_realloc(&graph->values, d->size);
            p_realloc(&graph->lines, d->size);
            p_clear(&graph->values, d->size);
            p_clear(&graph->lines, d->size);
            graph->index = 0;
            graph->current_max =  0;
            graph->max_index =  0;
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
    graph_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);

    w->draw = graph_draw;
    w->tell = graph_tell;
    w->align = align;
    d = w->data = p_new(graph_data_t, 1);

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
