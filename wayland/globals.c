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

#include <mousegrabber.h>
#include <root.h>

#include "wayland/globals.h"
#include "globalconf.h"

#include "wayland/mousegrabber.h"
#include "wayland/root.h"

#include "way-cooler-mousegrabber-unstable-v1.h"
#include "way-cooler-keybindings-unstable-v1.h"

#include <glib-unix.h>
#include <wayland-client.h>

extern struct mousegrabber_impl mousegrabber_impl;
extern struct root_impl root_impl;

/* Instance of an event source that we use to integrate the wayland event queue
 * with GLib's MainLoop.
 */
struct InterfaceEventSource
{
	GSource source;
	struct wl_display *display;
	gpointer fd_tag;
};

static void awesome_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version);

static const struct wl_registry_listener wl_registry_listener =
{
    .global = awesome_handle_global
};

/* This function is called to prepare polling event source. We just flush
 * and indicate that we have no timeouts, nor are currently pending.
 */
static gboolean interface_prepare(GSource *base, gint *timeout)
{
	struct InterfaceEventSource *interface_source =
			(struct InterfaceEventSource *)base;

	wl_display_flush(interface_source->display);
	*timeout = -1;

	return FALSE;
}

/* This function is called after file descriptors were checked. We indicate that
 * we need to be dispatched if any events on the epoll fd we got from libwayland
 * are pending / need to be handled.
 */
static gboolean interface_check(GSource *base)
{
	struct InterfaceEventSource *interface_source =
			(struct InterfaceEventSource *)base;
	GIOCondition condition =
			g_source_query_unix_fd(base, interface_source->fd_tag);

	/* We need to dispatch if anything happened on the fd */
	return condition != 0;
}

/* This function is called to actually "do" some work. We just run the wayland
 * event queue with a timeout of 0.
 */
static gboolean interface_dispatch(
		GSource *base, GSourceFunc callback, gpointer data)
{
	struct InterfaceEventSource *interface_source =
			(struct InterfaceEventSource *)base;
	if (wl_display_roundtrip(interface_source->display) == -1) {
		exit(0);
	}

	(void)callback;
	(void)data;

	return G_SOURCE_CONTINUE;
}

static GSourceFuncs interface_funcs =
{
		.prepare = interface_prepare,
		.check = interface_check,
		.dispatch = interface_dispatch,
};

static void setup_glib_listeners(struct wl_display *display)
{
    struct InterfaceEventSource *interface_source;
    GSource *source = g_source_new(&interface_funcs, sizeof(*interface_source));

    interface_source = (struct InterfaceEventSource *)source;
    interface_source->display = display;
    wl_display_roundtrip(interface_source->display);

    interface_source->fd_tag = g_source_add_unix_fd(
            source, wl_display_get_fd(display), G_IO_IN | G_IO_ERR | G_IO_HUP);
    g_source_set_can_recurse(source, TRUE);

    g_source_attach(source, NULL);
}

static void awesome_handle_global(void *data, struct wl_registry *registry,
        uint32_t name, const char *interface, uint32_t version)
{
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        globalconf.wl_compositor = wl_registry_bind(registry, name,
                &wl_compositor_interface, version);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        globalconf.wl_seat = wl_registry_bind(registry, name,
                &wl_seat_interface, version);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        globalconf.wl_shm = wl_registry_bind(registry, name,
                &wl_shm_interface, version);
    }
    else if (strcmp(interface, zway_cooler_mousegrabber_interface.name) == 0)
    {
        globalconf.wl_mousegrabber = wl_registry_bind(registry, name,
                &zway_cooler_mousegrabber_interface, version);
        zway_cooler_mousegrabber_add_listener(globalconf.wl_mousegrabber,
                &mousegrabber_listener, NULL);
        wl_display_roundtrip(globalconf.wl_display);
    }
    else if (strcmp(interface, zway_cooler_keybindings_interface.name) == 0)
    {
        globalconf.wl_keybindings = wl_registry_bind(registry, name,
                &zway_cooler_keybindings_interface, version);
        zway_cooler_keybindings_add_listener(globalconf.wl_keybindings,
                &keybindings_listener, NULL);
    }
}

static void setup_wayland_globals(struct wl_display *display,
        struct wl_registry * registry)
{
    wl_registry_add_listener(globalconf.wl_registry, &wl_registry_listener, NULL);
    wl_display_roundtrip(display);

    assert(globalconf.wl_compositor && globalconf.wl_shm && globalconf.wl_seat);

    if (globalconf.wl_mousegrabber == NULL)
    {
        fatal("Expected compositor to advertise Way Cooler mousegrabber protocol");
    }
}

void init_wayland(void)
{
    globalconf.wl_display = wl_display_connect(NULL);
    if (globalconf.wl_display == NULL)
    {
        fatal("Unable to connect to Wayland compositor");
    }
    globalconf.wl_registry = wl_display_get_registry(globalconf.wl_display);

    setup_wayland_globals(globalconf.wl_display, globalconf.wl_registry);

    setup_glib_listeners(globalconf.wl_display);

    mousegrabber_impl = (struct mousegrabber_impl){
        .grab_mouse = wayland_grab_mouse,
        .release_mouse = wayland_release_mouse,
    };
    root_impl = (struct root_impl){
        .grab_keys = wayland_grab_keys,
    };
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
