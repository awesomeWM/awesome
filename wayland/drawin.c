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
#include "wayland/drawin.h"

#include <stdint.h>

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1.h"

#include "wayland/drawable.h"
#include "globalconf.h"

static void layer_surface_configure(void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial, uint32_t w, uint32_t h)
{
    struct drawin_t *drawin = data;
    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;

    drawin->geometry.width = w;
    drawin->geometry.height = h;

    drawin_apply_moveresize(drawin);

    zwlr_layer_surface_v1_ack_configure(surface, serial);
    wayland_drawin->configured = true;

    if (wayland_drawin->mapped)
        wl_surface_attach(wayland_drawable->wl_surface,
                wayland_drawable->buffer, 0, 0);
}

static void layer_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *surface)
{
    struct drawin_t *drawin = data;
    struct wayland_drawin *wayland_drawin = drawin->impl_data;

	zwlr_layer_surface_v1_destroy(wayland_drawin->layer_surface);
    wayland_drawable_cleanup(drawin->drawable);

    wayland_drawin->layer_surface = NULL;
}

struct zwlr_layer_surface_v1_listener layer_surface_listener =
{
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

xcb_window_t wayland_get_xcb_window(struct drawin_t *drawin)
{
    fatal("Attempted to get an xcb window while using Wayland\n");
}

drawin_t *wayland_get_drawin_by_window(xcb_window_t win)
{
    fatal("Attempted to get a drawin using a xcb_window_t while using Wayland");
}

void wayland_drawin_allocate(struct drawin_t *drawin)
{
    drawin->impl_data = calloc(1, sizeof(struct wayland_drawin));

    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;
    struct zwlr_layer_shell_v1 *layer_shell = globalconf.layer_shell;

    assert(wayland_drawable->wl_surface);

    wayland_drawin->layer_surface =
        zwlr_layer_shell_v1_get_layer_surface(layer_shell,
                wayland_drawable->wl_surface, NULL,
                ZWLR_LAYER_SHELL_V1_LAYER_TOP, "awesome");
    zwlr_layer_surface_v1_set_size(wayland_drawin->layer_surface,
            drawin->geometry.width, drawin->geometry.height);
	zwlr_layer_surface_v1_set_keyboard_interactivity(
			wayland_drawin->layer_surface, true);
	zwlr_layer_surface_v1_add_listener(wayland_drawin->layer_surface,
			&layer_surface_listener, drawin);

    // XXX This is to get around not being able to set explicit x/y coordinates.
    // We anchor to the top left and then use the margins to set ourselves.
    zwlr_layer_surface_v1_set_anchor(wayland_drawin->layer_surface,
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    wayland_drawin->configured = false;
    wayland_drawin->mapped = false;

	wl_surface_commit(wayland_drawable->wl_surface);
	wl_display_roundtrip(globalconf.wl_display);
}

void wayland_drawin_cleanup(struct drawin_t *drawin)
{
    struct wayland_drawin *wayland_drawin = drawin->impl_data;

    zwlr_layer_surface_v1_destroy(wayland_drawin->layer_surface);

    free(wayland_drawin);
    drawin->impl_data = NULL;
}

void wayland_drawin_moveresize(struct drawin_t *drawin)
{
    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;

    assert(wayland_drawin->layer_surface);
    assert(wayland_drawable->wl_surface);

    zwlr_layer_surface_v1_set_size(wayland_drawin->layer_surface,
            drawin->geometry.width, drawin->geometry.height);
    zwlr_layer_surface_v1_set_margin(wayland_drawin->layer_surface,
            drawin->geometry.y, 0, 0, drawin->geometry.x);

    if (wayland_drawin->mapped)
    {
        wl_surface_attach(wayland_drawable->wl_surface,
                wayland_drawable->buffer, 0, 0);
        wl_surface_damage_buffer(wayland_drawable->wl_surface,
                drawin->geometry.x, drawin->geometry.y,
                drawin->geometry.width, drawin->geometry.height);
        wl_surface_commit(wayland_drawable->wl_surface);
        wl_display_roundtrip(globalconf.wl_display);
    }
}

void wayland_drawin_refresh(struct drawin_t *drawin, int16_t x, int16_t y,
        uint16_t w, uint16_t h)
{
    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;

    assert(wayland_drawin->layer_surface);
    assert(drawin->drawable);

    if (!drawin->drawable->refreshed)
        return;

    /* Make sure it really has the size it should have */
    drawin_apply_moveresize(drawin);

    /* Make cairo do all pending drawing */
    cairo_surface_flush(drawin->drawable->surface);

    if (wayland_drawin->mapped)
    {
        wl_surface_attach(wayland_drawable->wl_surface,
                wayland_drawable->buffer, 0, 0);
        wl_surface_damage_buffer(wayland_drawable->wl_surface, x, y, w, h);
        wl_surface_commit(wayland_drawable->wl_surface);
        wl_display_roundtrip(globalconf.wl_display);
    }
}

void wayland_drawin_map(struct drawin_t *drawin)
{
    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;

    assert(wayland_drawin->layer_surface);

    wl_surface_attach(wayland_drawable->wl_surface, wayland_drawable->buffer, 0, 0);

    if (wayland_drawin->configured)
    {
        wayland_drawin->mapped = true;

        wl_surface_damage_buffer(wayland_drawable->wl_surface, 0, 0,
                drawin->geometry.width, drawin->geometry.height);
        wl_surface_commit(wayland_drawable->wl_surface);
        wl_display_roundtrip(globalconf.wl_display);
    }

}

void wayland_drawin_unmap(struct drawin_t *drawin)
{
    struct wayland_drawin *wayland_drawin = drawin->impl_data;
    struct wayland_drawable *wayland_drawable = drawin->drawable->impl_data;

    assert(wayland_drawin->layer_surface);

    if (wayland_drawin->configured)
    {
        wayland_drawin->mapped = false;

        wayland_drawable_unset_surface(drawin->drawable);
        wl_surface_damage_buffer(wayland_drawable->wl_surface, 0, 0,
                drawin->geometry.width, drawin->geometry.height);
        wl_surface_commit(wayland_drawable->wl_surface);
        wl_display_roundtrip(globalconf.wl_display);
    }

}

double wayland_drawin_get_opacity(struct drawin_t *drawin)
{
    // TODO
    return 1.0;
}

void wayland_drawin_set_cursor(struct drawin_t *drawin, const char *cursor)
{
    // TODO
}

void wayland_drawin_systray_kickout(struct drawin_t *drawin)
{
    // TODO
}

void wayland_drawin_set_shape_bounding(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    // TODO
}

void wayland_drawin_set_shape_clip(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    // TODO
}

void wayland_drawin_set_shape_input(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    // TODO
}

cairo_surface_t *wayland_drawin_get_shape_bounding(struct drawin_t *drawin)
{
    abort();
}

cairo_surface_t *wayland_drawin_get_shape_clip(struct drawin_t *drawin)
{
    abort();
}

cairo_surface_t *wayland_drawin_get_shape_input(struct drawin_t *drawin)
{
    abort();
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
