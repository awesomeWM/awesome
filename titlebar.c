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

/** Get a titlebar for a client
 * \param c The client.
 * \return A titlebar.
 */
titlebar_t *
titlebar_getbyclient(client_t *c)
{
    titlebar_t *t;

    for(t = globalconf.titlebar; t; t = t->next)
        if(t->client == c)
            return t;

    return NULL;
}

/** Get a titlebar which own a window.
 * \param The window.
 * \return A titlebar.
 */
titlebar_t *
titlebar_getbywin(xcb_window_t win)
{
    titlebar_t *t;

    for(t = globalconf.titlebar; t; t = t->next)
        if(t->sw && t->sw->window == win)
            return t;

    return NULL;
}

/** Draw the titlebar content.
 * \param c the client
 * \todo stop duplicating the context
 */
void
titlebar_draw(titlebar_t *titlebar)
{
    xcb_drawable_t dw = 0;
    draw_context_t *ctx;
    area_t geometry;
    xcb_screen_t *s;

    if(!titlebar || !titlebar->sw || !titlebar->position)
        return;

    s = xcb_aux_get_screen(globalconf.connection,
                           titlebar->sw->phys_screen);

    /** \todo move this in init */
    switch(titlebar->position)
    {
      case Off:
        return;
      case Right:
      case Left:
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth,
                          dw,
                          s->root,
                          titlebar->sw->geometry.height,
                          titlebar->sw->geometry.width);
        ctx = draw_context_new(globalconf.connection, titlebar->sw->phys_screen,
                               titlebar->sw->geometry.height,
                               titlebar->sw->geometry.width,
                               dw,
                               titlebar->colors.fg,
                               titlebar->colors.bg);
        geometry.width = titlebar->sw->geometry.height;
        geometry.height = titlebar->sw->geometry.width;
        break;
      default:
        ctx = draw_context_new(globalconf.connection, titlebar->sw->phys_screen,
                               titlebar->sw->geometry.width,
                               titlebar->sw->geometry.height,
                               titlebar->sw->drawable,
                               titlebar->colors.fg,
                               titlebar->colors.bg);
        geometry = titlebar->sw->geometry;
        break;
    }

    widget_render(titlebar->widgets, ctx, titlebar->sw->gc, titlebar->sw->drawable,
                  titlebar->client->screen, titlebar->position,
                  titlebar->sw->geometry.x, titlebar->sw->geometry.y, titlebar);

    switch(titlebar->position)
    {
      case Left:
        draw_rotate(ctx, ctx->drawable, titlebar->sw->drawable,
                    ctx->width, ctx->height,
                    ctx->height, ctx->width,
                    - M_PI_2, 0, titlebar->sw->geometry.height);
        xcb_free_pixmap(globalconf.connection, dw);
        break;
      case Right:
        draw_rotate(ctx, ctx->drawable, titlebar->sw->drawable,
                    ctx->width, ctx->height,
                    ctx->height, ctx->width,
                    M_PI_2, titlebar->sw->geometry.width, 0);
        xcb_free_pixmap(globalconf.connection, dw);
      default:
        break;
    }

    simplewindow_refresh_drawable(titlebar->sw);

    draw_context_delete(&ctx);
}

/** Update the titlebar geometry for a floating client.
 * \param c the client
 */
void
titlebar_update_geometry_floating(titlebar_t *titlebar)
{
    int width, x_offset = 0, y_offset = 0;

    if(!titlebar || !titlebar->sw)
        return;

    switch(titlebar->position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(titlebar->width)
            width = MIN(titlebar->width, titlebar->client->geometry.width);
        else
            width = titlebar->client->geometry.width + 2 * titlebar->client->border;
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * titlebar->client->border + titlebar->client->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (titlebar->client->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 titlebar->client->geometry.x + x_offset,
                                 titlebar->client->geometry.y - titlebar->sw->geometry.height,
                                 width,
                                 titlebar->sw->geometry.height);
        break;
      case Bottom:
        if(titlebar->width)
            width = MIN(titlebar->width, titlebar->client->geometry.width);
        else
            width = titlebar->client->geometry.width + 2 * titlebar->client->border;
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * titlebar->client->border + titlebar->client->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (titlebar->client->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 titlebar->client->geometry.x + x_offset,
                                 titlebar->client->geometry.y + titlebar->client->geometry.height + 2 * titlebar->client->border,
                                 width,
                                 titlebar->sw->geometry.height);
        break;
      case Left:
        if(titlebar->width)
            width = MIN(titlebar->width, titlebar->client->geometry.height);
        else
            width = titlebar->client->geometry.height + 2 * titlebar->client->border;
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * titlebar->client->border + titlebar->client->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (titlebar->client->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 titlebar->client->geometry.x - titlebar->sw->geometry.width,
                                 titlebar->client->geometry.y + y_offset,
                                 titlebar->sw->geometry.width,
                                 width);
        break;
      case Right:
        if(titlebar->width)
            width = MIN(titlebar->width, titlebar->client->geometry.height);
        else
            width = titlebar->client->geometry.height + 2 * titlebar->client->border;
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * titlebar->client->border + titlebar->client->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (titlebar->client->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 titlebar->client->geometry.x + titlebar->client->geometry.width + 2 * titlebar->client->border,
                                 titlebar->client->geometry.y + y_offset,
                                 titlebar->sw->geometry.width,
                                 width);
        break;
    }
    titlebar_draw(titlebar);
}


/** Update the titlebar geometry for a tiled client.
 * \param c the client
 * \param geometry the geometry the client will receive
 */
void
titlebar_update_geometry(titlebar_t *titlebar, area_t geometry)
{
    int width, x_offset = 0 , y_offset = 0;

    if(!titlebar || !titlebar->sw)
        return;

    switch(titlebar->position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(!titlebar->width)
            width = geometry.width + 2 * titlebar->client->border;
        else
            width = MIN(titlebar->width, geometry.width);
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * titlebar->client->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 geometry.x + x_offset,
                                 geometry.y,
                                 width,
                                 titlebar->sw->geometry.height);
        break;
      case Bottom:
        if(titlebar->width)
            width = geometry.width + 2 * titlebar->client->border;
        else
            width = MIN(titlebar->width, geometry.width);
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * titlebar->client->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 geometry.x + x_offset,
                                 geometry.y + geometry.height
                                     - titlebar->sw->geometry.height + 2 * titlebar->client->border,
                                 width,
                                 titlebar->sw->geometry.height);
        break;
      case Left:
        if(!titlebar->width)
            width = geometry.height + 2 * titlebar->client->border;
        else
            width = MIN(titlebar->width, geometry.height);
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * titlebar->client->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 geometry.x,
                                 geometry.y + y_offset,
                                 titlebar->sw->geometry.width,
                                 width);
        break;
      case Right:
        if(titlebar->width)
            width = geometry.height + 2 * titlebar->client->border;
        else
            width = MIN(titlebar->width, geometry.height);
        switch(titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * titlebar->client->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(titlebar->sw,
                                 geometry.x + geometry.width
                                     - titlebar->sw->geometry.width + 2 * titlebar->client->border,
                                 geometry.y + y_offset,
                                 titlebar->sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(titlebar);
}

/** Set client titlebar position.
 * \param c The client.
 * \param p The position.
 */
void
titlebar_init(titlebar_t *titlebar)
{
    int width = 0, height = 0;

    switch(titlebar->position)
    {
      default:
        titlebar->position = Off;
        if(titlebar->sw)
            xcb_unmap_window(globalconf.connection, titlebar->sw->window);
        return;
      case Top:
      case Bottom:
        if(titlebar->width)
            width = MIN(titlebar->width, titlebar->client->geometry.width);
        else
            width = titlebar->client->geometry.width + 2 * titlebar->client->border;
        height = titlebar->height;
        break;
      case Left:
      case Right:
        if(titlebar->width)
            height = MIN(titlebar->width, titlebar->client->geometry.height);
        else
            height = titlebar->client->geometry.height + 2 * titlebar->client->border;
        width = titlebar->height;
        break;
    }

    /* Delete old statusbar */
    simplewindow_delete(&titlebar->sw);

    titlebar->sw = simplewindow_new(globalconf.connection,
                                    titlebar->client->phys_screen, 0, 0,
                                    width, height, 0);
    titlebar_draw(titlebar);
    xcb_map_window(globalconf.connection, titlebar->sw->window);
}

/** Create a new titlebar.
 * \param A table with values: align, position, fg, bg, width and height.
 * \return A brand new titlebar.
 */
static int
luaA_titlebar_new(lua_State *L)
{
    titlebar_t **tb;
    int objpos;
    const char *color;

    luaA_checktable(L, 1);

    tb = lua_newuserdata(L, sizeof(titlebar_t *));
    *tb = p_new(titlebar_t, 1);
    objpos = lua_gettop(L);

    (*tb)->align = draw_align_get_from_str(luaA_getopt_string(L, 1, "align", "left"));

    (*tb)->width = luaA_getopt_number(L, 1, "width", 0);
    (*tb)->height = luaA_getopt_number(L, 1, "height", 0);
    if((*tb)->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        (*tb)->height = 1.5 * globalconf.font->height;

    (*tb)->position = position_get_from_str(luaA_getopt_string(L, 1, "position", "top"));

    lua_getfield(L, 1, "fg");
    if((color = luaL_optstring(L, -1, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*tb)->colors.fg);
    else
        (*tb)->colors.fg = globalconf.colors.fg;

    lua_getfield(L, 1, "bg");
    if((color = luaL_optstring(L, -1, NULL)))
        xcolor_new(globalconf.connection, globalconf.default_screen,
                       color, &(*tb)->colors.bg);
    else
        (*tb)->colors.bg = globalconf.colors.bg;

    titlebar_ref(tb);

    lua_pushvalue(L, objpos);
    return luaA_settype(L, "titlebar");
}

/** Set the client where the titlebar belongs to.
 * \param A client where to attach titlebar, or none for removing.
 */
static int
luaA_titlebar_client_set(lua_State *L)
{
    titlebar_t *oldt, **t = luaL_checkudata(L, 1, "titlebar");
    client_t **c = NULL;
   
    if(lua_gettop(L) == 2)
        c = luaL_checkudata(L, 2, "client");

    if(!c)
    {
        if((*t)->client
           && (*t)->client->isfloating
           && layout_get_current((*t)->client->screen) != layout_floating)
            globalconf.screens[(*t)->client->screen].need_arrange = true;

        simplewindow_delete(&(*t)->sw);
        titlebar_list_detach(&globalconf.titlebar, *t);
        titlebar_unref(t);
    }
    else
    {
        if((oldt = titlebar_getbyclient(*c)))
        {
            simplewindow_delete(&oldt->sw);
            titlebar_list_detach(&globalconf.titlebar, oldt);
            titlebar_unref(&oldt);
        }

        titlebar_list_push(&globalconf.titlebar, *t);
        titlebar_ref(t);

        (*t)->client = *c;
        titlebar_init(*t);

        if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
            titlebar_update_geometry_floating(*t);
        else
            globalconf.screens[(*c)->screen].need_arrange = true;
    }

    return 0;
}

/** Add a widget to a titlebar.
 * \param A widget.
 */
static int
luaA_titlebar_widget_add(lua_State *L)
{
    titlebar_t **tb = luaL_checkudata(L, 1, "titlebar");
    widget_t **widget = luaL_checkudata(L, 2, "widget");
    widget_node_t *w = p_new(widget_node_t, 1);

    w->widget = *widget;
    widget_node_list_append(&(*tb)->widgets, w);
    widget_ref(widget);

    titlebar_draw(*tb);

    return 0;
}

static int
luaA_titlebar_gc(lua_State *L)
{
    titlebar_t **titlebar = luaL_checkudata(L, 1, "titlebar");
    titlebar_unref(titlebar);
    return 0;
}

static int
luaA_titlebar_tostring(lua_State *L)
{
    titlebar_t **p = luaL_checkudata(L, 1, "titlebar");
    lua_pushfstring(L, "[titlebar udata(%p)]", *p);
    return 1;
}

static int
luaA_titlebar_eq(lua_State *L)
{
    titlebar_t **t1 = luaL_checkudata(L, 1, "titlebar");
    titlebar_t **t2 = luaL_checkudata(L, 2, "titlebar");
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
    { "client_set", luaA_titlebar_client_set },
    { "widget_add", luaA_titlebar_widget_add },
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
