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

#include "globalconf.h"
#include <root.h>
#include "objects/screen.h"
#include "common/array.h"
#include "wayland/root.h"
#include "way-cooler-keybindings-unstable-v1.h"
#include "wlr-layer-shell-unstable-v1.h"
#include "wayland/utils.h"

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1.h"
#include <xcb/xcb.h>

extern struct root_impl root_impl;

struct wayland_wallpaper
{
    cairo_surface_t *surface;
    struct wl_surface *wl_surface;
    struct wl_buffer *buffer;
    struct zwlr_layer_surface_v1 *layer_surface;
    void *shm_data;
    size_t shm_size;
};

// TODO We need a list of them, for multi-head.
static struct wayland_wallpaper wayland_wallpaper;

static void wallpaper_surface_configure(void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial, uint32_t w, uint32_t h)
{
    zwlr_layer_surface_v1_ack_configure(surface, serial);

    wl_surface_attach(wayland_wallpaper.wl_surface,
            wayland_wallpaper.buffer, 0, 0);
	wl_surface_commit(wayland_wallpaper.wl_surface);
	wl_display_roundtrip(globalconf.wl_display);
}

static void wallpaper_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *surface)
{
	zwlr_layer_surface_v1_destroy(wayland_wallpaper.layer_surface);
}

struct zwlr_layer_surface_v1_listener wallpaper_surface_listener =
{
	.configure = wallpaper_surface_configure,
	.closed = wallpaper_surface_closed,
};

int wayland_set_wallpaper(cairo_pattern_t *pattern)
{
    area_t area = {
        .width = globalconf.primary_screen->geometry.width,
        .height = globalconf.primary_screen->geometry.height,
    };
    int stride = 0;

    wayland_setup_buffer(area, &wayland_wallpaper.buffer, &stride,
            &wayland_wallpaper.shm_data, &wayland_wallpaper.shm_size);
    wayland_wallpaper.surface =
        cairo_image_surface_create_for_data(wayland_wallpaper.shm_data,
                CAIRO_FORMAT_ARGB32, area.width, area.height, stride);
    wayland_wallpaper.wl_surface =
        wl_compositor_create_surface(globalconf.wl_compositor);

    cairo_t *cr = cairo_create(wayland_wallpaper.surface);
    /* Paint the pattern to the surface */
    cairo_set_source(cr, pattern);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_flush(wayland_wallpaper.surface);

    struct zwlr_layer_shell_v1 *layer_shell = globalconf.layer_shell;
    wayland_wallpaper.layer_surface =
        zwlr_layer_shell_v1_get_layer_surface(layer_shell,
                wayland_wallpaper.wl_surface, NULL,
                ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "awesome");
    zwlr_layer_surface_v1_set_size(wayland_wallpaper.layer_surface,
            area.width, area.height);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
            wayland_wallpaper.layer_surface, false);
    zwlr_layer_surface_v1_add_listener(wayland_wallpaper.layer_surface,
            &wallpaper_surface_listener, NULL);

    wl_surface_commit(wayland_wallpaper.wl_surface);
    wl_display_roundtrip(globalconf.wl_display);

    return true;
}

void wayland_update_wallpaper(void)
{
    fprintf(stderr, "TODO update wayland wallpaper\n");
}

static void on_key(void *data, struct zway_cooler_keybindings *keybindings,
        uint32_t time, uint32_t keycode, uint32_t state, uint32_t mods)
{
    /* get keysym ignoring all modifiers */
    xcb_keysym_t keysym =
        xcb_key_symbols_get_keysym(globalconf.keysyms, keycode, 0);
    bool pressed = state == ZWAY_COOLER_KEYBINDINGS_KEY_STATE_PRESSED;

    // TODO Check if intersects client, like in X11, and emit proper signals.

    root_handle_key(&globalconf.keys, false, time, keycode, mods,
            pressed, keysym, &root_impl);
}

struct zway_cooler_keybindings_listener keybindings_listener =
{
    .key = on_key,
};

void wayland_grab_keys(void) {
    zway_cooler_keybindings_clear_keys(globalconf.wl_keybindings);

    foreach(key_, globalconf.keys)
    {
        keyb_t *key = *key_;

        if (key->keycode)
        {
            zway_cooler_keybindings_register_key(globalconf.wl_keybindings,
                    key->keycode, key->modifiers);
        }
        else
        {
            xcb_keycode_t *keycodes =
                xcb_key_symbols_get_keycode(globalconf.keysyms, key->keysym);
            if(keycodes)
            {
                for(xcb_keycode_t *keycode = keycodes; *keycode; keycode++)
                {
                    zway_cooler_keybindings_register_key(globalconf.wl_keybindings,
                            *keycode, key->modifiers);
                }
            }
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
