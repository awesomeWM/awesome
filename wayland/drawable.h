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

#ifndef AWESOME_WAYLAND_DRAWABLE_H
#define AWESOME_WAYLAND_DRAWABLE_H

#include "objects/drawable.h"

struct wayland_drawable
{
    struct wl_surface *wl_surface;
    struct wl_buffer *buffer;
    void *shm_data;
	size_t shm_size;
};

xcb_pixmap_t wayland_get_pixmap(struct drawable_t *drawable);

void wayland_drawable_allocate(struct drawable_t *drawable);
void wayland_drawable_cleanup(struct drawable_t *drawable);
void wayland_drawable_unset_surface(struct drawable_t *drawable);
void wayland_drawable_wipe(struct drawable_t *drawable);
void wayland_drawable_create_buffer(struct drawable_t *drawable);

#endif // AWESOME_WAYLAND_DRAWABLE_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
