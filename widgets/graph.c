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

#include <math.h>

#include "widget.h"
#include "common/tokenize.h"
#include "common/draw.h"

extern awesome_t globalconf;

typedef enum
{
    Bottom_Style = 0,
    Top_Style,
    Line_Style
} plot_style_t;

typedef struct plot_t plot_t;

/** The plot data structure. */
struct plot_t
{
    /** Grapht title of the plot sections */
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
    plot_style_t draw_style;
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
    plot_t *next, *prev;
};

static void
plot_delete(plot_t **g)
{
    p_delete(&(*g)->title);
    p_delete(&(*g)->lines);
    p_delete(&(*g)->values);
    p_delete(&(*g)->pcolor_center);
    p_delete(&(*g)->pcolor_end);
    p_delete(g);
}

DO_SLIST(plot_t, plot, plot_delete)

/** The private graph data structure */
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
    plot_t *plots;
} graph_data_t;

static void
plot_pcolor_set(xcolor_t **ppcolor, const char *new_color)
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

/** Add a plot to a graph.
 * \param d The graph private data.
 * \param title The plot title.
 * \return A new plot.
 */
static plot_t *
graph_plot_add(graph_data_t *d, const char *title)
{
    plot_t *plot = p_new(plot_t, 1);

    plot->title = a_strdup(title);
    plot->values = p_new(float, d->size);
    plot->lines = p_new(int, d->size);
    plot->max_value = 100.0;
    plot->color_start = globalconf.colors.fg;
    plot->vertical_gradient = true;

    plot_list_append(&d->plots, plot);

    return plot;
}

/** Draw a graph widget.
 * \param ctx The draw context.
 * \param screen The screen number.
 * \param w The widget node we are called from.
 * \param offset The offset to draw at.
 * \param used The already used width.
 * \param p A pointer to the object we're drawing onto.
 */
static int
graph_draw(draw_context_t *ctx,
           int screen __attribute__ ((unused)),
           widget_node_t *w,
           int offset,
           int used __attribute__ ((unused)),
           void *p __attribute__ ((unused)))
{
    int margin_top, y;
    graph_data_t *d = w->widget->data;
    area_t rectangle, pattern_area;
    plot_t *plot;

    if(!d->plots)
        return 0;

    w->area.x = widget_calculate_offset(ctx->width,
                                        d->width, offset,
                                        w->widget->align);
    w->area.y = 0;

    /* box = the plot inside the rectangle */
    if(!d->box_height)
        d->box_height = round(ctx->height * d->height) - 2;

    margin_top = round((ctx->height - (d->box_height + 2)) / 2) + w->area.y;

    /* draw background */
    rectangle.x = w->area.x + 1;
    rectangle.y = margin_top + 1;
    rectangle.width = d->size;
    rectangle.height = d->box_height;
    draw_rectangle(ctx, rectangle, 1.0, true, d->bg);

    /* for plot drawing */
    rectangle.y = margin_top + d->box_height + 1; /* bottom left corner as starting point */
    rectangle.width = d->size; /* rectangle.height is not used */

    draw_graph_setup(ctx); /* setup some drawing options */

    /* gradient begin either left or on the right of the rectangle */
    if(d->grow == Right)
        pattern_area.x = rectangle.x + rectangle.width;
    else
        pattern_area.x = rectangle.x;

    for(plot = d->plots; plot; plot = plot->next)
        switch(plot->draw_style)
        {
            case Top_Style:
              pattern_area.y = rectangle.y - rectangle.height;
              if(plot->vertical_gradient)
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

              for(y = 0; y < d->size; y++)
              {
                  /* reverse values (because drawing from top) */
                  d->draw_from[y] = d->box_height; /* i.e. no smaller value -> from top of box */
                  d->draw_to[y] = d->box_height - plot->lines[y]; /* i.e. on full plot -> 0 = bottom */
              }
              draw_graph(ctx, rectangle , d->draw_from, d->draw_to, plot->index, d->grow, pattern_area,
                         &plot->color_start, plot->pcolor_center, plot->pcolor_end);
              break;
            case Bottom_Style:
              pattern_area.y = rectangle.y;
              if(plot->vertical_gradient)
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

              p_clear(d->draw_from, d->size);
              draw_graph(ctx, rectangle, d->draw_from, plot->lines, plot->index, d->grow, pattern_area,
                         &plot->color_start, plot->pcolor_center, plot->pcolor_end);
              break;
            case Line_Style:
              pattern_area.y = rectangle.y;
              if(plot->vertical_gradient)
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

              draw_graph_line(ctx, rectangle, plot->lines, plot->index, d->grow, pattern_area,
                              &plot->color_start, plot->pcolor_center, plot->pcolor_end);
              break;
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

/** Set various graph general properties.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A widget.
 * \lparam A table with various properties set.
 */
static int
luaA_graph_properties_set(lua_State *L)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    graph_data_t *d = (*widget)->data;
    plot_t *plot;
    int width;
    const char *buf;
    size_t len;
    position_t pos;

    luaA_checktable(L, 2);

    d->height = luaA_getopt_number(L, 2, "height", d->height);

    width = luaA_getopt_number(L, 2, "width", d->width);
    if(width != d->width)
    {
        d->width = width;
        d->size = d->width - 2;
        for(plot = d->plots; plot; plot = plot->next)
        {
            p_realloc(&plot->values, d->size);
            p_realloc(&plot->lines, d->size);
            p_clear(plot->values, d->size);
            p_clear(plot->lines, d->size);
            plot->index = 0;
            plot->current_max =  0;
            plot->max_index =  0;
        }
    }

    if((buf = luaA_getopt_string(L, 2, "bg", NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen, buf, &d->bg);
    if((buf = luaA_getopt_string(L, 2, "bordercolor", NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen, buf, &d->bordercolor);

    if((buf = luaA_getopt_lstring(L, 2, "grow", NULL, &len)))
        switch((pos = position_fromstr(buf, len)))
        {
          case Left:
          case Right:
            d->grow = pos;
            break;
          default:
            break;
        }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Set various plot graph properties.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A widget.
 * \lparam A plot name.
 * \lparam A table with various properties set.
 */
static int
luaA_graph_plot_properties_set(lua_State *L)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    graph_data_t *d = (*widget)->data;
    float max_value;
    const char *title, *buf;
    size_t len;
    plot_t *plot;

    title = luaL_checkstring(L, 2);
    luaA_checktable(L, 3);

    for(plot = d->plots; plot; plot = plot->next)
        if(!a_strcmp(title, plot->title))
            break;
    /* no plot found -> create one */
    if(!plot)
        plot = graph_plot_add(d, title);

    if((buf = luaA_getopt_string(L, 3, "fg", NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen, buf, &plot->color_start);
    if((buf = luaA_getopt_string(L, 3, "fg_center", NULL)))
        plot_pcolor_set(&plot->pcolor_center, buf);
    if((buf = luaA_getopt_string(L, 3, "fg_end", NULL)))
        plot_pcolor_set(&plot->pcolor_end, buf);

    plot->vertical_gradient = luaA_getopt_boolean(L, 3, "vertical_gradient", plot->vertical_gradient);
    plot->scale = luaA_getopt_boolean(L, 3, "scale", plot->scale);

    max_value = luaA_getopt_number(L, 3, "max_value", plot->max_value);
    if(max_value != plot->max_value)
        plot->max_value = plot->current_max = max_value;

    if((buf = luaA_getopt_lstring(L, 3, "style", NULL, &len)))
        switch (a_tokenize(buf, len))
        {
          case A_TK_BOTTOM:
            plot->draw_style = Bottom_Style;
            break;
          case A_TK_LINE:
            plot->draw_style = Line_Style;
            break;
          case A_TK_TOP:
            plot->draw_style = Top_Style;
            break;
          default:
            break;
        }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Add data to a plot.
 * \param l The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A widget.
 * \lparam A plot name.
 * \lparam A data value.
 */
static int
luaA_graph_plot_data_add(lua_State *L)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    graph_data_t *d = (*widget)->data;
    plot_t *plot;
    const char *title = luaL_checkstring(L, 2);
    float value;
    int i;

    for(plot = d->plots; plot; plot = plot->next)
        if(!a_strcmp(title, plot->title))
            break;

    /* no plot found -> create one */
    if(!plot)
        plot = graph_plot_add(d, title);

    /* assign incoming value */
    value = MAX(luaL_checknumber(L, 3), 0);

    if(++plot->index >= d->size) /* cycle inside the array */
        plot->index = 0;

    if(plot->scale) /* scale option is true */
    {
        plot->values[plot->index] = value;

        if(value > plot->current_max) /* a new maximum value found */
        {
            plot->max_index = plot->index;
            plot->current_max = value;

            /* recalculate */
            for (i = 0; i < d->size; i++)
                plot->lines[i] = round(plot->values[i] * d->box_height / plot->current_max);
        }
        /* old max_index reached + current_max > normal, re-check/generate */
        else if(plot->max_index == plot->index
                && plot->current_max > plot->max_value)
        {
            /* find the new max */
            for(i = 0; i < d->size; i++)
                if(plot->values[i] > plot->values[plot->max_index])
                    plot->max_index = i;

            plot->current_max = MAX(plot->values[plot->max_index], plot->max_value);

            /* recalculate */
            for(i = 0; i < d->size; i++)
                plot->lines[i] = round(plot->values[i] * d->box_height / plot->current_max);
        }
        else
            plot->lines[plot->index] = round(value * d->box_height / plot->current_max);
    }
    else /* scale option is false - limit to d->box_height */
    {
        if(value < plot->max_value)
            plot->lines[plot->index] = round(value * d->box_height / plot->max_value);
        else
            plot->lines[plot->index] = d->box_height;
    }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Index function for graph widget.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_graph_index(lua_State *L)
{
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_PROPERTIES_SET:
        lua_pushcfunction(L, luaA_graph_properties_set);
        return 1;
      case A_TK_PLOT_PROPERTIES_SET:
        lua_pushcfunction(L, luaA_graph_plot_properties_set);
        return 1;
      case A_TK_PLOT_DATA_ADD:
        lua_pushcfunction(L, luaA_graph_plot_data_add);
        return 1;
      default:
        return 0;
    }
}

/** Destroy definitively a graph widget.
 * \param widget Who slay.
 */
static void
graph_destructor(widget_t *widget)
{
    graph_data_t *d = widget->data;

    plot_list_wipe(&d->plots);
    p_delete(&d->draw_from);
    p_delete(&d->draw_to);
    p_delete(&d);
}

/** Create a brand new graph.
 * \param align The widget alignment.
 * \return A graph widget.
 */
widget_t *
graph_new(alignment_t align)
{
    widget_t *w;
    graph_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);

    w->draw = graph_draw;
    w->index = luaA_graph_index;
    w->destructor = graph_destructor;
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
