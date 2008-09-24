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

#include "structs.h"
#include "wibox.h"

client_t * client_getbytitlebar(wibox_t *);
client_t * client_getbytitlebarwin(xcb_window_t);
void titlebar_geometry_compute(client_t *, area_t, area_t *);
void titlebar_init(client_t *);
void titlebar_client_detach(client_t *);
void titlebar_client_attach(client_t *, wibox_t *);

int luaA_titlebar_newindex(lua_State *, wibox_t *, awesome_token_t);

/** Add the titlebar geometry and border to a geometry.
 * \param t The titlebar
 * \param border The client border size.
 * \param geometry The geometry
 * \return A new geometry bigger if the titlebar is visible.
 */
static inline area_t
titlebar_geometry_add(wibox_t *t, int border, area_t geometry)
{
    if(t)
        switch(t->position)
        {
          case Top:
            geometry.y -= t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.height += t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.width += 2 * border;
            break;
          case Bottom:
            geometry.height += t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.width += 2 * border;
            break;
          case Left:
            geometry.x -= t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.width += t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.height += 2 * border;
            break;
          case Right:
            geometry.width += t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.height += 2 * border;
            break;
          default:
            break;
        }
    else
    {
        geometry.width += 2 * border;
        geometry.height += 2 * border;
    }

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
    if(t)
        switch(t->position)
        {
          case Top:
            geometry.y += t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.height -= t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.width -= 2 * border;
            break;
          case Bottom:
            geometry.height -= t->sw.geometry.height + 2 * t->sw.border.width - border;
            geometry.width -= 2 * border;
            break;
          case Left:
            geometry.x += t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.width -= t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.height -= 2 * border;
            break;
          case Right:
            geometry.width -= t->sw.geometry.width + 2 * t->sw.border.width - border;
            geometry.height -= 2 * border;
            break;
          default:
            break;
        }
    else
    {
        geometry.width -= 2 * border;
        geometry.height -= 2 * border;
    }

    return geometry;
}

/** Update the titlebar geometry for a tiled client.
 * \param c The client.
 * \param geometry The geometry the client will receive.
 */
static inline void
titlebar_update_geometry_tiled(client_t *c, area_t geometry)
{
    area_t geom;

    if(!c->titlebar)
        return;

    titlebar_geometry_compute(c, geometry, &geom);
    wibox_moveresize(c->titlebar, geom);
}

/** Update the titlebar geometry for a floating client.
 * \param c The client.
 */
static inline void
titlebar_update_geometry_floating(client_t *c)
{
    return titlebar_update_geometry_tiled(c, c->geometry);
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
