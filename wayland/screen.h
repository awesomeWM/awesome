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
#ifndef AWESOME_WAYLAND_SCREEN_H
#define AWESOME_WAYLAND_SCREEN_H

#include "common/luaclass.h"
#include "objects/screen.h"

#include "xdg-output-unstable-v1.h"
#include <wayland-client.h>

ARRAY_TYPE(struct wl_output, wl_output);

struct wayland_wallpaper;

struct wayland_screen_output
{
    char *name;
    int32_t mm_width, mm_height;
    wl_output_array_t outputs;
};

ARRAY_TYPE(struct wayland_screen_output, wayland_screen_output);
ARRAY_FUNCS(struct wayland_screen_output, wayland_screen_output, DO_NOTHING);

struct wayland_viewport
{
    int x, y, width, height;
    wayland_screen_output_array_t outputs;

    struct wayland_viewport *next;
};

struct wayland_screen
{
    struct zxdg_output_v1 *xdg_output;
    struct wl_output *wl_output;
    struct wayland_wallpaper *wallpaper;
    screen_t *screen;
    bool configured;
};

extern struct zxdg_output_v1_listener xdg_output_listener;

void wayland_new_screen(screen_t *screen, void *data);
void wayland_wipe_screen(screen_t *screen);
void wayland_cleanup_screens(void);
void wayland_mark_fake_screen(screen_t *screen);
void wayland_scan_screens(void);
void wayland_get_screens(lua_State *L, struct screen_array_t *screens);

int wayland_viewport_get_outputs(lua_State *L, void *viewport);
int wayland_get_viewports(lua_State *L);
int wayland_get_outputs(lua_State *L, screen_t *s);

screen_t *wayland_update_primary(void);
screen_t *wayland_screen_by_name(char *name);

bool wayland_outputs_changed(screen_t *existing, screen_t *other);
bool wayland_does_screen_exist(screen_t *screen, screen_array_t screens);
bool wayland_is_fake_screen(screen_t *screen);
bool wayland_is_same_screen(screen_t *left, screen_t *right);

#endif  // AWESOME_WAYLAND_SCREEN_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
