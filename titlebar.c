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

#include "titlebar.h"
#include "client.h"
#include "widget.h"
#include "wibox.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, wibox_t, titlebar, "titlebar", wibox_ref)
DO_LUA_GC(wibox_t, titlebar, "titlebar", wibox_unref)
DO_LUA_EQ(wibox_t, titlebar, "titlebar")

/** Get a client by its titlebar.
 * \param titlebar The titlebar.
 * \return A client.
 */
client_t *
client_getbytitlebar(wibox_t *titlebar)
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
        if(c->titlebar && c->titlebar->sw.window == win)
            return c;

    return NULL;
}

/** Draw the titlebar content.
 * \param c The client.
 */
void
titlebar_draw(client_t *c)
{
    if(!c || !c->titlebar || !c->titlebar->position)
        return;

    widget_render(c->titlebar->widgets, &c->titlebar->sw.ctx,
                  c->titlebar->sw.gc, c->titlebar->sw.pixmap,
                  c->screen, c->titlebar->position,
                  c->titlebar->sw.geometry.x, c->titlebar->sw.geometry.y,
                  c->titlebar, AWESOME_TYPE_TITLEBAR);

    simplewindow_refresh_pixmap(&c->titlebar->sw);

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
            width = MAX(1, MIN(c->titlebar->width, geometry.width - 2 * c->titlebar->sw.border.width));
        else
            width = MAX(1, geometry.width + 2 * c->border - 2 * c->titlebar->sw.border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width - 2 * c->titlebar->sw.border.width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        res->x = geometry.x + x_offset;
        res->y = geometry.y - c->titlebar->height - 2 * c->titlebar->sw.border.width + c->border;
        res->width = width;
        res->height = c->titlebar->height;
        break;
      case Bottom:
        if(c->titlebar->width)
            width = MAX(1, MIN(c->titlebar->width, geometry.width - 2 * c->titlebar->sw.border.width));
        else
            width = MAX(1, geometry.width + 2 * c->border - 2 * c->titlebar->sw.border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width - 2 * c->titlebar->sw.border.width;
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
            width = MAX(1, MIN(c->titlebar->width, geometry.height - 2 * c->titlebar->sw.border.width));
        else
            width = MAX(1, geometry.height + 2 * c->border - 2 * c->titlebar->sw.border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width - 2 * c->titlebar->sw.border.width;
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
            width = MAX(1, MIN(c->titlebar->width, geometry.height - 2 * c->titlebar->sw.border.width));
        else
            width = MAX(1, geometry.height + 2 * c->border - 2 * c->titlebar->sw.border.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width - 2 * c->titlebar->sw.border.width;
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

    /* already had a window? */
    if(c->titlebar->sw.window)
        simplewindow_wipe(&c->titlebar->sw);

    switch(c->titlebar->position)
    {
      default:
        c->titlebar->position = Off;
        return;
      case Top:
      case Bottom:
        if(c->titlebar->width)
            width = MIN(c->titlebar->width, c->geometry.width - 2 * c->titlebar->sw.border.width);
        else
            width = c->geometry.width + 2 * c->border - 2 * c->titlebar->sw.border.width;
        height = c->titlebar->height;
        break;
      case Left:
      case Right:
        if(c->titlebar->width)
            height = MIN(c->titlebar->width, c->geometry.height - 2 * c->titlebar->sw.border.width);
        else
            height = c->geometry.height + 2 * c->border - 2 * c->titlebar->sw.border.width;
        width = c->titlebar->height;
        break;
    }

    titlebar_geometry_compute(c, c->geometry, &geom);

    simplewindow_init(&c->titlebar->sw, c->phys_screen,
                      geom.x, geom.y, geom.width, geom.height,
                      c->titlebar->border.width, c->titlebar->position,
                      &c->titlebar->colors.fg, &c->titlebar->colors.bg);

    simplewindow_border_color_set(&c->titlebar->sw, &c->titlebar->border.color);

    client_need_arrange(c);

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
    wibox_t *tb;
    const char *buf;
    size_t len;
    xcolor_init_request_t reqs[3];
    int8_t i, reqs_nbr = -1;

    luaA_checktable(L, 2);

    tb = p_new(wibox_t, 1);

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
        reqs[++reqs_nbr] = xcolor_init_unchecked(&tb->colors.fg, buf, len);

    tb->colors.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&tb->colors.bg, buf, len);

    tb->border.color = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "border_color", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&tb->border.color, buf, len);

    tb->border.width = luaA_getopt_number(L, 2, "border_width", 0);

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

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
    wibox_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    client_t *c = NULL;
    int i;
    position_t position;

    switch(a_tokenize(attr, len))
    {
      case A_TK_CLIENT:
        if(!lua_isnil(L, 3))
        {
            client_t **newc = luaA_checkudata(L, 3, "client");
            if((*newc)->titlebar)
            {
                xcb_unmap_window(globalconf.connection, (*newc)->titlebar->sw.window);
                wibox_unref(&(*newc)->titlebar);
            }
            /* Attach titlebar to client */
            (*newc)->titlebar = wibox_ref(titlebar);
            titlebar_init(*newc);
            c = *newc;
        }
        else if((c = client_getbytitlebar(*titlebar)))
        {
            xcb_unmap_window(globalconf.connection, (*titlebar)->sw.window);
            /* unref and NULL the ref */
            wibox_unref(&c->titlebar);
            c->titlebar = NULL;
            client_need_arrange(c);
        }
        client_stack();
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
            if(xcolor_init_reply(xcolor_init_unchecked(&(*titlebar)->border.color, buf, len)))
                simplewindow_border_color_set(&c->titlebar->sw, &c->titlebar->border.color);
        return 0;
      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*titlebar)->colors.fg, buf, len)))
            {
                (*titlebar)->sw.ctx.fg = (*titlebar)->colors.fg;
                (*titlebar)->need_update = true;
            }
        return 0;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*titlebar)->colors.bg, buf, len)))
            {
                (*titlebar)->sw.ctx.bg = (*titlebar)->colors.bg;
                (*titlebar)->need_update = true;
            }
        break;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        position = position_fromstr(buf, len);
        if(position != (*titlebar)->position)
        {
            (*titlebar)->position = position;
            c = client_getbytitlebar(*titlebar);
            titlebar_init(c);
        }
        break;
      default:
        return 0;
    }

    if((c || (c = client_getbytitlebar(*titlebar))))
        client_need_arrange(c);

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
 * \lfield position Position.
 */
static int
luaA_titlebar_index(lua_State *L)
{
    size_t len;
    wibox_t **titlebar = luaA_checkudata(L, 1, "titlebar");
    const char *attr = luaL_checklstring(L, 2, &len);
    client_t *c;

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
      case A_TK_POSITION:
        lua_pushstring(L, position_tostr((*titlebar)->position));
        break;
      default:
        return 0;
    }

    return 1;
}

static int
luaA_titlebar_widgets(lua_State *L)
{
    wibox_t **titlebar = luaA_checkudata(L, 1, "titlebar");

    if(lua_gettop(L) == 2)
    {
        luaA_widget_set(L, 2, *titlebar, &(*titlebar)->widgets);
        (*titlebar)->need_update = true;
        return 1;
    }
    return luaA_widget_get(L, (*titlebar)->widgets);
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
    wibox_t **p = luaA_checkudata(L, 1, "titlebar");
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
    { "widgets", luaA_titlebar_widgets },
    { "__index", luaA_titlebar_index },
    { "__newindex", luaA_titlebar_newindex },
    { "__eq", luaA_titlebar_eq },
    { "__gc", luaA_titlebar_gc },
    { "__tostring", luaA_titlebar_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
