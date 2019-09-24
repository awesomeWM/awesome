/*
 * screen.c - screen management
 *
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
#include "wayland/screen.h"

#include <stdint.h>

#include "xdg-output-unstable-v1.h"

static struct wayland_viewport *first_screen_viewport = NULL;
static struct wayland_viewport *last_screen_viewport = NULL;

static struct wayland_viewport
*viewport_dedupe(struct wayland_viewport *new_viewport)
{
    for (struct wayland_viewport *viewport = first_screen_viewport;
         viewport != NULL;
         viewport = viewport->next)
    {
        if (viewport->x == new_viewport->x
                && viewport-> y == new_viewport->y
                && viewport->width == new_viewport->width
                && viewport->height == new_viewport->height)
        {
            foreach(output, new_viewport->outputs)
            {
                wayland_screen_output_array_append(&viewport->outputs, *output);
            }
            free(new_viewport);
            return viewport;
        }
    }
    if (first_screen_viewport == NULL)
    {
        first_screen_viewport = new_viewport;
        last_screen_viewport = new_viewport;
    }
    else
    {
        last_screen_viewport->next = new_viewport;
        last_screen_viewport = new_viewport;
    }
    assert(first_screen_viewport && last_screen_viewport);
    return new_viewport;
}

static void viewport_clean(void)
{
    foreach(screen, globalconf.screens)
    {
        (*screen)->viewport = NULL;
    }
    for (struct wayland_viewport *viewport = first_screen_viewport;
         viewport != NULL;
         viewport = viewport->next)
    {
        foreach(output, viewport->outputs)
        {
            free(output->name);
            output->name = NULL;
        }
        wayland_screen_output_array_wipe(&viewport->outputs);
        free(viewport);
    }
    first_screen_viewport = last_screen_viewport = NULL;
}

static void xdg_output_on_logical_position(void *data,
        struct zxdg_output_v1 *zxdg_output_v1, int32_t x, int32_t y)
{
    screen_t *screen = data;
    struct wayland_viewport *viewport = screen->viewport;
    viewport->x = x;
    viewport->y = y;
}

static void xdg_output_on_logical_size(void *data,
        struct zxdg_output_v1 *zxdg_output_v1, int32_t width, int32_t height)
{}

static void xdg_output_on_done(void *data,
        struct zxdg_output_v1 *zxdg_output_v1)
{
    screen_t *screen = data;
    lua_State *L = globalconf_get_lua_State();
    viewport_dedupe(screen->viewport);
    screen_added(L, screen);
}

static void xdg_output_on_name(void *data,
        struct zxdg_output_v1 *zxdg_output_v1, const char *name)
{
    screen_t *screen = data;
    struct wayland_viewport *viewport = screen->viewport;
    struct wayland_screen_output *output = &viewport->outputs.tab[0];
    output->name = strdup(name);
}

static void xdg_output_on_description(void *data,
        struct zxdg_output_v1 *zxdg_output_v1, const char *description)
{}

struct zxdg_output_v1_listener xdg_output_listener =
{
    .logical_position = xdg_output_on_logical_position,
    .logical_size = xdg_output_on_logical_size,
    .done = xdg_output_on_done,
    .name = xdg_output_on_name,
    .description = xdg_output_on_description,
};

static void wl_output_on_geometry(void *data, struct wl_output *wl_output,
        int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
        int32_t subpixel, const char *make, const char *model,
        int32_t transform)
{
    screen_t *screen = data;
    struct wayland_viewport *viewport = screen->viewport;
    struct wayland_screen_output *output = &viewport->outputs.tab[0];
    output->mm_width = physical_width;
    output->mm_height = physical_height;
}

static void wl_output_on_mode(void *data, struct wl_output *wl_output,
        uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    screen_t *screen = data;
    struct wayland_viewport *viewport = screen->viewport;
    viewport->width = screen->geometry.width = width;
    viewport->height = screen->geometry.height = height;
}

// This is for atomic updates, this is not the object being destroyed.
static void wl_output_on_done(void *data, struct wl_output *wl_output)
{
    screen_t *screen = data;
    struct wayland_screen *wayland_screen = screen->impl_data;

    if (wayland_screen->wl_output != NULL
            && globalconf.xdg_output_manager != NULL
            && wayland_screen->xdg_output == NULL)
    {
        /* Get the XDG information if the manager is available. Otherwise happen
           once the manager is added
         */
        wayland_screen->xdg_output =
            zxdg_output_manager_v1_get_xdg_output(globalconf.xdg_output_manager,
                    wayland_screen->wl_output);
        // TODO Clean up on destroy
        zxdg_output_v1_add_listener(wayland_screen->xdg_output,
                &xdg_output_listener, screen);
        wl_display_roundtrip(globalconf.wl_display);
    }
}

static void wl_output_on_scale(void *data, struct wl_output *wl_output, int32_t factor)
{}

static struct wl_output_listener wl_output_listener =
{
    .geometry = wl_output_on_geometry,
    .mode = wl_output_on_mode,
    .done = wl_output_on_done,
    .scale = wl_output_on_scale,
};

void wayland_new_screen(screen_t *screen, void *data)
{
    assert(data);
    struct wl_output *wl_output = data;

    screen->impl_data = calloc(1, sizeof(struct wayland_screen));
    struct wayland_screen *wayland_screen = screen->impl_data;
    wayland_screen->screen = screen;
    wayland_screen->wl_output = wl_output;

    // De-dup later, once we have all the positioning information from XDG.
    struct wayland_viewport *viewport = calloc(1, sizeof(struct wayland_viewport));
    screen->viewport = viewport;
    wayland_screen_output_array_init(&viewport->outputs);
    struct wayland_screen_output output = {0};
    wayland_screen_output_array_append(&viewport->outputs, output);

    wl_output_add_listener(wl_output, &wl_output_listener, screen);
    wl_display_roundtrip(globalconf.wl_display);
}

void wayland_wipe_screen(screen_t *screen)
{
    free(screen->impl_data);
    screen->impl_data = NULL;
}

void wayland_cleanup_screens(void)
{
    viewport_clean();
}

void wayland_mark_fake_screen(screen_t *screen)
{
    struct wayland_screen *wayland_screen = screen->impl_data;
    wayland_screen->wl_output = NULL;
}

void wayland_scan_screens(void)
{
    wl_display_roundtrip(globalconf.wl_display);
    /*
      NOTE This is the first time we are getting the Wayland globals,
      so we have to check this here.

      XXX We fetch the globals so late because when outputs are advertised they
      immediately try to make a screen but if we do it too early then Lua isn't
      initialized and we get segfaults.
    */
    assert(globalconf.wl_compositor && globalconf.wl_shm && globalconf.wl_seat);
    if (globalconf.wl_mousegrabber == NULL)
        fatal("Expected compositor to advertise Way Cooler mousegrabber protocol");
}

void wayland_get_screens(lua_State *L, screen_array_t *screens)
{}

int wayland_viewport_get_outputs(lua_State *L, void *viewport)
{
    return 0;
}

int wayland_get_viewports(lua_State *L)
{
    return 0;
}

int wayland_get_outputs(lua_State *L, screen_t *s)
{
    struct wayland_viewport *viewport = s->viewport;
    lua_createtable(L, 0, viewport->outputs.len);
    foreach(output, viewport->outputs)
    {
        lua_createtable(L, 2, 0);

        lua_pushinteger(L, output->mm_width);
        lua_setfield(L, -2, "mm_width");
        lua_pushinteger(L, output->mm_height);
        lua_setfield(L, -2, "mm_height");

        lua_setfield(L, -2, output->name);
    }
    /* The table of tables we created. */
    return 1;
}

screen_t *wayland_update_primary(void)
{
    // XXX This doesn't make sense under Wayland.
    return NULL;
}

screen_t *wayland_screen_by_name(char *name)
{
    fatal("TODO");
}

bool wayland_outputs_changed(screen_t *existing, screen_t *other)
{
    fatal("TODO");
}

bool wayland_does_screen_exist(screen_t *screen, screen_array_t screens)
{
    struct wayland_screen *wayland_screen = screen->impl_data;

    foreach(old_screen, screens)
    {
        struct wayland_screen *wayland_old_screen = (*old_screen)->impl_data;
        if (wayland_screen->wl_output == wayland_old_screen->wl_output)
            return true;
    }
    return false;
}

bool wayland_is_fake_screen(screen_t *screen)
{
    struct wayland_screen *wayland_screen = screen->impl_data;
    return wayland_screen->wl_output == NULL;
}

bool wayland_is_same_screen(screen_t *left, screen_t *right)
{
    struct wayland_screen *left_wayland_screen = left->impl_data;
    struct wayland_screen *right_wayland_screen = right->impl_data;
    return left_wayland_screen->wl_output == right_wayland_screen->wl_output;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
