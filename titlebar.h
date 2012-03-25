/*
 * titlebar.h - titlebar management header
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

#ifndef AWESOME_TITLEBAR_H
#define AWESOME_TITLEBAR_H

#include "wibox.h"
#include "client.h"
#include "window.h"

client_t * client_getbytitlebar(wibox_t *);
client_t * client_getbytitlebarwin(xcb_window_t);
void titlebar_geometry_compute(client_t *, area_t, area_t *);
void titlebar_init(client_t *);
void titlebar_client_detach(client_t *);
void titlebar_client_attach(client_t *);
void titlebar_set_visible(wibox_t *, bool);
void titlebar_ban(wibox_t *);
void titlebar_unban(wibox_t *);

int luaA_titlebar_set_position(lua_State *, int);

static inline bool
titlebar_isvisible(client_t *c, screen_t *screen)
{
    if(client_isvisible(c, screen))
    {
        if(c->fullscreen)
            return false;
        if(!c->titlebar || !c->titlebar->visible)
            return false;
        return true;
    }
    return false;
}

/** Add the titlebar geometry and border to a geometry.
 * \param t The titlebar
 * \param border The client border size.
 * \param geometry The geometry
 * \return A new geometry bigger if the titlebar is visible.
 */
static inline area_t
titlebar_geometry_add(wibox_t *t, int border, area_t geometry)
{
    /* We need to add titlebar border to the total width and height.
     * This can then be subtracted/added to the width/height/x/y.
     * In this case the border is included, because it belongs to a different window.
     */
    if(t && t->visible)
        switch(t->position)
        {
          case Top:
            geometry.y -= t->geometry.height;
            geometry.height += t->geometry.height;
            break;
          case Bottom:
            geometry.height += t->geometry.height;
            break;
          case Left:
            geometry.x -= t->geometry.width;
            geometry.width += t->geometry.width;
            break;
          case Right:
            geometry.width += t->geometry.width;
            break;
          default:
            break;
        }

    /* Adding a border to a client only changes width and height, x and y are including border. */
    geometry.width += 2 * border;
    geometry.height += 2 * border;

    return geometry;
}

/** Remove the titlebar geometry and border width to a geometry.
 * \param t The titlebar.
 * \param border The client border size.
 * \param geometry The geometry.
 * \return A new geometry smaller if the titlebar is visible.
 */
static inline area_t
titlebar_geometry_remove(wibox_t *t, int border, area_t geometry)
{
    /* We need to add titlebar border to the total width and height.
     * This can then be subtracted/added to the width/height/x/y.
     * In this case the border is included, because it belongs to a different window.
     */
    if(t && t->visible)
        switch(t->position)
        {
          case Top:
            geometry.y += t->geometry.height;
            unsigned_subtract(geometry.height, t->geometry.height);
            break;
          case Bottom:
            unsigned_subtract(geometry.height, t->geometry.height);
            break;
          case Left:
            geometry.x += t->geometry.width;
            unsigned_subtract(geometry.width, t->geometry.width);
            break;
          case Right:
            unsigned_subtract(geometry.width, t->geometry.width);
            break;
          default:
            break;
        }

    /* Adding a border to a client only changes width and height, x and y are including border. */
    unsigned_subtract(geometry.width, 2*border);
    unsigned_subtract(geometry.height, 2*border);

    return geometry;
}

/** Update the titlebar geometry for a client.
 * \param c The client.
 */
static inline void
titlebar_update_geometry(client_t *c)
{
    area_t geom;

    if(!c->titlebar)
        return;

    /* Client geometry without titlebar, but including borders, since that is always consistent. */
    titlebar_geometry_compute(c, titlebar_geometry_remove(c->titlebar, 0, c->geometry), &geom);
    luaA_object_push(globalconf.L, c->titlebar);
    wibox_moveresize(globalconf.L, -1, geom);
    lua_pop(globalconf.L, 1);
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
