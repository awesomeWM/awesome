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
#include "workspace.h"
#include "layouts/floating.h"

extern awesome_t globalconf;

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

    if(!c->titlebar || !c->titlebar->sw || !c->titlebar->position)
        return;

    s = xcb_aux_get_screen(globalconf.connection,
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
                  workspace_screen_get(workspace_client_get(c)), c->titlebar->position,
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

/** Update the titlebar geometry for a floating client.
 * \param c the client
 */
void
titlebar_update_geometry_floating(client_t *c)
{
    int width, x_offset = 0, y_offset = 0;

    if(!c->titlebar || !c->titlebar->sw)
        return;

    switch(c->titlebar->position)
    {
      default:
        return;
      case Top:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.width);
        else
            width = c->geometry.width + 2 * c->border;
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y - c->titlebar->sw->geometry.height,
                                 width,
                                 c->titlebar->sw->geometry.height);
        break;
      case Bottom:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.width);
        else
            width = c->geometry.width + 2 * c->border;
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y + c->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar->sw->geometry.height);
        break;
      case Left:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.height);
        else
            width = c->geometry.height + 2 * c->border;
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 c->geometry.x - c->titlebar->sw->geometry.width,
                                 c->geometry.y + y_offset,
                                 c->titlebar->sw->geometry.width,
                                 width);
        break;
      case Right:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.height);
        else
            width = c->geometry.height + 2 * c->border;
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 c->geometry.x + c->geometry.width + 2 * c->border,
                                 c->geometry.y + y_offset,
                                 c->titlebar->sw->geometry.width,
                                 width);
        break;
    }
    titlebar_draw(c);
}


/** Update the titlebar geometry for a tiled client.
 * \param c The client.
 * \param geometry The geometry the client will receive.
 */
void
titlebar_update_geometry(client_t *c, area_t geometry)
{
    int width, x_offset = 0 , y_offset = 0;

    if(!c->titlebar || !c->titlebar->sw)
        return;

    switch(c->titlebar->position)
    {
      default:
        return;
      case Top:
        if(!c->titlebar->width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar->width, geometry.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 geometry.x + x_offset,
                                 geometry.y,
                                 width,
                                 c->titlebar->sw->geometry.height);
        break;
      case Bottom:
        if(c->titlebar->width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar->width, geometry.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 geometry.x + x_offset,
                                 geometry.y + geometry.height
                                     - c->titlebar->sw->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar->sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar->width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar->width, geometry.height);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 geometry.x,
                                 geometry.y + y_offset,
                                 c->titlebar->sw->geometry.width,
                                 width);
        break;
      case Right:
        if(c->titlebar->width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar->width, geometry.height);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar->sw,
                                 geometry.x + geometry.width
                                     - c->titlebar->sw->geometry.width + 2 * c->border,
                                 geometry.y + y_offset,
                                 c->titlebar->sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(c);
}

/** Set client titlebar position.
 * \param c The client.
 */
void
titlebar_init(client_t *c)
{
    int width = 0, height = 0;

    switch(c->titlebar->position)
    {
      default:
        c->titlebar->position = Off;
        if(c->titlebar->sw)
            xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
        return;
      case Top:
      case Bottom:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.width);
        else
            width = c->geometry.width + 2 * c->border;
        height = c->titlebar->height;
        break;
      case Left:
      case Right:
        if(c->titlebar->width)
            height = MIN(c->titlebar->width, c->geometry.height);
        else
            height = c->geometry.height + 2 * c->border;
        width = c->titlebar->height;
        break;
    }

    /* Delete old statusbar */
    simplewindow_delete(&c->titlebar->sw);

    c->titlebar->sw = simplewindow_new(globalconf.connection,
                                    c->phys_screen, 0, 0,
                                    width, height, 0);
    titlebar_draw(c);
    xcb_map_window(globalconf.connection, c->titlebar->sw->window);
}

/** Create a new titlebar.
 * \param A table with values: align, position, fg, bg, width and height.
 * \return A brand new titlebar.
 */
static int
luaA_titlebar_new(lua_State *L)
{
    titlebar_t *tb;
    const char *color;

    luaA_checktable(L, 1);

    tb = p_new(titlebar_t, 1);

    tb->align = draw_align_get_from_str(luaA_getopt_string(L, 1, "align", "left"));

    tb->width = luaA_getopt_number(L, 1, "width", 0);
    tb->height = luaA_getopt_number(L, 1, "height", 0);
    if(tb->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        tb->height = 1.5 * globalconf.font->height;

    tb->position = position_get_from_str(luaA_getopt_string(L, 1, "position", "top"));

    lua_getfield(L, 1, "fg");
    if((color = luaL_optstring(L, -1, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &tb->colors.fg);
    else
        tb->colors.fg = globalconf.colors.fg;

    lua_getfield(L, 1, "bg");
    if((color = luaL_optstring(L, -1, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &tb->colors.bg);
    else
        tb->colors.bg = globalconf.colors.bg;

    return luaA_titlebar_userdata_new(tb);
}

/** Add a widget to a titlebar.
 * \param A widget.
 */
static int
luaA_titlebar_widget_add(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    widget_t **widget = luaA_checkudata(L, 2, "widget");
    widget_node_t *w = p_new(widget_node_t, 1);
    client_t *c;

    w->widget = *widget;
    widget_node_list_append(&(*tb)->widgets, w);
    widget_ref(widget);

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar == *tb)
        {
            titlebar_draw(c);
            break;
        }

    return 0;
}

/** Get all widgets from a titlebar.
 * \return A table with all widgets from the titlebar.
 */
static int
luaA_titlebar_widget_get(lua_State *L)
{
    titlebar_t **tb = luaA_checkudata(L, 1, "titlebar");
    widget_node_t *widget;
    int i = 1;

    lua_newtable(L);

    for(widget = (*tb)->widgets; widget; widget = widget->next)
    {
        luaA_widget_userdata_new(widget->widget);
        /* ref again for the list */
        widget_ref(&widget->widget);
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/** Get the client which the titlebar is attached to. That is a the same as
 * checking if every clients's titlebar is equal to titlebar.
 * \return A client if the titlebar is attached, nil otherwise.
 */
static int
luaA_titlebar_client_get(lua_State *L)
{
    titlebar_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    client_t *c;

    if((c = client_getbytitlebar(*titlebar)))
        return luaA_client_userdata_new(c);

    return 0;
}

/** Create a new titlebar userdata.
 * \param t The titlebar.
 */
int
luaA_titlebar_userdata_new(titlebar_t *t)
{
    titlebar_t **tb = lua_newuserdata(globalconf.L, sizeof(titlebar_t *));
    *tb = t;
    titlebar_ref(tb);
    return luaA_settype(globalconf.L, "titlebar");
}

static int
luaA_titlebar_gc(lua_State *L)
{
    titlebar_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    titlebar_unref(titlebar);
    *titlebar = NULL;
    return 0;
}

static int
luaA_titlebar_tostring(lua_State *L)
{
    titlebar_t **p = luaA_checkudata(L, 1, "titlebar");
    lua_pushfstring(L, "[titlebar udata(%p)]", *p);
    return 1;
}

static int
luaA_titlebar_eq(lua_State *L)
{
    titlebar_t **t1 = luaA_checkudata(L, 1, "titlebar");
    titlebar_t **t2 = luaA_checkudata(L, 2, "titlebar");
    lua_pushboolean(L, (*t1 == *t2));
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
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
