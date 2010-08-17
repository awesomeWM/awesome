/*
 * titlebar.c - titlebar management
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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
#include "luaa.h"

/** Get a client by its titlebar.
 * \param titlebar The titlebar.
 * \return A client.
 */
client_t *
client_getbytitlebar(wibox_t *titlebar)
{
    foreach(c, globalconf.clients)
        if((*c)->titlebar == titlebar)
            return *c;

    return NULL;
}

/** Get a client by its titlebar window.
 * \param win The window.
 * \return A client.
 */
client_t *
client_getbytitlebarwin(xcb_window_t win)
{
    foreach(c, globalconf.clients)
        if((*c)->titlebar && (*c)->titlebar->window == win)
            return *c;

    return NULL;
}

/** Move a titlebar out of the viewport.
 * \param titlebar The titlebar.
 */
void
titlebar_ban(wibox_t *titlebar)
{
    /* Do it manually because client geometry remains unchanged. */
    if(titlebar && !titlebar->isbanned)
    {
        client_t *c;

        if(titlebar->window)
            xcb_unmap_window(globalconf.connection, titlebar->window);

        /* Remove titlebar geometry from client. */
        if((c = client_getbytitlebar(titlebar)))
            c->geometry = titlebar_geometry_remove(titlebar, 0, c->geometry);

        titlebar->isbanned = true;
    }
}

/** Move a titlebar on top of its client.
 * \param titlebar The titlebar.
 */
void
titlebar_unban(wibox_t *titlebar)
{
    /* Do this manually because the system doesn't know we moved the toolbar.
     * Note that !visible titlebars are unmapped and for fullscreen it'll
     * end up offscreen anyway. */
    if(titlebar && titlebar->isbanned)
    {
        client_t *c;

        if(titlebar->window)
            xcb_map_window(globalconf.connection, titlebar->window);

        titlebar->isbanned = false;

        /* Add titlebar geometry from client. */
        if((c = client_getbytitlebar(titlebar)))
            c->geometry = titlebar_geometry_add(titlebar, 0, c->geometry);
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
        res->y = geometry.y - c->titlebar->geometry.height;
        res->width = width;
        res->height = c->titlebar->geometry.height;
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
        res->height = c->titlebar->geometry.height;
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
        res->x = geometry.x - c->titlebar->geometry.width;
        res->y = geometry.y + y_offset;
        res->width = c->titlebar->geometry.width;
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
        res->width = c->titlebar->geometry.width;
        res->height = height;
        break;
    }
}

static void
workaround_broken_titlebars(client_t *c)
{
    /* Ugly hack to fix #610 */
    if(c->maximized_horizontal)
    {
        c->maximized_horizontal = false;
        client_set_maximized_horizontal(globalconf.L, -1, true);
    }
    if(c->maximized_vertical)
    {
        c->maximized_vertical = false;
        client_set_maximized_vertical(globalconf.L, -1, true);
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
        wibox_wipe(c->titlebar);
        c->titlebar->type = WIBOX_TYPE_NORMAL;
        c->titlebar->screen = NULL;

        luaA_object_unref(globalconf.L, c->titlebar);
        c->titlebar = NULL;

        hook_property(c, "titlebar");
        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "property::titlebar", 0);

        workaround_broken_titlebars(c);

        lua_pop(globalconf.L, 1);
        client_stack();
    }
}

/** Attach a wibox to a client as its titlebar.
 * \param c The client.
 */
void
titlebar_client_attach(client_t *c)
{
    /* check if we can register the object */
    wibox_t *t = luaA_object_ref_class(globalconf.L, -1, &wibox_class);

    titlebar_client_detach(c);

    /* check if titlebar is already on a client */
    titlebar_client_detach(client_getbytitlebar(t));

    /* check if client already has a titlebar. */
    titlebar_client_detach(c);

    /* set the object as new client's titlebar */
    c->titlebar = t;

    t->type = WIBOX_TYPE_TITLEBAR;
    t->screen = c->screen;

    switch(t->position)
    {
      case Top:
      case Bottom:
        if(!t->geometry.height)
            t->geometry.height = 1.5 * globalconf.font->height;
        break;
      case Left:
      case Right:
        if(!t->geometry.width)
            t->geometry.width = 1.5 * globalconf.font->height;
        break;
    }

    /* Update client geometry to include the titlebar. */
    c->geometry = titlebar_geometry_add(c->titlebar, 0, c->geometry);

    /* Client geometry without titlebar, but including borders, since that is always consistent. */
    titlebar_geometry_compute(c, titlebar_geometry_remove(c->titlebar, 0, c->geometry), &t->geometry);

    wibox_init(t, c->phys_screen);

    t->need_update = true;

    /* Call update geometry. This will move the wibox to the right place,
     * which might be the same as `wingeom', but then it will ban the
     * titlebar if needed. */
    titlebar_update_geometry(c);

    xcb_map_window(globalconf.connection, t->window);

    hook_property(c, "titlebar");

    luaA_object_push(globalconf.L, c);
    luaA_object_emit_signal(globalconf.L, -1, "property::titlebar", 0);

    workaround_broken_titlebars(c);

    lua_pop(globalconf.L, 1);
    client_stack();
}

/** Map or unmap a titlebar wibox.
 * \param t The wibox/titlebar.
 * \param visible The new state of the titlebar.
 */
void
titlebar_set_visible(wibox_t *t, bool visible)
{
    if(visible != t->visible)
    {
        if((t->visible = visible))
            titlebar_unban(t);
        else
            titlebar_ban(t);

        hook_property(t, "visible");
        client_stack();
    }
}

int
luaA_titlebar_set_position(lua_State *L, int udx)
{
    wibox_t *titlebar = luaA_checkudata(L, udx, &wibox_class);
    size_t len;
    const char *buf = luaL_checklstring(L, -1, &len);
    position_t position = position_fromstr(buf, len);
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
                tmp = titlebar->geometry.width;
                titlebar->geometry.width = titlebar->geometry.height;
                titlebar->geometry.height = tmp;
                break;
            }
            wibox_set_orientation(L, udx, North);
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
                tmp = titlebar->geometry.width;
                titlebar->geometry.width = titlebar->geometry.height;
                titlebar->geometry.height = tmp;
                break;
            }
            wibox_set_orientation(L, udx, South);
            break;
          case Top:
          case Bottom:
            switch(titlebar->position)
            {
                int tmp;
              case Left:
              case Right:
                tmp = titlebar->geometry.width;
                titlebar->geometry.width = titlebar->geometry.height;
                titlebar->geometry.height = tmp;
                break;
              case Top:
              case Bottom:
                break;
            }
            wibox_set_orientation(L, udx, East);
            break;
        }
        titlebar->position = position;
        client_t *c;
        if((c = client_getbytitlebar(titlebar)))
        {
            titlebar_update_geometry(c);
            /* call geometry hook for client because some like to
             * set titlebar width in that hook, which make sense */
            hook_property(c, "geometry");
        }
    }
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
