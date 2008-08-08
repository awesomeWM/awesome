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
#include "widget.h"

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
                               &c->titlebar->colors.fg,
                               &c->titlebar->colors.bg);
        break;
      default:
        ctx = draw_context_new(globalconf.connection, c->titlebar->sw->phys_screen,
                               c->titlebar->sw->geometry.width,
                               c->titlebar->sw->geometry.height,
                               c->titlebar->sw->pixmap,
                               &c->titlebar->colors.fg,
                               &c->titlebar->colors.bg);
        break;
    }

    widget_render(c->titlebar->widgets, ctx, c->titlebar->sw->gc, c->titlebar->sw->pixmap,
                  c->screen, c->titlebar->position,
                  c->titlebar->sw->geometry.x, c->titlebar->sw->geometry.y,
                  c->titlebar, AWESOME_TYPE_TITLEBAR);

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

    c->titlebar->need_update = false;
}

/** Titlebar refresh function.
 */
void
titlebar_refresh(void)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar && c->titlebar->need_update)
            titlebar_draw(c);
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

    c->titlebar->need_update = true;
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
    const char *buf;
    size_t len;

    luaA_checktable(L, 2);

    tb = p_new(titlebar_t, 1);

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    tb->align = draw_align_fromstr(buf, len);

    tb->width = luaA_getopt_number(L, 2, "width", 0);
    tb->height = luaA_getopt_number(L, 2, "height", 0);
    if(tb->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        tb->height = 1.5 * globalconf.font->height;

    buf = luaA_getopt_lstring(L, 2, "position", "top", &len);
    tb->position = position_fromstr(buf, len);

    tb->colors.fg = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "fg", NULL, &len)))
        xcolor_init(&tb->colors.fg, globalconf.connection,
                    globalconf.default_screen, buf, len);

    tb->colors.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        xcolor_init(&tb->colors.bg, globalconf.connection,
                    globalconf.default_screen, buf, len);

    tb->border.color = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "border_color", NULL, &len)))
        xcolor_init(&tb->border.color, globalconf.connection,
                    globalconf.default_screen, buf, len);

    tb->border.width = luaA_getopt_number(L, 2, "border_width", 0);

    return luaA_titlebar_userdata_new(globalconf.L, tb);
}

/** Titlebar newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_titlebar_newindex(lua_State *L)
{
    size_t len;
    titlebar_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    client_t *c = NULL, **newc;
    int i;
    widget_node_t *witer;

    switch(a_tokenize(attr, len))
    {
      case A_TK_CLIENT:
        if(!lua_isnil(L, 3))
            newc = luaA_checkudata(L, 3, "client");
        else
            newc = NULL;

        if(newc)
        {
            if((*newc)->titlebar)
            {
                simplewindow_delete(&(*newc)->titlebar->sw);
                titlebar_unref(&(*newc)->titlebar);
                globalconf.screens[(*newc)->screen].need_arrange = true;
            }
            /* Attach titlebar to client */
            (*newc)->titlebar = *titlebar;
            titlebar_ref(titlebar);
            titlebar_init(*newc);
            c = *newc;
        }
        else
        {
            if((c = client_getbytitlebar(*titlebar)))
            {
                simplewindow_delete(&(*titlebar)->sw);
                /* unref and NULL the ref */
                titlebar_unref(&c->titlebar);
                globalconf.screens[c->screen].need_arrange = true;
            }
        }
        break;
      case A_TK_ALIGN:
        if((buf = luaL_checklstring(L, 3, &len)))
            (*titlebar)->align = draw_align_fromstr(buf, len);
        else
            return 0;
        break;
      case A_TK_BORDER_WIDTH:
        if((i = luaL_checknumber(L, 3)) >= 0)
            (*titlebar)->border.width = i;
        else
            return 0;
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init(&(*titlebar)->border.color, globalconf.connection,
                           globalconf.default_screen, buf, len))
                if((*titlebar)->sw)
                    xcb_change_window_attributes(globalconf.connection, (*titlebar)->sw->window,
                                                 XCB_CW_BORDER_PIXEL, &(*titlebar)->border.color.pixel);
        return 0;
      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init(&(*titlebar)->colors.fg, globalconf.connection,
                          globalconf.default_screen, buf, len))
                (*titlebar)->need_update = true;
        return 0;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init(&(*titlebar)->colors.bg, globalconf.connection,
                           globalconf.default_screen, buf, len))
                (*titlebar)->need_update = true;
        return 0;
      case A_TK_WIDGETS:
        luaA_checktable(L, 3);

        /* remove all widgets */
        for(witer = (*titlebar)->widgets; witer; witer = (*titlebar)->widgets)
        {
            if(witer->widget->detach)
                witer->widget->detach(witer->widget, *titlebar);
            widget_unref(&witer->widget);
            widget_node_list_detach(&(*titlebar)->widgets, witer);
            p_delete(&witer);
        }

        (*titlebar)->need_update = true;

        /* now read all widgets and add them */
        lua_pushnil(L);
        while(lua_next(L, 3))
        {
            widget_t **widget = luaA_checkudata(L, -1, "widget");
            widget_node_t *w = p_new(widget_node_t, 1);
            w->widget = *widget;
            widget_node_list_append(&(*titlebar)->widgets, w);
            widget_ref(widget);
            lua_pop(L, 1);
        }
        break;
      default:
        return 0;
    }

    if((c || (c = client_getbytitlebar(*titlebar)))
       && client_isvisible(c, c->screen))
        globalconf.screens[c->screen].need_arrange = true;

    return 0;
}

/** Titlebar object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield client The client attached to this titlebar.
 * \lfield align Alignment relative to the client.
 * \lfield border_width Border width.
 * \lfield border_color Border color.
 * \lfield fg Foreground color.
 * \lfield bg Background color.
 */
static int
luaA_titlebar_index(lua_State *L)
{
    size_t len;
    titlebar_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    const char *attr = luaL_checklstring(L, 2, &len);
    client_t *c;
    widget_node_t *witer;
    int i = 0;

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(attr, len))
    {
      case A_TK_CLIENT:
        if((c = client_getbytitlebar(*titlebar)))
            return luaA_client_userdata_new(L, c);
        else
            return 0;
      case A_TK_ALIGN:
        lua_pushstring(L, draw_align_tostr((*titlebar)->align));
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, (*titlebar)->border.width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &(*titlebar)->border.color);
        break;
      case A_TK_FG:
        luaA_pushcolor(L, &(*titlebar)->colors.fg);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &(*titlebar)->colors.bg);
        break;
      case A_TK_WIDGETS:
        lua_newtable(L);
        for(witer = (*titlebar)->widgets; witer; witer = witer->next)
        {
            luaA_widget_userdata_new(L, witer->widget);
            lua_rawseti(L, -2, ++i);
        }
        break;
      default:
        return 0;
    }

    return 1;
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
    { "__call", luaA_titlebar_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_titlebar_meta[] =
{
    { "__index", luaA_titlebar_index },
    { "__newindex", luaA_titlebar_newindex },
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
