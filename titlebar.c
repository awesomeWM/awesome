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
                  c->titlebar);

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

/** Create a new titlebar (DEPRECATED).
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
    deprecate();

    return luaA_wibox_new(L);
}

/** Detach a wibox titlebar from its client.
 * \param c The client.
 */
void
titlebar_client_detach(client_t *c)
{
    /* If client has a titlebar, kick it out. */
    if(c && c->titlebar)
    {
        wibox_unref(&c->titlebar);
        simplewindow_wipe(&c->titlebar->sw);
        c->titlebar->type = WIBOX_TYPE_NONE;
        c->titlebar = NULL;
        client_need_arrange(c);
        client_stack();
    }
}

/** Attach a wibox to a client as its titlebar.
 * \param c The client.
 * \param t The wibox/titlebar.
 */
void
titlebar_client_attach(client_t *c, wibox_t *t)
{
    titlebar_client_detach(c);

    if(c && t)
    {
        /* check if titlebar is already on a client */
        titlebar_client_detach(client_getbytitlebar(t));
        c->titlebar = wibox_ref(&t);
        t->type = WIBOX_TYPE_TITLEBAR;
        titlebar_init(c);
        client_need_arrange(c);
        client_stack();
    }
}

/** Titlebar newindex.
 * \param L The Lua VM state.
 * \param titlebar The wibox titlebar.
 * \param tok The attribute token.
 * \return The number of elements pushed on stack.
 */
int
luaA_titlebar_newindex(lua_State *L, wibox_t *titlebar, awesome_token_t tok)
{
    size_t len;
    const char *buf;
    client_t *c = NULL;
    int i;
    position_t position;

    switch(tok)
    {
      case A_TK_ALIGN:
        if((buf = luaL_checklstring(L, 3, &len)))
            titlebar->align = draw_align_fromstr(buf, len);
        else
            return 0;
        break;
      case A_TK_BORDER_WIDTH:
        if((i = luaL_checknumber(L, 3)) >= 0)
            titlebar->border.width = i;
        else
            return 0;
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&titlebar->border.color, buf, len)))
                simplewindow_border_color_set(&c->titlebar->sw, &c->titlebar->border.color);
        return 0;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        position = position_fromstr(buf, len);
        if(position != titlebar->position)
        {
            titlebar->position = position;
            c = client_getbytitlebar(titlebar);
            titlebar_init(c);
        }
        break;
      default:
        return 0;
    }

    if((c || (c = client_getbytitlebar(titlebar))))
        client_need_arrange(c);

    return 0;
}

const struct luaL_reg awesome_titlebar_methods[] =
{
    { "__call", luaA_titlebar_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_titlebar_meta[] =
{
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
