/*
 * widget.c - widget managing
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "screen.h"
#include "mouse.h"
#include "widget.h"
#include "wibox.h"
#include "client.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/xutil.h"

LUA_OBJECT_FUNCS(widget_class, widget_t, widget);

static void
widget_wipe(widget_t *widget)
{
    if(widget->destructor)
        widget->destructor(widget);
    button_array_wipe(&widget->buttons);
}

/** Get a widget node from a wibox by coords.
 * \param orientation Wibox orientation.
 * \param widgets The widget list.
 * \param width The container width.
 * \param height The container height.
 * \param x X coordinate of the widget.
 * \param y Y coordinate of the widget.
 * \return A widget.
 */
widget_t *
widget_getbycoords(orientation_t orientation, widget_node_array_t *widgets,
                   int width, int height, int16_t *x, int16_t *y)
{
    int tmp;

    /* Need to transform coordinates like it was top/bottom */
    switch(orientation)
    {
      case South:
        tmp = *y;
        *y = width - *x;
        *x = tmp;
        break;
      case North:
        tmp = *y;
        *y = *x;
        *x = height - tmp;
        break;
      default:
        break;
    }
    foreach(w, *widgets)
        if(w->widget->isvisible
           && *x >= w->geometry.x && *x < w->geometry.x + w->geometry.width
           && *y >= w->geometry.y && *y < w->geometry.y + w->geometry.height)
            return w->widget;

    return NULL;
}

/** Convert a Lua table to a list of widget nodes.
 * \param L The Lua VM state.
 * \param idx Index of the table where to store the widgets references.
 * \param widgets The linked list of widget node.
 */
static void
luaA_table2widgets(lua_State *L, int idx, widget_node_array_t *widgets)
{
    if(lua_istable(L, -1))
    {
        int table_idx = luaA_absindex(L, idx);
        int i = 1;
        lua_pushnumber(L, i++);
        lua_gettable(L, -2);
        while(!lua_isnil(L, -1))
        {
            luaA_table2widgets(L, table_idx, widgets);
            lua_pushnumber(L, i++);
            lua_gettable(L, -2);
        }

        /* remove the nil and the table */
        lua_pop(L, 2);
    }
    else
    {
        widget_t *widget = luaA_toudata(L, -1, &widget_class);
        if(widget)
        {
            widget_node_t w;
            p_clear(&w, 1);
            w.widget = luaA_object_ref_item(L, idx, -1);
            widget_node_array_append(widgets, w);
        }
        else
            lua_pop(L, 1); /* remove value */
    }
}

/** Retrieve a list of widget geometries using a Lua layout function.
 * a table which contains the geometries is then pushed onto the stack
 * \param wibox The wibox.
 * \return True is everything is ok, false otherwise.
 */
static bool
widget_geometries(wibox_t *wibox)
{
    /* get the layout field of the widget table */
    if(wibox->widgets_table)
    {
        /* push wibox */
        luaA_object_push(globalconf.L, wibox);
        /* push widgets table */
        luaA_object_push_item(globalconf.L, -1, wibox->widgets_table);
        /* remove wibox */
        lua_remove(globalconf.L, -2);
        /* get layout field from the table */
        lua_getfield(globalconf.L, -1, "layout");
        /* remove the widget table */
        lua_remove(globalconf.L, -2);
    }
    else
        lua_pushnil(globalconf.L);

    /* if the layout field is a function */
    if(lua_isfunction(globalconf.L, -1))
    {
        /* Push 1st argument: wibox geometry */
        area_t geometry = wibox->geometry;
        geometry.x = 0;
        geometry.y = 0;
        /* we need to exchange the width and height of the wibox window if it
         * it is rotated, so the layout function doesn't need to care about that
         */
        if(wibox->orientation != East)
        {
            int i = geometry.height;
            geometry.height = geometry.width;
            geometry.width = i;
        }
        luaA_pusharea(globalconf.L, geometry);
        /* Push 2nd argument: widget table */
        luaA_object_push(globalconf.L, wibox);
        luaA_object_push_item(globalconf.L, -1, wibox->widgets_table);
        lua_remove(globalconf.L, -2);
        /* Push 3rd argument: wibox screen */
        lua_pushnumber(globalconf.L, screen_array_indexof(&globalconf.screens, wibox->screen) + 1);
        /* Re-push the layout function */
        lua_pushvalue(globalconf.L, -4);
        /* call the layout function with 3 arguments (wibox geometry, widget
         * table, screen) and wait for one result */
        if(!luaA_dofunction(globalconf.L, 3, 1))
            return false;

        /* Remove the left over layout function */
        lua_remove(globalconf.L, -2);
    }
    else
    {
        /* Remove the "nil function" */
        lua_pop(globalconf.L, 1);

        /* If no layout function has been specified, we just push a table with
         * geometries onto the stack. These geometries are nothing fancy, they
         * have x = y = 0 and their height and width set to the widgets demands
         * or the wibox size, depending on which is less.
         */

        /* push wibox */
        luaA_object_push(globalconf.L, wibox);

        widget_node_array_t *widgets = &wibox->widgets;
        wibox_widget_node_array_wipe(globalconf.L, -1);
        widget_node_array_init(widgets);

        /* push widgets table */
        luaA_object_push_item(globalconf.L, -1, wibox->widgets_table);
        /* Convert widgets table */
        luaA_table2widgets(globalconf.L, -2, widgets);
        /* remove wibox */
        lua_remove(globalconf.L, -1);

        lua_newtable(globalconf.L);
        for(int i = 0; i < widgets->len; i++)
        {
            lua_pushnumber(globalconf.L, i + 1);
            widget_t *widget = widgets->tab[i].widget;
            lua_pushnumber(globalconf.L, screen_array_indexof(&globalconf.screens, wibox->screen) + 1);
            area_t geometry = widget->extents(globalconf.L, widget);
            lua_pop(globalconf.L, 1);
            geometry.x = geometry.y = 0;
            geometry.width = MIN(wibox->geometry.width, geometry.width);
            geometry.height = MIN(wibox->geometry.height, geometry.height);

            luaA_pusharea(globalconf.L, geometry);

            lua_settable(globalconf.L, -3);
        }
    }
    return true;
}

/** Render a list of widgets.
 * \param wibox The wibox.
 * \todo Remove GC.
 */
void
widget_render(wibox_t *wibox)
{
    lua_State *L = globalconf.L;
    draw_context_t *ctx = &wibox->ctx;
    area_t rectangle = { 0, 0, 0, 0 };
    color_t col;

    rectangle.width = ctx->width;
    rectangle.height = ctx->height;

    if (!widget_geometries(wibox))
        return;

    if(ctx->bg.alpha != 0xffff)
    {
        int x = wibox->geometry.x + wibox->border_width,
            y = wibox->geometry.y + wibox->border_width;
        xcb_get_property_reply_t *prop_r;
        char *data;
        xcb_pixmap_t rootpix;
        xcb_get_property_cookie_t prop_c;
        xcb_screen_t *s = globalconf.screen;
        prop_c = xcb_get_property_unchecked(globalconf.connection, false, s->root, _XROOTPMAP_ID,
                                            XCB_ATOM_PIXMAP, 0, 1);
        if((prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)))
        {
            if(prop_r->value_len
               && (data = xcb_get_property_value(prop_r))
               && (rootpix = *(xcb_pixmap_t *) data))
               switch(wibox->orientation)
               {
                 case North:
                   draw_rotate(ctx,
                               rootpix, ctx->pixmap,
                               s->width_in_pixels, s->height_in_pixels,
                               ctx->width, ctx->height,
                               M_PI_2,
                               y + ctx->width,
                               - x);
                   break;
                 case South:
                   draw_rotate(ctx,
                               rootpix, ctx->pixmap,
                               s->width_in_pixels, s->height_in_pixels,
                               ctx->width, ctx->height,
                               - M_PI_2,
                               - y,
                               x + ctx->height);
                   break;
                 case East:
                   xcb_copy_area(globalconf.connection, rootpix,
                                 wibox->pixmap, globalconf.gc,
                                 x, y,
                                 0, 0,
                                 ctx->width, ctx->height);
                   break;
               }
            p_delete(&prop_r);
        }
    }

    widget_node_array_t *widgets = &wibox->widgets;

    /* push wibox */
    luaA_object_push(globalconf.L, wibox);

    wibox_widget_node_array_wipe(globalconf.L, -1);
    widget_node_array_init(widgets);

    /* push widgets table */
    luaA_object_push_item(globalconf.L, -1, wibox->widgets_table);
    luaA_table2widgets(L, -2, widgets);
    /* remove wibox */
    lua_remove(globalconf.L, -1);

    size_t num_widgets = lua_objlen(L, -1);
    if(lua_objlen(L, -1) != (size_t) widgets->len)
    {
        warn("Widget layout returned wrong number of geometries. Got %d widgets and %lu geometries.",
                widgets->len, (unsigned long) lua_objlen(L, -1));
        /* Set num_widgets to min(widgets->len, lua_objlen(L, -1)); */
        if(num_widgets > (size_t) widgets->len)
            num_widgets = widgets->len;
    }

    /* get computed geometries */
    for(size_t i = 0; i < num_widgets; i++)
    {
        lua_pushnumber(L, i + 1);
        lua_gettable(L, -2);

        widgets->tab[i].geometry.x = luaA_getopt_number(L, -1, "x", wibox->geometry.x);
        widgets->tab[i].geometry.y = luaA_getopt_number(L, -1, "y", wibox->geometry.y);
        widgets->tab[i].geometry.width = luaA_getopt_number(L, -1, "width", 1);
        widgets->tab[i].geometry.height = luaA_getopt_number(L, -1, "height", 1);

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    /* draw background image, only if the background color is not opaque */
    if(wibox->bg_image && ctx->bg.alpha != 0xffff)
        draw_surface(ctx, 0, 0, 1.0, wibox->bg_image);

    /* draw background color */
    xcolor_to_color(&ctx->bg, &col);
    draw_rectangle(ctx, rectangle, 1.0, true, &col);

    /* draw everything! */
    for(int i = 0; i < widgets->len; i++)
        if(widgets->tab[i].widget->isvisible)
            widgets->tab[i].widget->draw(widgets->tab[i].widget,
                                         ctx, widgets->tab[i].geometry, wibox);

    switch(wibox->orientation)
    {
        case South:
          draw_rotate(ctx, ctx->pixmap, wibox->pixmap,
                      ctx->width, ctx->height,
                      ctx->height, ctx->width,
                      M_PI_2, ctx->height, 0);
          break;
        case North:
          draw_rotate(ctx, ctx->pixmap, wibox->pixmap,
                      ctx->width, ctx->height,
                      ctx->height, ctx->width,
                      - M_PI_2, 0, ctx->width);
          break;
        case East:
          break;
    }
}

/** Invalidate widgets which should be refresh depending on their types.
 * \param type Widget type to invalidate.
 */
void
widget_invalidate_bytype(widget_constructor_t *type)
{
    foreach(wibox, globalconf.wiboxes)
        foreach(wnode, (*wibox)->widgets)
            if(wnode->widget->type == type)
            {
                (*wibox)->need_update = true;
                break;
            }
}

/** Set a wibox needs update because it has widget.
 * \param widget The widget to look for.
 */
void
widget_invalidate_bywidget(widget_t *widget)
{
    foreach(wibox, globalconf.wiboxes)
        if(!(*wibox)->need_update)
            foreach(wnode, (*wibox)->widgets)
                if(wnode->widget == widget)
                {
                    (*wibox)->need_update = true;
                    break;
                }
}

/** Create a new widget.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with at least a type value.
 * \lreturn A brand new widget.
 */
static int
luaA_widget_new(lua_State *L)
{
    luaA_class_new(L, &widget_class);

    widget_t *w = luaA_checkudata(L, -1, &widget_class);
    /* Set visible by default. */
    w->isvisible = true;

    return 1;
}

/** Get or set mouse buttons bindings to a widget.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A widget.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects of this widget.
 */
static int
luaA_widget_buttons(lua_State *L)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);

    if(lua_gettop(L) == 2)
    {
        luaA_button_array_set(L, 1, 2, &widget->buttons);
        luaA_object_emit_signal(L, 1, "property::buttons", 0);
        return 1;
    }

    return luaA_button_array_get(L, 1, &widget->buttons);
}

/** Generic widget.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield visible The widget visibility.
 * \lfield mouse_enter A function to execute when the mouse enter the widget.
 * \lfield mouse_leave A function to execute when the mouse leave the widget.
 */
static int
luaA_widget_index(lua_State *L)
{
    const char *prop = luaL_checkstring(L, 2);

    /* Try standard method */
    if(luaA_class_index(L))
        return 1;

    /* Then call special widget index */
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    return widget->index ? widget->index(L, prop) : 0;
}

/** Generic widget newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_widget_newindex(lua_State *L)
{
    const char *prop = luaL_checkstring(L, 2);

    /* Try standard method */
    luaA_class_newindex(L);

    /* Then call special widget newindex */
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    return widget->newindex ? widget->newindex(L, prop) : 0;
}

static int
luaA_widget_extents(lua_State *L)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    area_t g = {
        .x = 0,
        .y = 0,
        .width = 0,
        .height = 0
    };

    if(widget->extents)
        g = widget->extents(L, widget);

    lua_newtable(L);
    lua_pushnumber(L, g.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, g.height);
    lua_setfield(L, -2, "height");

    return 1;
}

static int
luaA_widget_get_type(lua_State *L, widget_t *w)
{
    if (w->type == widget_textbox) lua_pushstring(L, "textbox");
    else if (w->type == widget_systray) lua_pushstring(L, "systray");
    else if (w->type == widget_imagebox) lua_pushstring(L, "imagebox");
    else lua_pushstring(L, "unknown");

    return 1;
}

static int
luaA_widget_set_type(lua_State *L, widget_t *w)
{
    const char *type = luaL_checkstring(L, -1);
    widget_constructor_t *wc = NULL;

    if(a_strcmp(type, "textbox") == 0)
        wc = widget_textbox;
    else if(a_strcmp(type, "systray") == 0)
        wc = widget_systray;
    else if(a_strcmp(type, "imagebox") == 0)
        wc = widget_imagebox;

    if(!wc)
        luaL_error(L, "unknown widget type: %s", type);

    wc(w);
    w->type = wc;
    luaA_object_emit_signal(L, -3, "property::type", 0);

    return 0;
}

static int
luaA_widget_get_visible(lua_State *L, widget_t *w)
{
    lua_pushboolean(L, w->isvisible);
    return 1;
}

static int
luaA_widget_set_visible(lua_State *L, widget_t *w)
{
    w->isvisible = luaA_checkboolean(L, -1);
    widget_invalidate_bywidget(w);
    luaA_object_emit_signal(L, -3, "property::visible", 0);
    return 0;
}

void
widget_class_setup(lua_State *L)
{
    static const struct luaL_reg widget_methods[] =
    {
        LUA_CLASS_METHODS(widget)
        { "__call", luaA_widget_new },
        { NULL, NULL }
    };

    static const struct luaL_reg widget_meta[] =
    {
        LUA_OBJECT_META(widget)
        { "buttons", luaA_widget_buttons },
        { "extents", luaA_widget_extents },
        { "__index", luaA_widget_index },
        { "__newindex", luaA_widget_newindex },
        { NULL, NULL }
    };

    luaA_class_setup(L, &widget_class, "widget", NULL,
                     (lua_class_allocator_t) widget_new,
                     (lua_class_collector_t) widget_wipe,
                     NULL, NULL, NULL,
                     widget_methods, widget_meta);
    luaA_class_add_property(&widget_class, "visible",
                            (lua_class_propfunc_t) luaA_widget_set_visible,
                            (lua_class_propfunc_t) luaA_widget_get_visible,
                            (lua_class_propfunc_t) luaA_widget_set_visible);
    luaA_class_add_property(&widget_class, "type",
                            (lua_class_propfunc_t) luaA_widget_set_type,
                            (lua_class_propfunc_t) luaA_widget_get_type,
                            NULL);

    signal_add(&widget_class.signals, "mouse::enter");
    signal_add(&widget_class.signals, "mouse::leave");
    signal_add(&widget_class.signals, "property::buttons");
    signal_add(&widget_class.signals, "property::type");
    signal_add(&widget_class.signals, "property::visible");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
