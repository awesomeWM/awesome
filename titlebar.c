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
#include "screen.h"

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

/** Move a titlebar out of the viewport.
 * \param titlebar The titlebar.
 */
void
titlebar_ban(wibox_t *titlebar)
{
    /* Do it manually because client geometry remains unchanged. */
    if(titlebar)
    {
        simple_window_t *sw = &titlebar->sw;

        if(sw->window)
        {
            uint32_t request[] = { - sw->geometry.width, - sw->geometry.height };
            /* Move the titlebar to the same place as the window. */
            xcb_configure_window(globalconf.connection, sw->window,
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                 request);
        }
    }
}

/** Get titlebar area.
 * \param c The client
 * \param geometry The client geometry including borders, excluding titlebars.
 * \param res Pointer to area of titlebar, must be allocated already.
 */
void
titlebar_geometry_compute(client_t *c, area_t geometry, area_t *res)
{
    int height, width, x_offset = 0, y_offset = 0;

    switch(c->titlebar->position)
    {
      default:
        return;
      case Top:
        width = MAX(1, geometry.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        res->x = geometry.x + x_offset;
        res->y = geometry.y - c->titlebar->sw.geometry.height;
        res->width = width;
        res->height = c->titlebar->sw.geometry.height;
        break;
      case Bottom:
        width = MAX(1, geometry.width);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            x_offset = geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        res->x = geometry.x + x_offset;
        res->y = geometry.y + geometry.height;
        res->width = width;
        res->height = c->titlebar->sw.geometry.height;
        break;
      case Left:
        height = MAX(1, geometry.height);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = geometry.height - height;
            break;
          case AlignCenter:
            y_offset = (geometry.height - height) / 2;
            break;
        }
        res->x = geometry.x - c->titlebar->sw.geometry.width;
        res->y = geometry.y + y_offset;
        res->width = c->titlebar->sw.geometry.width;
        res->height = height;
        break;
      case Right:
        height = MAX(1, geometry.height);
        switch(c->titlebar->align)
        {
          default:
            break;
          case AlignRight:
            y_offset = geometry.height - height;
            break;
          case AlignCenter:
            y_offset = (geometry.height - height) / 2;
            break;
        }
        res->x = geometry.x + geometry.width;
        res->y = geometry.y + y_offset;
        res->width = c->titlebar->sw.geometry.width;
        res->height = height;
        break;
    }
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
        /* Update client geometry to exclude the titlebar. */
        c->geometry = titlebar_geometry_remove(c->titlebar, 0, c->geometry);
        simplewindow_wipe(&c->titlebar->sw);
        c->titlebar->type = WIBOX_TYPE_NORMAL;
        c->titlebar->screen = SCREEN_UNDEF;
        wibox_unref(&c->titlebar);
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
        area_t wingeom;

        /* check if titlebar is already on a client */
        titlebar_client_detach(client_getbytitlebar(t));

        /* check if client already has a titlebar. */
        titlebar_client_detach(c);

        c->titlebar = wibox_ref(&t);
        t->type = WIBOX_TYPE_TITLEBAR;
        t->screen = c->screen;

        switch(t->position)
        {
          case Floating:
            t->position = Top;
          case Top:
          case Bottom:
            if(!t->sw.geometry.height)
                t->sw.geometry.height = 1.5 * globalconf.font->height;
            break;
          case Left:
          case Right:
            if(!t->sw.geometry.width)
                t->sw.geometry.width = 1.5 * globalconf.font->height;
            break;
        }

        /* Update client geometry to include the titlebar. */
        c->geometry = titlebar_geometry_add(c->titlebar, 0, c->geometry);

        /* Client geometry without titlebar, but including borders, since that is always consistent. */
        titlebar_geometry_compute(c, titlebar_geometry_remove(c->titlebar, 0, c->geometry), &wingeom);

        simplewindow_init(&t->sw, c->phys_screen,
                          wingeom, 0, t->sw.orientation,
                          &t->sw.ctx.fg, &t->sw.ctx.bg);
        simplewindow_border_color_set(&t->sw, &t->sw.border.color);

        t->need_update = true;

        /* Call update geometry. This will move the wibox to the right place,
         * which might be the same as `wingeom', but then it will ban the
         * titlebar if needed. */
        titlebar_update_geometry(c);

        if(t->isvisible)
            xcb_map_window(globalconf.connection, t->sw.window);

        client_need_arrange(c);
        client_stack();
    }
}

/** Map or unmap a titlebar wibox.
 * \param t The wibox/titlebar.
 * \param visible The new state of the titlebar.
 */
void
titlebar_set_visible(wibox_t *t, bool visible)
{
    if (visible != t->isvisible)
    {
        /* The price of (un)mapping something small like a titlebar is pretty cheap.
         * It would complicate matters if this rare case was treated like clients.
         * Clients are moved out of the viewport when banned.
         */
        if ((t->isvisible = visible))
            xcb_map_window(globalconf.connection, t->sw.window);
        else
            xcb_unmap_window(globalconf.connection, t->sw.window);

        globalconf.screens[t->screen].need_arrange = true;
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
    client_t *c = NULL;

    switch(tok)
    {
        position_t position;
        int i;
        size_t len;
        const char *buf;

      case A_TK_ALIGN:
        if((buf = luaL_checklstring(L, 3, &len)))
            titlebar->align = draw_align_fromstr(buf, len);
        else
            return 0;
        break;
      case A_TK_BORDER_WIDTH:
        if((i = luaL_checknumber(L, 3)) >= 0)
            simplewindow_border_width_set(&titlebar->sw, i);
        else
            return 0;
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&titlebar->sw.border.color, buf, len)))
                simplewindow_border_color_set(&titlebar->sw, &titlebar->sw.border.color);
        return 0;
      case A_TK_POSITION:
        buf = luaL_checklstring(L, 3, &len);
        position = position_fromstr(buf, len);
        if(position != titlebar->position)
        {
            switch(position)
            {
              case Left:
                switch(titlebar->position)
                {
                    int tmp;
                  case Left:
                  case Right:
                    break;
                  case Top:
                  case Bottom:
                  case Floating:
                    tmp = titlebar->sw.geometry.width;
                    titlebar->sw.geometry.width = titlebar->sw.geometry.height;
                    titlebar->sw.geometry.height = tmp;
                    break;
                }
                simplewindow_orientation_set(&titlebar->sw, North);
                break;
              case Right:
                switch(titlebar->position)
                {
                    int tmp;
                  case Left:
                  case Right:
                    break;
                  case Top:
                  case Bottom:
                  case Floating:
                    tmp = titlebar->sw.geometry.width;
                    titlebar->sw.geometry.width = titlebar->sw.geometry.height;
                    titlebar->sw.geometry.height = tmp;
                    break;
                }
                simplewindow_orientation_set(&titlebar->sw, South);
                break;
              case Top:
              case Bottom:
              case Floating:
                switch(titlebar->position)
                {
                    int tmp;
                  case Left:
                  case Right:
                    tmp = titlebar->sw.geometry.width;
                    titlebar->sw.geometry.width = titlebar->sw.geometry.height;
                    titlebar->sw.geometry.height = tmp;
                    break;
                  case Top:
                  case Bottom:
                  case Floating:
                    break;
                }
                simplewindow_orientation_set(&titlebar->sw, East);
                break;
            }
            titlebar->position = position;
            if((c = client_getbytitlebar(titlebar)))
            {
                titlebar_update_geometry(c);
                /* call geometry hook for client because some like to
                 * set titlebar width in that hook, which make sense */
                hooks_property(c, "geometry");
            }
        }
        break;
      default:
        return 0;
    }

    if((c || (c = client_getbytitlebar(titlebar))))
        client_need_arrange(c);

    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
