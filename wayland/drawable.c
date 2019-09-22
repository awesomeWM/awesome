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

#include "drawable.h"

#include "wayland/utils.h"

#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1.h"
#include "globalconf.h"


xcb_pixmap_t wayland_get_pixmap(struct drawable_t *drawable)
{
    fatal("Attempted to get an X11 pixmap while using Wayland");
}

void wayland_drawable_allocate(struct drawable_t *drawable)
{
    drawable->impl_data = calloc(1, sizeof(struct wayland_drawable));

    struct wayland_drawable *wayland_drawable = drawable->impl_data;
    wayland_drawable->wl_surface =
        wl_compositor_create_surface(globalconf.wl_compositor);
}

void wayland_drawable_cleanup(struct drawable_t *drawable)
{
    wayland_drawable_unset_surface(drawable);

    struct wayland_drawable *wayland_drawable = drawable->impl_data;
    if (wayland_drawable->wl_surface != NULL)
        wl_surface_destroy(wayland_drawable->wl_surface);

    free(drawable->impl_data);
    drawable->impl_data = NULL;
}

void wayland_drawable_create_buffer(struct drawable_t *drawable)
{
    struct wayland_drawable *wayland_drawable = drawable->impl_data;

    if (drawable->geometry.width == 0 || drawable->geometry.height == 0)
        return;

    int stride = 0;
    wayland_drawable_unset_surface(drawable);
    wayland_setup_buffer(drawable->geometry, &wayland_drawable->buffer,
            &stride, &wayland_drawable->shm_data, &wayland_drawable->shm_size);

    drawable->surface =
        cairo_image_surface_create_for_data(wayland_drawable->shm_data,
                CAIRO_FORMAT_ARGB32, drawable->geometry.width,
                drawable->geometry.height,
                stride);
}

void wayland_drawable_unset_surface(struct drawable_t *drawable)
{
    struct wayland_drawable *wayland_drawable = drawable->impl_data;

    if (wayland_drawable->wl_surface != NULL && wayland_drawable->buffer != NULL)
    {
        wl_surface_attach(wayland_drawable->wl_surface, NULL, 0, 0);
        wl_buffer_destroy(wayland_drawable->buffer);
        wayland_drawable->buffer = NULL;
    }

}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
