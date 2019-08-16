/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#ifndef AWESOME_X11_DRAWABLE_H
#define AWESOME_X11_DRAWABLE_H

#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>

#include "objects/drawable.h"

struct x11_drawable
{
	xcb_pixmap_t pixmap;
};

xcb_pixmap_t x11_get_pixmap(struct drawable_t *drawable);

void x11_drawable_allocate(struct drawable_t *drawable);
void x11_drawable_unset_surface(struct drawable_t *drawable);
void x11_drawable_wipe(struct drawable_t *drawable);
void x11_drawable_create_pixmap(struct drawable_t *drawable);
void x11_drawable_cleanup(struct drawable_t *drawable);

#endif  // AWESOME_X11_DRAWABLE_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
