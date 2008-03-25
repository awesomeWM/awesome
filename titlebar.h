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

void titlebar_init(Client *);
void titlebar_update(Client *);
void titlebar_update_geometry_floating(Client *);
area_t titlebar_update_geometry(Client *, area_t);
area_t titlebar_geometry_add(Titlebar *, area_t);
area_t titlebar_geometry_remove(Titlebar *, area_t);

Uicb uicb_client_toggletitlebar;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
