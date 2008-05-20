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

void titlebar_draw(client_t *);
void titlebar_update_geometry_floating(client_t *);
void titlebar_update_geometry(client_t *, area_t);
void titlebar_init(client_t *);

/** Add the titlebar geometry to a geometry.
 * \param t the titlebar
 * \param geometry the geometry
 * \return a new geometry bigger if the titlebar is visible
 */
static inline area_t
titlebar_geometry_add(titlebar_t *t, area_t geometry)
{
    switch(t->position)
    {
      case Top:
        geometry.y -= t->height;
        geometry.height += t->height;
        break;
      case Bottom:
        geometry.height += t->height;
        break;
      case Left:
        geometry.x -= t->width;
        geometry.width += t->width;
        break;
      case Right:
        geometry.width += t->width;
        break;
      default:
        break;
    }

    return geometry;
}

/** Remove the titlebar geometry to a geometry.
 * \param t the titlebar
 * \param geometry the geometry
 * \return a new geometry smaller if the titlebar is visible
 */
static inline area_t
titlebar_geometry_remove(titlebar_t *t, area_t geometry)
{
    switch(t->position)
    {
      case Top:
        geometry.y += t->height;
        geometry.height -= t->height;
        break;
      case Bottom:
        geometry.height -= t->height;
        break;
      case Left:
        geometry.x += t->width;
        geometry.width -= t->width;
        break;
      case Right:
        geometry.width -= t->width;
        break;
      default:
        break;
    }

    return geometry;
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
