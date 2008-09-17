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

/** Recompute the ARGB32 data from an image.
 * \param image The image.
 * \return Data.
 */
static void
image_compute(image_t *image)
{
    int size, i;
    uint32_t *data;
    double alpha;
    uint8_t *dataimg;

    imlib_context_set_image(image->image);

    data = imlib_image_get_data_for_reading_only();

    image->width = imlib_image_get_width();
    image->height = imlib_image_get_height();

    size = image->width * image->height;

    p_delete(&image->data);
    image->data = dataimg = p_new(uint8_t, size * 4);

    for(i = 0; i < size; i++, dataimg += 4)
    {
        dataimg[3] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[3] / 255.0;
        dataimg[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[0] = (data[i]         & 0xff) * alpha; /* B */
    }
}

/** Create a new image from ARGB32 data.
 * \param width The image width.
 * \param height The image height.
 * \param data The image data.
 * \return A brand new image.
 */
image_t *
image_new_from_argb32(int width, int height, uint32_t *data)
{
    Imlib_Image imimage;
    image_t *image = NULL;

    if((imimage = imlib_create_image_using_data(width, height, data)))
    {
        image = p_new(image_t, 1);
        image->image = imimage;
        image_compute(image);
    }

    return image;
}

/** Load an image from filename.
 * \param filename The image file to load.
 * \return A new image.
 */
image_t *
image_new_from_file(const char *filename)
{
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

    image = p_new(image_t, 1);
    image->image = imimage;

    image_compute(image);

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
