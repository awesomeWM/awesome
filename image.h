/*
 * image.h - image object header
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <Imlib2.h>

#include "common/util.h"
#include "common/luaobject.h"
#include "common/luaclass.h"

typedef struct
{
    LUA_OBJECT_HEADER
    /** Imlib2 image */
    Imlib_Image image;
    /** Image data */
    uint8_t *data;
    /** Flag telling if the image is up to date or needs computing before
     * drawing */
    bool isupdated;
} image_t;

lua_class_t image_class;
LUA_OBJECT_FUNCS(image_class, image_t, image)

int image_new_from_argb32(int, int, uint32_t *);
uint8_t * image_getdata(image_t *);
int image_getwidth(image_t *);
int image_getheight(image_t *);

xcb_pixmap_t image_to_1bit_pixmap(image_t *, xcb_drawable_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
