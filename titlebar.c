/*
 * titlebar.c - titlebar management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include <math.h>

#include "titlebar.h"
#include "client.h"
#include "screen.h"
#include "widget.h"
#include "layouts/floating.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, titlebar_t, titlebar, "titlebar", titlebar_ref)
DO_LUA_GC(titlebar_t, titlebar, "titlebar", titlebar_unref)
DO_LUA_EQ(titlebar_t, titlebar, "titlebar")

/** Get a client by its titlebar.
 * \param titlebar The titlebar.
 * \return A client.
 */
client_t *
client_getbytitlebar(titlebar_t *titlebar)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar == titlebar)
            return c;

    return NULL;
}

/** Get a client by its titlebar window.
 * \param win The window.
 * \return A client.
 */
client_t *
client_getbytitlebarwin(xcb_window_t win)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar && c->titlebar->sw && c->titlebar->sw->window == win)
            return c;

    return NULL;
}

/** Draw the titlebar content.
 * \param c The client.
 */
void
titlebar_draw(client_t *c)
{
    xcb_drawable_t dw = 0;
    draw_context_t *ctx;
    xcb_screen_t *s;

    if(!c || !c->titlebar || !c->titlebar->sw || !c->titlebar->position)
        return;

    s = xutil_screen_get(globalconf.connection,
                           c->titlebar->sw->phys_screen);

    switch(c->titlebar->position)
    {
      case Off:
        return;
      case Right:
      case Left:
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth,
                          dw,
                          s->root,
                          c->titlebar->sw->geometry.height,
                          c->titlebar->sw->geometry.width);
        ctx = draw_context_new(globalconf.connection, c->titlebar->sw->phys_screen,
                               c->titlebar->sw->geometry.height,
                               c->titlebar->sw->geometry.width,
                               dw,
                               c->titlebar->colors.fg,
                               c->titlebar->colors.bg);
        break;
      default:
        ctx = draw_context_new(globalconf.connection, c->titlebar->sw->phys_screen,
                               c->titlebar->sw->geometry.width,
                               c->titlebar->sw->geometry.height,
                               c->titlebar->sw->pixmap,
                               c->titlebar->colors.fg,
                               c->titlebar->colors.bg);
        break;
    }

    widget_render(c->titlebar->widgets, ctx, c->titlebar->sw->gc, c->titlebar->sw->pixmap,
                  c->screen, c->titlebar->position,
                  c->titlebar->sw->geometry.x, c->titlebar->sw->geometry.y, c->titlebar);

    switch(c->titlebar->position)
    {
      case Left:
        draw_rotate(ctx, ctx->pixmap, c->titlebar->sw->pixmap,
                    ctx->width, ctx->height,
                    ctx->height, ctx->width,
                    - M_PI_2, 0, c->titlebar->sw->geometry.height);
        xcb_free_pixmap(globalconf.connection, dw);
        break;
      case Right:
        draw_rotate(ctx, ctx->pixmap, c->titlebar->sw->pixmap,
                    ctx->width, ctx->height,
                    ctx->height, ctx->width,
                    M_PI_2, c->titlebar->sw->geometry.width, 0);
        xcb_free_pixmap(globalconf.connection, dw);
      default:
        break;
    }

    simplewindow_refresh_pixmap(c->titlebar->sw);

    draw_context_delete(&ctx);
}

void
titlebar_geometry_compute(client_t *c, area_t geometry, area_t *res)
{
    int width, x_offset = 0, y_offset = 0;

    switch(c->titlebar->position)
    {
      default:
        return;
      case Top:
        if(c->titlebar->width)
            width = MAX(1, MIN(c->titlebar->width, geometry.width - 2 * c->titlebar->border.width));
        else
            width = MAX(1, geometry.width + 2 * c->border - 2 * c->titlebar->border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width - 2 * c->titlebar->border.width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        res->x = geometry.x + x_offset;
        res->y = geometry.y - c->titlebar->height - 2 * c->titlebar->border.width + c->border;
        res->width = width;
        res->height = c->titlebar->height;
        break;
      case Bottom:
        if(c->titlebar->width)
            width = MAX(1, MIN(c->titlebar->width, geometry.width - 2 * c->titlebar->border.width));
        else
            width = MAX(1, geometry.width + 2 * c->border - 2 * c->titlebar->border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width - 2 * c->titlebar->border.width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        res->x = geometry.x + x_offset;
        res->y = geometry.y + geometry.height + c->border;
        res->width = width;
        res->height = c->titlebar->height;
        break;
      case Left:
        if(c->titlebar->width)
            width = MAX(1, MIN(c->titlebar->width, geometry.height - 2 * c->titlebar->border.width));
        else
            width = MAX(1, geometry.height + 2 * c->border - 2 * c->titlebar->border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width - 2 * c->titlebar->border.width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        res->x = geometry.x - c->titlebar->height + c->border;
        res->y = geometry.y + y_offset;
        res->width = c->titlebar->height;
        res->height = width;
        break;
      case Right:
        if(c->titlebar->width)
            width = MAX(1, MIN(c->titlebar->width, geometry.height - 2 * c->titlebar->border.width));
        else
            width = MAX(1, geometry.height + 2 * c->border - 2 * c->titlebar->border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width - 2 * c->titlebar->border.width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        res->x = geometry.x + geometry.width + c->border;
        res->y = geometry.y + y_offset;
        res->width = c->titlebar->height;
        res->height = width;
        break;
    }
}

/** Set client titlebar position.
 * \param c The client.
 */
void
titlebar_init(client_t *c)
{
    int width = 0, height = 0;
    area_t geom;

    switch(c->titlebar->position)
    {
      default:
        c->titlebar->position = Off;
        return;
      case Top:
      case Bottom:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.width - 2 * c->titlebar->border.width);
        else
            width = c->geometry.width + 2 * c->border - 2 * c->titlebar->border.width;
        height = c->titlebar->height;
        break;
      case Left:
      case Right:
        if(c->titlebar->width)
            height = MIN(c->titlebar->width, c->geometry.height - 2 * c->titlebar->border.width);
        else
            height = c->geometry.height + 2 * c->border - 2 * c->titlebar->border.width;
        width = c->titlebar->height;
        break;
    }
    
    titlebar_geometry_compute(c, c->geometry, &geom);

    c->titlebar->sw = simplewindow_new(globalconf.connection, c->phys_screen, geom.x, geom.y,
                                       geom.width, geom.height, c->titlebar->border.width);

    if(c->titlebar->border.width)
        xcb_change_window_attributes(globalconf.connection, c->titlebar->sw->window,
                                     XCB_CW_BORDER_PIXEL, &c->titlebar->border.color.pixel);

    if(client_isvisible(c, c->screen))
        globalconf.screens[c->screen].need_arrange = true;

    titlebar_draw(c);
}

/** Create a new titlebar.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lparam A table with values: align, position, fg, bg, border_width,
 * border_color, width and height.
 * \lreturn A brand new titlebar.
 */
static int
luaA_titlebar_new(lua_State *L)
{
    titlebar_t *tb;
    const char *color;
    size_t len;

    luaA_checktable(L, 1);

    tb = p_new(titlebar_t, 1);

    tb->align = draw_align_fromstr(luaA_getopt_string(L, 1, "align", "left", &len), len);

    tb->width = luaA_getopt_number(L, 1, "width", 0);
    tb->height = luaA_getopt_number(L, 1, "height", 0);
    if(tb->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        tb->height = 1.5 * globalconf.font->height;

    tb->position = position_fromstr(luaA_getopt_string(L, 1, "position", "top", &len), len);

    if((color = luaA_getopt_string(L, -1, "fg", NULL, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &tb->colors.fg);
    else
        tb->colors.fg = globalconf.colors.fg;

    if((color = luaA_getopt_string(L, 1, "bg", NULL, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &tb->colors.bg);
    else
        tb->colors.bg = globalconf.colors.bg;

    if((color = luaA_getopt_string(L, 1, "border_color", NULL, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                   color, &tb->border.color);

    tb->border.width = luaA_getopt_number(L, 1, "border_width", 0);

    return luaA_titlebar_userdata_new(globalconf.L, tb);
}

/** Add a widget to a titlebar.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lvalue A titlebar.
 * \lparam A widget.
 */
static int
luaA_titlebar_widget_add(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    widget_t **widget = luaA_checkudata(L, 2, "widget");
    widget_node_t *witer, *w = p_new(widget_node_t, 1);

    if((*widget)->type == systray_new)
        luaL_error(L, "cannot add systray widget to titlebar");

    /* check that there is not already a widget with that name in the titlebar */
    for(witer = (*tb)->widgets; witer; witer = witer->next)
        if(witer->widget != *widget
           && !a_strcmp(witer->widget->name, (*widget)->name))
            luaL_error(L, "a widget with name `%s' already on titlebar", witer->widget->name);

    w->widget = *widget;
    widget_node_list_append(&(*tb)->widgets, w);
    widget_ref(widget);

    titlebar_draw(client_getbytitlebar(*tb));

    return 0;
}

/** Get all widgets from a titlebar.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lvalue A titlebar
 * \lreturn A table with all widgets from the titlebar.
 */
static int
luaA_titlebar_widget_get(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    widget_node_t *witer;

    lua_newtable(L);

    for(witer = (*tb)->widgets; witer; witer = witer->next)
    {
        luaA_widget_userdata_new(L, witer->widget);
        lua_setfield(L, -2, witer->widget->name);
    }

    return 1;
}

/** Get the client which the titlebar is attached to. That is a the same as
 * checking if every clients's titlebar is equal to titlebar.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A titlebar.
 * \lreturn A client if the titlebar is attached, nil otherwise.
 */
static int
luaA_titlebar_client_get(lua_State *L)
{
    titlebar_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    client_t *c;

    if((c = client_getbytitlebar(*titlebar)))
        return luaA_client_userdata_new(L, c);

    return 0;
}

/** Set titlebar colors.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lvalue A titlebar.
 * \lparam A table with keys `fg' and `bg', for foreground and background colors.
 */
static int
luaA_titlebar_colors_set(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    const char *color;

    luaA_checktable(L, 2);

    if((color = luaA_getopt_string(L, 2, "fg", NULL, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*tb)->colors.fg);

    if((color = luaA_getopt_string(L, 2, "bg", NULL, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*tb)->colors.bg);

    titlebar_draw(client_getbytitlebar(*tb));

    return 0;
}

/** Set titlebar border color and width.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lvalue A titlebar.
 * \lparam A table with keys `color' and `width'.
 */
static int
luaA_titlebar_border_set(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    client_t *c;
    const char *color;
    int border_width;

    luaA_checktable(L, 2);

    if((color = luaA_getopt_string(L, 2, "color", NULL, NULL)))
    {
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*tb)->border.color);
        if((*tb)->sw)
            xcb_change_window_attributes(globalconf.connection, (*tb)->sw->window,
                                         XCB_CW_BORDER_PIXEL, &(*tb)->border.color.pixel);
    }

    if((border_width = luaA_getopt_number(L, 2, "width", -1)) >= 0)
    {
        (*tb)->border.width = border_width;
        if((*tb)->sw)
            simplewindow_border_width_set((*tb)->sw, border_width);
        if((c = client_getbytitlebar(*tb)) && client_isvisible(c, c->screen))
        {
            globalconf.screens[c->screen].need_arrange = true;
            if(c->isfloating)
                titlebar_update_geometry_floating(c);
        }
    }

    return 0;
}

/** Convert a titlebar to a printable string.
 * \param L The Lua VM state.
 * \return The number of value pushed.
 *
 * \luastack
 * \lvalue A titlebar.
 * \lreturn A string.
 */
static int
luaA_titlebar_tostring(lua_State *L)
{
    titlebar_t **p = luaA_checkudata(L, 1, "titlebar");
    lua_pushfstring(L, "[titlebar udata(%p)]", *p);
    return 1;
}

const struct luaL_reg awesome_titlebar_methods[] =
{
    { "new", luaA_titlebar_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_titlebar_meta[] =
{
    { "widget_add", luaA_titlebar_widget_add },
    { "widget_get", luaA_titlebar_widget_get },
    { "client_get", luaA_titlebar_client_get },
    { "colors_set", luaA_titlebar_colors_set },
    { "border_set", luaA_titlebar_border_set },
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
