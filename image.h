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

#include <lua.h>
#include <Imlib2.h>

#include "common/util.h"
#include "common/refcount.h"

typedef struct
{
    /** Reference counter */
    int refcount;
    /** Imlib2 image */
    Imlib_Image image;
    /** Image width */
    int width;
    /** Image height */
    int height;
    /** Image data */
    uint8_t *data;
} image_t;

static inline void
image_delete(image_t **i)
{
    if(*i)
    {
        imlib_context_set_image((*i)->image);
        imlib_free_image();
        p_delete(&(*i)->data);
        p_delete(i);
    }
}

DO_RCNT(image_t, image, image_delete)

image_t * image_new_from_file(const char *);
image_t * image_new_from_argb32(int, int, uint32_t *);
uint8_t * image_data_argb32_get(image_t *);

int luaA_image_userdata_new(lua_State *, image_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
