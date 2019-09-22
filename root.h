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

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <cairo/cairo.h>

#include "objects/key.h"

#ifndef AWESOME_ROOT_H
#define AWESOME_ROOT_H

struct root_impl
{
	int (*set_wallpaper)(cairo_pattern_t *pattern);
	void (*update_wallpaper)(void);
    void (*grab_keys)(void);
};

void root_update_wallpaper(void);

void root_handle_key(key_array_t *arr, bool pushed_to_stack,
		uint32_t timestamp, uint32_t keycode, uint16_t state,
        bool pressed, xcb_keysym_t keysym, struct root_impl *root);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
