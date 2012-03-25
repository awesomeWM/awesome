/*
 * progressbar.c - progressbar widget
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
#include "luaa.h"
#include "common/tokenize.h"

/** Progressbar bar data structure */
typedef struct
{
    /** Title of the data/bar */
    char *title;
    /** These or lower values won't fill the bar at all*/
    float min_value;
    /** These or higher values fill the bar fully */
    float max_value;
    /** Pointer to value */
    float value;
    /** Reverse filling */
    bool reverse;
    /** Foreground color */
    color_t fg;
    /** Foreground color of turned-off ticks */
    color_t fg_off;
    /** Foreground color when bar is half-full */
    color_t fg_center;
    /** Foreground color when bar is full */
    color_t fg_end;
    /** Background color */
    color_t bg;
    /** Border color */
    color_t border_color;
} bar_t;

/** Delete a bar.
 * \param bar The bar to annihilate.
 */
static void
bar_delete(bar_t *bar)
{
    p_delete(&bar->title);
}

DO_ARRAY(bar_t, bar, bar_delete)

/** Progressbar private data structure */
typedef struct
{
    /** Width of the data_items */
    int width;
    /** Pixel between data items (bars) */
    int gap;
    /** Border width in pixels */
    int border_width;
    /** Padding between border and ticks/bar */
    int border_padding;
    /** Gap/distance between the individual ticks */
    int ticks_gap;
    /** Total number of ticks */
    int ticks_count;
    /** 90 Degree's turned */
    bool vertical;
    /** Height 0-1, where 1.0 is height of bar */
    float height;
    /** The bars */
    bar_array_t bars;
} progressbar_data_t;

/** Add a new bar to the progressbar private data structure.
 * \param bars The bar array.
 * \param title The bar title.
 * \return The new bar.
 */
static bar_t *
progressbar_bar_add(bar_array_t *bars, const char *title)
{
    bar_t bar;

    p_clear(&bar, 1);

    bar.title = a_strdup(title);
    bar.max_value = 100.0;

    xcolor_to_color(&globalconf.colors.fg, &bar.fg);
    xcolor_to_color(&globalconf.colors.bg, &bar.bg);
    xcolor_to_color(&globalconf.colors.bg, &bar.fg_off);
    xcolor_to_color(&globalconf.colors.fg, &bar.border_color);

    /* append the bar in the list */
    bar_array_append(bars, bar);

    return &bars->tab[bars->len - 1];
}

/** Get the bar, and create one if it does not exist.
 * \param bars The bar array.
 * \param title The bar title.
 * \return A maybe new bar.
 */
static bar_t *
progressbar_bar_get(bar_array_t *bars, const char *title)
{
    bar_t *bar;

    /* check if this section is defined already */
    for(int j = 0; j < bars->len; j++)
    {
        bar = &bars->tab[j];
        if(!a_strcmp(title, bar->title))
            return bar;
    }

    /* no bar found -> create one */
    return progressbar_bar_add(bars, title);
}

static area_t
progressbar_geometry(widget_t *widget, int screen)
{
    area_t geometry;
    progressbar_data_t *d = widget->data;


    if(d->vertical)
    {
        int pb_width = (int) ((d->width - 2 * (d->border_width + d->border_padding) * d->bars.len
                       - d->gap * (d->bars.len - 1)) / d->bars.len);
        geometry.width = d->bars.len * (pb_width + 2 * (d->border_width + d->border_padding)
                         + d->gap) - d->gap;
    }
    else
    {
        int pb_width = d->width - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            int unit = (pb_width + d->ticks_gap) / d->ticks_count;
            pb_width = unit * d->ticks_count - d->ticks_gap; /* rounded to match ticks... */
        }
        geometry.width = pb_width + 2 * (d->border_width + d->border_padding);
    }
    geometry.height = geometry.width;

    geometry.x = geometry.y = 0;

    return geometry;
}

static area_t
progressbar_extents(lua_State *L, widget_t *widget)
{
    return progressbar_geometry(widget, 0);
}

/** Draw a progressbar.
 * \param ctx The draw context.
 * \param w The widget node we're drawing for.
 * \param offset Offset to draw at.
 * \param used Space already used.
 * \param object The object pointer we're drawing onto.
 * \return The width used.
 */
static void
progressbar_draw(widget_t *widget, draw_context_t *ctx, area_t geometry, wibox_t *p)
{
    /* pb_.. values points to the widget inside a potential border */
    int values_ticks, pb_x, pb_y, pb_height, pb_width, pb_progress, pb_offset;
    int unit = 0; /* tick + gap */
    area_t rectangle;
    vector_t color_gradient;
    progressbar_data_t *d = widget->data;

    if(!d->bars.len)
        return;

    /* for a 'reversed' progressbar:
     * basic progressbar:
     * 1. the full space gets the size of the formerly empty one
     * 2. the pattern must be mirrored
     * 3. the formerly 'empty' side is drawn with fg colors, the 'full' with bg-color
     *
     * ticks:
     * 1. round the values to a full tick accordingly
     * 2. finally draw the gaps
     */

    pb_x = geometry.x + d->border_width + d->border_padding;
    pb_offset = 0;

    if(d->vertical)
    {
        pb_width = (int) ((d->width - 2 * (d->border_width + d->border_padding) * d->bars.len
                   - d->gap * (d->bars.len - 1)) / d->bars.len);

        /** \todo maybe prevent to calculate that stuff below over and over again
         * (->use static-values) */
        pb_height = (int) (ctx->height * d->height + 0.5)
                    - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            /* '+ d->ticks_gap' because a unit includes a ticks + ticks_gap */
            unit = (pb_height + d->ticks_gap) / d->ticks_count;
            pb_height = unit * d->ticks_count - d->ticks_gap;
        }

        pb_y = geometry.y + ((int) (ctx->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

        for(int i = 0; i < d->bars.len; i++)
        {
            bar_t *bar = &d->bars.tab[i];

            if(d->ticks_count && d->ticks_gap)
            {
                values_ticks = (int)(d->ticks_count * (bar->value - bar->min_value)
                               / (bar->max_value - bar->min_value) + 0.5);
                if(values_ticks)
                    pb_progress = values_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                /* e.g.: min = 50; max = 56; 53 should show 50% graph
                 * (53(val) - 50(min) / (56(max) - 50(min) = 3 / 5 = 0.5 = 50%
                 * round that ( + 0.5 and (int)) and finally multiply with height
                 */
                pb_progress = (int) (pb_height * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x + pb_offset - d->border_width - d->border_padding;
                rectangle.y = pb_y - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->bg);
                draw_rectangle(ctx, rectangle, d->border_width, false, &bar->border_color);
            }

            color_gradient.x = pb_x;
            color_gradient.x_offset =  0;
            color_gradient.y = pb_y;

            /* new value/progress in px + pattern setup */
            if(bar->reverse)
            {
                /* invert: top with bottom part */
                pb_progress = pb_height - pb_progress;
                color_gradient.y_offset = pb_height;
            }
            else
            {
                /* bottom to top */
                color_gradient.y += pb_height;
                color_gradient.y_offset = - pb_height;
            }

            /* bottom part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y + pb_height - pb_progress;
                rectangle.width = pb_width;
                rectangle.height = pb_progress;

                /* fg color */
                if(bar->reverse)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->fg_off);
                else
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, color_gradient,
                                            &bar->fg, &bar->fg_center, &bar->fg_end);
            }

            /* top part */
            if(pb_height - pb_progress > 0) /* not filled area */
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y;
                rectangle.width = pb_width;
                rectangle.height = pb_height - pb_progress;

                /* bg color */
                if(bar->reverse)
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, color_gradient,
                                            &bar->fg, &bar->fg_center, &bar->fg_end);
                else
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->fg_off);
            }
            /* draw gaps \todo improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = pb_width;
                rectangle.height = d->ticks_gap;
                rectangle.x = pb_x + pb_offset;
                for(rectangle.y = pb_y + (unit - d->ticks_gap);
                        pb_y + pb_height - d->ticks_gap >= rectangle.y;
                        rectangle.y += unit)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->bg);
            }
            pb_offset += pb_width + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }
    else /* a horizontal progressbar */
    {
        pb_width = d->width - 2 * (d->border_width + d->border_padding);

        if(d->ticks_count && d->ticks_gap)
        {
            unit = (pb_width + d->ticks_gap) / d->ticks_count;
            pb_width = unit * d->ticks_count - d->ticks_gap; /* rounded to match ticks... */
        }

        pb_height = (int) ((ctx->height * d->height
                            - d->bars.len * 2 * (d->border_width + d->border_padding)
                            - (d->gap * (d->bars.len - 1))) / d->bars.len + 0.5);
        pb_y = geometry.y + ((int) (ctx->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

        for(int i = 0; i < d->bars.len; i++)
        {
            bar_t *bar = &d->bars.tab[i];

            if(d->ticks_count && d->ticks_gap)
            {
                /* +0.5 rounds up ticks -> turn on a tick when half of it is reached */
                values_ticks = (int)(d->ticks_count * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);
                if(values_ticks)
                    pb_progress = values_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                pb_progress = (int) (pb_width * (bar->value - bar->min_value)
                                     / (bar->max_value - bar->min_value) + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x - d->border_width - d->border_padding;
                rectangle.y = pb_y + pb_offset - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->bg);
                draw_rectangle(ctx, rectangle, d->border_width, false, &bar->border_color);
            }

            color_gradient.y = pb_y;
            color_gradient.y_offset = 0;
            color_gradient.x = pb_x;

            /* new value/progress in px + pattern setup */
            if(bar->reverse)
            {
                /* reverse: right to left */
                pb_progress = pb_width - pb_progress;
                color_gradient.x += pb_width;
                color_gradient.x_offset = - pb_width;
            }
            else
                /* left to right */
                color_gradient.x_offset = pb_width;

            /* left part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x;
                rectangle.y = pb_y + pb_offset;
                rectangle.width = pb_progress;
                rectangle.height = pb_height;

                /* fg color */
                if(bar->reverse)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->fg_off);
                else
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, color_gradient,
                                            &bar->fg, &bar->fg_center, &bar->fg_end);
            }

            /* right part */
            if(pb_width - pb_progress > 0)
            {
                rectangle.x = pb_x + pb_progress;
                rectangle.y = pb_y +  pb_offset;
                rectangle.width = pb_width - pb_progress;
                rectangle.height = pb_height;

                /* bg color */
                if(bar->reverse)
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, color_gradient,
                                            &bar->fg, &bar->fg_center, &bar->fg_end);
                else
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->fg_off);
            }
            /* draw gaps \todo improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = d->ticks_gap;
                rectangle.height = pb_height;
                rectangle.y = pb_y + pb_offset;
                for(rectangle.x = pb_x + (unit - d->ticks_gap);
                        pb_x + pb_width - d->ticks_gap >= rectangle.x;
                        rectangle.x += unit)
                    draw_rectangle(ctx, rectangle, 1.0, true, &bar->bg);
            }

            pb_offset += pb_height + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }
}

/** Set various progressbar bars properties:
 * \param L The Lua VM state.
 * \return The number of elements pushed on the stack.
 * \luastack
 * \lvalue A widget.
 * \lparam A bar name.
 * \lparam A table with keys as properties names.
 */
static int
luaA_progressbar_bar_properties_set(lua_State *L)
{
    size_t len;
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    const char *buf, *title = luaL_checkstring(L, 2);
    bar_t *bar;
    progressbar_data_t *d = widget->data;
    color_init_cookie_t reqs[6];
    int i, reqs_nbr = -1;

    luaA_checktable(L, 3);

    bar = progressbar_bar_get(&d->bars, title);

    if((buf = luaA_getopt_lstring(L, 3, "fg", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->fg, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "fg_off", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->fg_off, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "bg", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->bg, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "border_color", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->border_color, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "fg_center", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->fg_center, buf, len);

    if((buf = luaA_getopt_lstring(L, 3, "fg_end", NULL, &len)))
        reqs[++reqs_nbr] = color_init_unchecked(&bar->fg_end, buf, len);

    bar->min_value = luaA_getopt_number(L, 3, "min_value", bar->min_value);
    /* hack to prevent max_value beeing less than min_value
     * and also preventing a division by zero when both are equal */
    if(bar->max_value <= bar->min_value)
        bar->max_value = bar->max_value + 0.0001;
    /* force a actual value into the newly possible range */
    if(bar->value < bar->min_value)
        bar->value = bar->min_value;

    bar->max_value = luaA_getopt_number(L, 3, "max_value", bar->max_value);
    if(bar->min_value >= bar->max_value)
        bar->min_value = bar->max_value - 0.0001;
    if(bar->value > bar->max_value)
        bar->value = bar->max_value;

    bar->reverse = luaA_getopt_boolean(L, 3, "reverse", bar->reverse);

    for(i = 0; i <= reqs_nbr; i++)
        color_init_reply(reqs[i]);

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Add a value to a progressbar bar.
 * \param L The Lua VM state.
 * \return The number of elements pushed on the stack.
 * \luastack
 * \lvalue A widget.
 * \lparam A bar name.
 * \lparam A data value.
 */
static int
luaA_progressbar_bar_data_add(lua_State *L)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    const char *title = luaL_checkstring(L, 2);
    progressbar_data_t *d = widget->data;
    bar_t *bar;

    bar = progressbar_bar_get(&d->bars, title);

    bar->value = luaL_checknumber(L, 3);
    bar->value = MAX(bar->min_value, MIN(bar->max_value, bar->value));

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Progressbar widget.
 * DEPRECATED, see awful.widget.progressbar.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on the stack.
 * \luastack
 * \lfield bar_properties_set Set the properties of a bar.
 * \lfield bar_data_add Add data to a bar.
 * \lfield gap Gap betweens bars.
 * \lfield ticks_gap Gap between ticks.
 * \lfield ticks_count Number of ticks.
 * \lfield border_padding Border padding.
 * \lfield border_width Border width.
 * \lfield width Bars width.
 * \lfield height Bars height.
 * \lfield vertical True: draw bar vertically, false: horizontally.
 */
static int
luaA_progressbar_index(lua_State *L, awesome_token_t token)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    progressbar_data_t *d = widget->data;

    switch(token)
    {
      case A_TK_BAR_PROPERTIES_SET:
        lua_pushcfunction(L, luaA_progressbar_bar_properties_set);
        break;
      case A_TK_BAR_DATA_ADD:
        lua_pushcfunction(L, luaA_progressbar_bar_data_add);
        break;
      case A_TK_GAP:
        lua_pushnumber(L, d->gap);
        break;
      case A_TK_TICKS_GAP:
        lua_pushnumber(L, d->ticks_gap);
        break;
      case A_TK_TICKS_COUNT:
        lua_pushnumber(L, d->ticks_count);
        break;
      case A_TK_BORDER_PADDING:
        lua_pushnumber(L, d->border_padding);
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, d->border_width);
        break;
      case A_TK_WIDTH:
        lua_pushnumber(L, d->width);
        break;
      case A_TK_HEIGHT:
        lua_pushnumber(L, d->height);
        break;
      case A_TK_VERTICAL:
        lua_pushboolean(L, d->vertical);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Newindex function for progressbar.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on the stack.
 */
static int
luaA_progressbar_newindex(lua_State *L, awesome_token_t token)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    progressbar_data_t *d = widget->data;

    switch(token)
    {
      case A_TK_GAP:
        d->gap = luaL_checknumber(L, 3);
        break;
      case A_TK_TICKS_COUNT:
        d->ticks_count = luaL_checknumber(L, 3);
        break;
      case A_TK_TICKS_GAP:
        d->ticks_gap = luaL_checknumber(L, 3);
        break;
      case A_TK_BORDER_PADDING:
        d->border_padding = luaL_checknumber(L, 3);
        break;
      case A_TK_BORDER_WIDTH:
        d->border_width = luaL_checknumber(L, 3);
        break;
      case A_TK_WIDTH:
        d->width = luaL_checknumber(L, 3);
        break;
      case A_TK_HEIGHT:
        d->height = luaL_checknumber(L, 3);
        break;
      case A_TK_VERTICAL:
        d->vertical = luaA_checkboolean(L, 3);
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Destroy a progressbar.
 * \param widget The widget to kill.
 */
static void
progressbar_destructor(widget_t *widget)
{
    progressbar_data_t *d = widget->data;

    bar_array_wipe(&d->bars);
    p_delete(&d);
}

/** Create a new progressbar.
 * \param w The widget to initialize.
 * \return A brand new progressbar.
 */
widget_t *
widget_progressbar(widget_t *w)
{
    luaA_deprecate(globalconf.L, "awful.widget.progressbar");
    w->draw = progressbar_draw;
    w->index = luaA_progressbar_index;
    w->newindex = luaA_progressbar_newindex;
    w->destructor = progressbar_destructor;
    w->extents = progressbar_extents;

    progressbar_data_t *d = w->data = p_new(progressbar_data_t, 1);

    d->height = 0.80;
    d->width = 80;

    d->ticks_gap = 1;
    d->border_width = 1;
    d->gap = 2;

    return w;
}

/* This is used for building documentation. */
static const struct luaL_reg awesome_progressbar_meta[] __attribute__ ((unused)) =
{
    { "bar_properties_set", luaA_progressbar_bar_properties_set },
    { "bar_data_add", luaA_progressbar_bar_data_add },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
