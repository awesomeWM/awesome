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

#include "draw.h"
#include "common/refcount.h"

typedef struct
{
    /** Reference counter */
    int refcount;
    draw_image_t *image;
} image_t;

static inline void
image_delete(image_t **i)
{
    if(*i)
    {
        draw_image_delete(&(*i)->image);
        p_delete(i);
    }
}

DO_RCNT(image_t, image, image_delete)

int luaA_image_userdata_new(lua_State *, image_t *);

/** Create a new image from a draw_image_t object.
 * \param i The image data.
 * \return A image_t object.
 */
static inline image_t *
image_new(draw_image_t *i)
{
    image_t *image = p_new(image_t, 1);
    image->image = i;
    return image;
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
