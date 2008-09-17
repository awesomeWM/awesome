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

#include <Imlib2.h>

#include "structs.h"
#include "image.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, image_t, image, "image", image_ref)
DO_LUA_GC(image_t, image, "image", image_unref)
DO_LUA_EQ(image_t, image, "image")

static const char *
image_imlib_load_strerror(Imlib_Load_Error e)
{
    switch(e)
    {
      case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
        return "no such file or directory";
      case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
        return "file is a directory";
      case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
        return "read permission denied";
      case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
        return "no loader for file format";
      case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
        return "path too long";
      case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
        return "path component non existant";
      case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
        return "path compoment not a directory";
      case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
        return "path points oustide address space";
      case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
        return "too many symbolic links";
      case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
        return "out of memory";
      case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
        return "out of file descriptors";
      case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
        return "write permission denied";
      case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
        return "out of disk space";
      case IMLIB_LOAD_ERROR_UNKNOWN:
        return "unknown error, that's really bad";
      case IMLIB_LOAD_ERROR_NONE:
        return "no error, oops";
    }

    return "unknown error";
}

/** Load an image from filename.
 * \param filename The image file to load.
 * \return A new image.
 */
image_t *
image_new_from_file(const char *filename)
{
    int w, h, size, i;
    uint32_t *data;
    double alpha;
    unsigned char *dataimg, *rdataimg;
    Imlib_Image imimage;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;
    image_t *image;

    if(!filename)
        return NULL;

    if(!(imimage = imlib_load_image_with_error_return(filename, &e)))
    {
        warn("cannot load image %s: %s", filename, image_imlib_load_strerror(e));
        return NULL;
    }

    imlib_context_set_image(imimage);

    w = imlib_image_get_width();
    h = imlib_image_get_height();

    size = w * h;

    data = imlib_image_get_data_for_reading_only();

    rdataimg = dataimg = p_new(unsigned char, size * 4);

    for(i = 0; i < size; i++, dataimg += 4)
    {
        dataimg[3] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[3] / 255.0;
        dataimg[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[0] = (data[i]         & 0xff) * alpha; /* B */
    }

    image = p_new(image_t, 1);

    image->data = rdataimg;
    image->width = w;
    image->height = h;

    imlib_free_image();

    return image;
}

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
    image_t *image;

    if((image = image_new_from_file(filename)))
        return luaA_image_userdata_new(L, image);

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
                    (*p)->width, (*p)->height);
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
