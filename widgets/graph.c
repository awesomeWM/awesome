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
#include "luaa.h"
#include "common/tokenize.h"

typedef enum
{
    Bottom_Style = 0,
    Top_Style,
    Line_Style
} plot_style_t;

/** The plot data structure. */
typedef struct
{
    /** Graph title of the plot sections */
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
    color_t color_start;
    /** Color at middle of graph */
    color_t pcolor_center;
    /** Color at end of graph */
    color_t pcolor_end;
    /** Create a vertical color gradient */
    bool vertical_gradient;
} plot_t;

static void
plot_delete(plot_t *g)
{
    p_delete(&g->title);
    p_delete(&g->lines);
    p_delete(&g->values);
}

DO_ARRAY(plot_t, plot, plot_delete)

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
    color_t bg;
    /** Border color */
    color_t border_color;
    /** Grow: Left or Right */
    position_t grow;
    /** Preparation/tmp array for draw_graph(); */
    int *draw_from;
    /** Preparation/tmp array for draw_graph(); */
    int *draw_to;
    /** Graph list */
    plot_array_t plots;
} graph_data_t;

/** Add a plot to a graph.
 * \param d The graph private data.
 * \param title The plot title.
 * \return A new plot.
 */
static plot_t *
graph_plot_add(graph_data_t *d, const char *title)
{
    plot_t plot;

    p_clear(&plot, 1);

    plot.title = a_strdup(title);
    plot.values = p_new(float, d->size);
    plot.lines = p_new(int, d->size);
    plot.max_value = 100.0;
    plot.vertical_gradient = true;

    xcolor_to_color(&globalconf.colors.fg, &plot.color_start);

    plot_array_append(&d->plots, plot);

    return &d->plots.tab[d->plots.len - 1];
}

/** Get the plot, and create one if it does not exist.
 * \param d The graph private data.
 * \param title The plot title.
 * \return A maybe new plot.
 */
static plot_t *
graph_plot_get(graph_data_t *d, const char *title)
{
    plot_t *plot;

    /* check if this section is defined already */
    for(int j = 0; j < d->plots.len; j++)
    {
        plot = &d->plots.tab[j];
        if(!a_strcmp(title, plot->title))
            return plot;
    }

    /* no plot found -> create one */
    return graph_plot_add(d, title);
}

static area_t
graph_geometry(widget_t *widget, int screen)
{
    area_t geometry;
    graph_data_t *d = widget->data;

    geometry.x = geometry.y = 0;
    geometry.height = d->width;
    geometry.width = d->width;

    return geometry;
}

static area_t
graph_extents(lua_State *L, widget_t *widget)
{
    return graph_geometry(widget, 0);
}

/** Draw a graph widget.
 * \param ctx The draw context.
 * \param w The widget node we are called from.
 * \param offset The offset to draw at.
 * \param used The already used width.
 * \param p A pointer to the object we're drawing onto.
 * \return The widget width.
 */
static void
graph_draw(widget_t *widget, draw_context_t *ctx,
           area_t geometry, wibox_t *p)
{
    int margin_top, y;
    graph_data_t *d = widget->data;
    area_t rectangle;
    vector_t color_gradient;

    if(!d->plots.len)
        return;

    /* box = the plot inside the rectangle */
    if(!d->box_height)
        d->box_height = round(ctx->height * d->height) - 2;

    margin_top = round((ctx->height - (d->box_height + 2)) / 2) + geometry.y;

    /* draw background */
    rectangle.x = geometry.x + 1;
    rectangle.y = margin_top + 1;
    rectangle.width = d->size;
    rectangle.height = d->box_height;
    draw_rectangle(ctx, rectangle, 1.0, true, &d->bg);

    /* for plot drawing */
    rectangle.y = margin_top + d->box_height + 1; /* bottom left corner as starting point */
    rectangle.width = d->size; /* rectangle.height is not used */

    draw_graph_setup(ctx); /* setup some drawing options */

    /* gradient begin either left or on the right of the rectangle */
    if(d->grow == Right)
        color_gradient.x = rectangle.x + rectangle.width;
    else
        color_gradient.x = rectangle.x;

    for(int i = 0; i < d->plots.len; i++)
    {
        plot_t *plot = &d->plots.tab[i];

        switch(plot->draw_style)
        {
            case Top_Style:
              color_gradient.y = rectangle.y - rectangle.height;
              if(plot->vertical_gradient)
              {
                  color_gradient.x_offset = 0;
                  color_gradient.y_offset = rectangle.height;
              }
              else
              {
                  color_gradient.y_offset = 0;

                  if(d->grow == Right)
                      color_gradient.x_offset = - rectangle.width;
                  else
                      color_gradient.x_offset = rectangle.width;
              }

              for(y = 0; y < d->size; y++)
              {
                  /* reverse values (because drawing from top) */
                  d->draw_from[y] = d->box_height; /* i.e. no smaller value -> from top of box */
                  d->draw_to[y] = d->box_height - plot->lines[y]; /* i.e. on full plot -> 0 = bottom */
              }
              draw_graph(ctx, rectangle , d->draw_from, d->draw_to, plot->index, d->grow, color_gradient,
                         &plot->color_start, &plot->pcolor_center, &plot->pcolor_end);
              break;
            case Bottom_Style:
              color_gradient.y = rectangle.y;
              if(plot->vertical_gradient)
              {
                  color_gradient.x_offset = 0;
                  color_gradient.y_offset = - rectangle.height;
              }
              else
              {
                  color_gradient.y_offset = 0;

                  if(d->grow == Right)
                      color_gradient.x_offset = - rectangle.width;
                  else
                      color_gradient.x_offset = rectangle.width;
              }

              p_clear(d->draw_from, d->size);
              draw_graph(ctx, rectangle, d->draw_from, plot->lines, plot->index, d->grow, color_gradient,
                         &plot->color_start, &plot->pcolor_center, &plot->pcolor_end);
              break;
            case Line_Style:
              color_gradient.y = rectangle.y;
              if(plot->vertical_gradient)
              {
                  color_gradient.x_offset = 0;
                  color_gradient.y_offset = -rectangle.height;
              }
              else
              {
                  color_gradient.y_offset = 0;
                  if(d->grow == Right)
                      color_gradient.x_offset = - rectangle.width;
                  else
                      color_gradient.x_offset = rectangle.width;
              }

              draw_graph_line(ctx, rectangle, plot->lines, plot->index, d->grow, color_gradient,
                              &plot->color_start, &plot->pcolor_center, &plot->pcolor_end);
              break;
        }
    }

    /* draw border (after line-drawing, what paints 0-values to the border) */
    rectangle.x = geometry.x;
    rectangle.y = margin_top;
    rectangle.width = d->size + 2;
    rectangle.height = d->box_height + 2;
    draw_rectangle(ctx, rectangle, 1.0, false, &d->border_color);
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
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    graph_data_t *d = widget->data;
    float max_value;
    const char *title, *buf;
    size_t len;
    plot_t *plot = NULL;
    color_init_cookie_t reqs[3];
    int i, reqs_nbr = -1;

    title = luaL_checkstring(L, 2);
    luaA_checktable(L, 3);

    plot = graph_plot_get(d, title);

    if((buf = luaA_getopt_lstring(L, 3, "fg", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&plot->color_start, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "fg_center", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&plot->pcolor_center, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "fg_end", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&plot->pcolor_end, buf, len);

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

    for(i = 0; i <= reqs_nbr; i++)
        color_init_reply(reqs[i]);

    widget_invalidate_bywidget(widget);

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
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    graph_data_t *d = widget->data;
    plot_t *plot = NULL;
    const char *title = luaL_checkstring(L, 2);
    float value;
    int i;

    if(!d->size)
        return 0;

    plot = graph_plot_get(d, title);

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

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Graph widget.
 * DEPRECATED, see awful.widget.graph.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield plot_properties_set A function to set plot properties.
 * \lfield plot_data_add A function to add data to a plot.
 * \lfield height Graph height.
 * \lfield widget Graph width.
 * \lfield bg Background color.
 * \lfield grow Direction to grow: left or right.
 */
static int
luaA_graph_index(lua_State *L, awesome_token_t token)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    graph_data_t *d = widget->data;

    switch(token)
    {
      case A_TK_PLOT_PROPERTIES_SET:
        lua_pushcfunction(L, luaA_graph_plot_properties_set);
        break;
      case A_TK_PLOT_DATA_ADD:
        lua_pushcfunction(L, luaA_graph_plot_data_add);
        break;
      case A_TK_HEIGHT:
        lua_pushnumber(L, d->height);
        break;
      case A_TK_WIDTH:
        lua_pushnumber(L, d->width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &d->border_color);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &d->bg);
        break;
      case A_TK_GROW:
        switch(d->grow)
        {
          case Left:
            lua_pushliteral(L, "left");
            break;
          case Right:
            lua_pushliteral(L, "right");
            break;
          default:
            return 0;
        }
        break;
      default:
        return 0;
    }

    return 1;
}

/** Newindex function for graph widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 */
static int
luaA_graph_newindex(lua_State *L, awesome_token_t token)
{
    size_t len;
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    graph_data_t *d = widget->data;
    const char *buf;
    int width;
    position_t pos;
    color_t color;

    switch(token)
    {
      case A_TK_HEIGHT:
        d->height = luaL_checknumber(L, 3);
        break;
      case A_TK_WIDTH:
        width = luaL_checknumber(L, 3);
        if(width >= 2 && width != d->width)
        {
            d->width = width;
            d->size = d->width - 2;
            p_realloc(&d->draw_from, d->size);
            p_realloc(&d->draw_to, d->size);
            for(int i = 0; i < d->plots.len; i++)
            {
                plot_t *plot = &d->plots.tab[i];
                p_realloc(&plot->values, d->size);
                p_realloc(&plot->lines, d->size);
                p_clear(plot->values, d->size);
                p_clear(plot->lines, d->size);
                plot->index = 0;
                plot->current_max =  0;
                plot->max_index =  0;
            }
        }
        else
            return 0;
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
        {
            if(color_init_reply(color_init_unchecked(&color, buf, len)))
                d->bg = color;
            else
                return 0;
        }
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
        {
            if(color_init_reply(color_init_unchecked(&color, buf, len)))
                d->border_color = color;
            else
                return 0;
        }
        break;
      case A_TK_GROW:
        buf = luaL_checklstring(L, 3, &len);
        switch((pos = position_fromstr(buf, len)))
        {
          case Left:
          case Right:
            d->grow = pos;
            break;
          default:
            return 0;
        }
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Destroy definitively a graph widget.
 * \param widget Who slay.
 */
static void
graph_destructor(widget_t *widget)
{
    graph_data_t *d = widget->data;

    plot_array_wipe(&d->plots);
    p_delete(&d->draw_from);
    p_delete(&d->draw_to);
    p_delete(&d);
}

/** Create a brand new graph.
 * \param w The widget to initialize.
 * \return The same widget.
 */
widget_t *
widget_graph(widget_t *w)
{
    luaA_deprecate(globalconf.L, "awful.widget.graph");
    w->draw = graph_draw;
    w->index = luaA_graph_index;
    w->newindex = luaA_graph_newindex;
    w->destructor = graph_destructor;
    w->extents = graph_extents;

    graph_data_t *d = w->data = p_new(graph_data_t, 1);

    d->width = 80;
    d->height = 0.80;
    d->size = d->width - 2;
    d->grow = Left;
    d->draw_from = p_new(int, d->size);
    d->draw_to = p_new(int, d->size);

    xcolor_to_color(&globalconf.colors.bg, &d->bg);
    xcolor_to_color(&globalconf.colors.fg, &d->border_color);

    return w;
}

/* This is used for building documentation. */
static const struct luaL_reg awesome_graph_meta[] __attribute__ ((unused)) =
{
    { "plot_properties_set", luaA_graph_plot_properties_set },
    { "plot_data_add", luaA_graph_plot_data_add },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
