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
#include "layouts/floating.h"

extern AwesomeConf globalconf;

static char *
titlebar_text(client_t *c)
{
    char *text;

    if(globalconf.focus->client == c)
        text = c->titlebar.text_focus;
    else if(c->isurgent)
        text = c->titlebar.text_urgent;
    else
        text = c->titlebar.text_normal;

    return client_markup_parse(c, text, a_strlen(text));
}

static inline area_t
titlebar_size(client_t *c)
{
    return draw_text_extents(globalconf.connection, globalconf.default_screen,
                             globalconf.font, titlebar_text(c));
}

/** Draw the titlebar content.
 * \param c the client
 */
void
titlebar_draw(client_t *c)
{
    xcb_drawable_t dw = 0;
    draw_context_t *ctx;
    area_t geometry;
    xcb_screen_t *s;
    char *text;

    if(!c->titlebar_sw)
        return;

    s = xcb_aux_get_screen(globalconf.connection,
                           c->titlebar_sw->phys_screen);

    switch(c->titlebar.position)
    {
      case Off:
        return;
      case Right:
      case Left:
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth,
                          dw,
                          s->root,
                          c->titlebar_sw->geometry.height,
                          c->titlebar_sw->geometry.width);
        ctx = draw_context_new(globalconf.connection, c->titlebar_sw->phys_screen,
                               c->titlebar_sw->geometry.height,
                               c->titlebar_sw->geometry.width,
                               dw);
        geometry.width = c->titlebar_sw->geometry.height;
        geometry.height = c->titlebar_sw->geometry.width;
        break;
      default:
        ctx = draw_context_new(globalconf.connection, c->titlebar_sw->phys_screen,
                               c->titlebar_sw->geometry.width,
                               c->titlebar_sw->geometry.height,
                               c->titlebar_sw->drawable);
        geometry = c->titlebar_sw->geometry;
        break;
    }

    text = titlebar_text(c);
    geometry.x = geometry.y = 0;
    draw_rectangle(ctx, geometry, 1.0, true, globalconf.colors.bg);
    draw_text(ctx, globalconf.font, &globalconf.colors.fg, geometry, text);
    p_delete(&text);

    switch(c->titlebar.position)
    {
      case Left:
        draw_rotate(ctx, c->titlebar_sw->drawable, ctx->height, ctx->width,
                    - M_PI_2, 0, c->titlebar_sw->geometry.height);
        xcb_free_pixmap(globalconf.connection, dw);
        break;
      case Right:
        draw_rotate(ctx, c->titlebar_sw->drawable, ctx->height, ctx->width,
                    M_PI_2, c->titlebar_sw->geometry.width, 0);
        xcb_free_pixmap(globalconf.connection, dw);
      default:
        break;
    }

    simplewindow_refresh_drawable(c->titlebar_sw);

    draw_context_delete(&ctx);
}

/** Update the titlebar geometry for a floating client.
 * \param c the client
 */
void
titlebar_update_geometry_floating(client_t *c)
{
    int width, x_offset = 0, y_offset = 0;

    if(!c->titlebar_sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y - c->titlebar_sw->geometry.height,
                                 width,
                                 c->titlebar_sw->geometry.height);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y + c->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar_sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 c->geometry.x - c->titlebar_sw->geometry.width,
                                 c->geometry.y + y_offset,
                                 c->titlebar_sw->geometry.width,
                                 width);
        break;
      case Right:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 c->geometry.x + c->geometry.width + 2 * c->border,
                                 c->geometry.y + y_offset,
                                 c->titlebar_sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(c);
}


/** Update the titlebar geometry for a tiled client.
 * \param c the client
 * \param geometry the geometry the client will receive
 */
void
titlebar_update_geometry(client_t *c, area_t geometry)
{
    int width, x_offset = 0 , y_offset = 0;

    if(!c->titlebar_sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 geometry.x + x_offset,
                                 geometry.y,
                                 width,
                                 c->titlebar_sw->geometry.height);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 geometry.x + x_offset,
                                 geometry.y + geometry.height
                                     - c->titlebar_sw->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar_sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 geometry.x,
                                 geometry.y + y_offset,
                                 c->titlebar_sw->geometry.width,
                                 width);
        break;
      case Right:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
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
        simplewindow_move_resize(c->titlebar_sw,
                                 geometry.x + geometry.width
                                     - c->titlebar_sw->geometry.width + 2 * c->border,
                                 geometry.y + y_offset,
                                 c->titlebar_sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(c);
}

/** Set client titlebar position.
 * \param c The client.
 * \param p The position.
 */
void
titlebar_init(client_t *c)
{
    int width = 0, height = 0;

    if(!c->titlebar.height)
        c->titlebar.height = draw_text_extents(globalconf.connection, globalconf.default_screen,
                                               globalconf.font,
                                               client_markup_parse(c,
                                                                   c->titlebar.text_focus,
                                                                   a_strlen(c->titlebar.text_focus))).height;
    switch(c->titlebar.position)
    {
      default:
        c->titlebar.position = Off;
        if(c->titlebar_sw)
            xcb_unmap_window(globalconf.connection, c->titlebar_sw->window);
        return;
        return;
      case Top:
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        height = c->titlebar.height;
        break;
      case Left:
      case Right:
        if(!c->titlebar.width)
            height = c->geometry.height + 2 * c->border;
        else
            height = MIN(c->titlebar.width, c->geometry.height);
        width = c->titlebar.height;
        break;
    }

    /* Delete old statusbar */
    simplewindow_delete(&c->titlebar_sw);

    c->titlebar_sw = simplewindow_new(globalconf.connection,
                                      c->phys_screen, 0, 0,
                                      width, height, 0);
    titlebar_draw(c);
    xcb_map_window(globalconf.connection, c->titlebar_sw->window);
}

static int
luaA_titlebar_new(lua_State *L)
{
    titlebar_t **tb;
    int objpos;

    luaA_checktable(L, 1);

    tb = lua_newuserdata(L, sizeof(titlebar_t *));
    *tb = p_new(titlebar_t, 1);
    objpos = lua_gettop(L);

    (*tb)->text_normal = a_strdup(luaA_getopt_string(L, 1, "text_normal", "<title/>"));
    (*tb)->text_focus = a_strdup(luaA_getopt_string(L, 1, "text_focus", "<title/>"));
    (*tb)->text_urgent = a_strdup(luaA_getopt_string(L, 1, "text_urgent", "<title/>"));

    (*tb)->align = draw_align_get_from_str(luaA_getopt_string(L, 1, "align", "left"));

    (*tb)->width = luaA_getopt_number(L, 1, "width", 0);
    (*tb)->height = luaA_getopt_number(L, 1, "height", 0);
    if((*tb)->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        (*tb)->height = 1.5 * globalconf.font->height;

    (*tb)->position = position_get_from_str(luaA_getopt_string(L, 1, "position", "top"));

    titlebar_ref(tb);

    lua_pushvalue(L, objpos);
    return luaA_settype(L, "titlebar");
}

static int
luaA_titlebar_mouse(lua_State *L)
{
    size_t i, len;
    int b;
    button_t *button;

    /* arg 1 is modkey table */
    luaA_checktable(L, 1);
    /* arg 2 is mouse button */
    b = luaL_checknumber(L, 2);
    /* arg 3 is cmd to run */
    luaA_checkfunction(L, 3);

    button = p_new(button_t, 1);
    button->button = xutil_button_fromint(b);
    button->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 1);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 1, i);
        button->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    button_list_push(&globalconf.buttons.titlebar, button);

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
    { "mouse", luaA_titlebar_mouse },
    { NULL, NULL }
};
const struct luaL_reg awesome_titlebar_meta[] =
{
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
