/*
 * image.h - image object header
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_IMAGE_H
#define AWESOME_IMAGE_H

#include <xcb/xcb.h>
#include "common/luaclass.h"

typedef struct image image_t;

void image_class_setup(lua_State *);
int image_new_from_argb32(lua_State *L, int, int, uint32_t *);
uint8_t * image_getdata(image_t *);
int image_getwidth(image_t *);
int image_getheight(image_t *);

xcb_pixmap_t image_to_1bit_pixmap(image_t *, xcb_drawable_t);

lua_class_t image_class;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
