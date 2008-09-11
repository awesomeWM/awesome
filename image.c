/*
 * image.c - image object
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

#include "structs.h"
#include "image.h"

extern awesome_t globalconf;

DO_LUA_NEW(static, image_t, image, "image", image_ref)
DO_LUA_GC(image_t, image, "image", image_unref)
DO_LUA_EQ(image_t, image, "image")

/** Create a new image object.
 * \param L The Lua stack.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The image path.
 * \lreturn An image object.
 */
static int
luaA_image_new(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 2);
    draw_image_t *dimg;

    if((dimg = draw_image_new(filename)))
    {
        image_t *image = p_new(image_t, 1);
        image->image = dimg;
        return luaA_image_userdata_new(L, image);
    }

    return 0;
}

/** Return a formated string for an image.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue  An image.
 * \lreturn A string.
 */
static int
luaA_image_tostring(lua_State *L)
{
    image_t **p = luaA_checkudata(L, 1, "image");
    lua_pushfstring(L, "[image udata(%p) width(%d) height(%d)]", *p,
                    (*p)->image->width, (*p)->image->height);
    return 1;
}

const struct luaL_reg awesome_image_methods[] =
{
    { "__call", luaA_image_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_image_meta[] =
{
    { "__gc", luaA_image_gc },
    { "__eq", luaA_image_eq },
    { "__tostring", luaA_image_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
