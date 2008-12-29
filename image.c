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

    if((imimage = imlib_create_image_using_copied_data(width, height, data)))
    {
        imlib_context_set_image(imimage);
        imlib_image_set_has_alpha(true);
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
 * \lparam The image path, or nil to create an empty image.
 * \lparam The image width if nil was set as first arg.
 * \lparam The image height if nil was set as first arg.
 * \lreturn An image object.
 */
static int
luaA_image_new(lua_State *L)
{
    const char *filename;

    if((filename = lua_tostring(L, 2)))
    {
        image_t *image;
        if((image = image_new_from_file(filename)))
            return luaA_image_userdata_new(L, image);
    }
    else if(lua_isnil(L, 2))
    {
        int width = luaL_checknumber(L, 3);
        int height = luaL_checknumber(L, 4);

        if(width <= 0 || height <= 0)
            luaL_error(L, "request image has invalid size");

        Imlib_Image imimage = imlib_create_image(width, height);
        image_t *image = p_new(image_t, 1);
        image->image = imimage;
        image_compute(image);
        return luaA_image_userdata_new(L, image);
    }

    return 0;
}

/** Create a new image object from ARGB32 data.
 * \param L The Lua stack.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The image width.
 * \lparam The image height.
 * \lparam The image data as a string in ARGB32 format.
 * \lreturn An image object.
 */
static int
luaA_image_argb32_new(lua_State *L)
{
    size_t len;
    image_t *image;
    unsigned int width = luaL_checknumber(L, 1);
    unsigned int height = luaL_checknumber(L, 2);
    const char *data = luaL_checklstring(L, 3, &len);

    if(width * height * 4 != len)
        luaL_error(L, "string size does not match image size");

    if((image = image_new_from_argb32(width, height, (uint32_t *) data)))
        return luaA_image_userdata_new(L, image);

    return 0;
}

/** Performs 90 degree rotations on the current image. Passing 0 orientation
 * does not rotate, 1 rotates clockwise by 90 degree, 2, rotates clockwise by
 * 180 degrees, 3 rotates clockwise by 270 degrees.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The rotation to perform.
 */
static int
luaA_image_orientate(lua_State *L)
{
    image_t **image = luaA_checkudata(L, 1, "image");
    int orientation = luaL_checknumber(L, 2);

    imlib_context_set_image((*image)->image);
    imlib_image_orientate(orientation);

    image_compute(*image);

    return 0;
}

/** Rotate an image with specified angle radians and return a new image.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The angle in radians.
 * \lreturn A rotated image.
 */
static int
luaA_image_rotate(lua_State *L)
{
    image_t **image = luaA_checkudata(L, 1, "image"), *new;
    double angle = luaL_checknumber(L, 2);

    new = p_new(image_t, 1);

    imlib_context_set_image((*image)->image);
    new->image = imlib_create_rotated_image(angle);

    image_compute(new);

    return luaA_image_userdata_new(L, new);
}

/** Crop an image to the given rectangle.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The top left x coordinate of the rectangle.
 * \lparam The top left y coordinate of the rectangle.
 * \lparam The width of the rectangle.
 * \lparam The height of the rectangle.
 * \lreturn A cropped image.
 */
static int
luaA_image_crop(lua_State *L)
{
    image_t **image = luaA_checkudata(L, 1, "image"), *new;
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);

    new = p_new(image_t, 1);

    imlib_context_set_image((*image)->image);
    new->image = imlib_create_cropped_image(x, y, w, h);

    image_compute(new);

    return luaA_image_userdata_new(L, new);
}

/** Crop the image to the given rectangle and scales it.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The top left x coordinate of the source rectangle.
 * \lparam The top left y coordinate of the source rectangle.
 * \lparam The width of the source rectangle.
 * \lparam The height of the source rectangle.
 * \lparam The width of the destination rectangle.
 * \lparam The height of the destination rectangle.
 * \lreturn A cropped image.
 */
static int
luaA_image_crop_and_scale(lua_State *L)
{
    image_t **image = luaA_checkudata(L, 1, "image"), *new;
    int source_x = luaL_checkint(L, 2);
    int source_y = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);
    int dest_w = luaL_checkint(L, 6);
    int dest_h = luaL_checkint(L, 7);

    new = p_new(image_t, 1);

    imlib_context_set_image((*image)->image);
    new->image = imlib_create_cropped_scaled_image(source_x,
                                                   source_y,
                                                   w, h,
                                                   dest_w, dest_h);

    image_compute(new);

    return luaA_image_userdata_new(L, new);
}

/** Saves the image to the given path. The file extension (e.g. .png or .jpg)
 * will affect the output format.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The image path.
 */
static int
luaA_image_save(lua_State *L)
{
    image_t **image = luaA_checkudata(L, 1, "image");
    const char *path = luaL_checkstring(L, 2);
    Imlib_Load_Error err;

    imlib_context_set_image((*image)->image);
    imlib_save_image_with_error_return(path, &err);

    if(err != IMLIB_LOAD_ERROR_NONE)
        warn("cannot save image %s: %s", path, image_imlib_load_strerror(err));

    return 0;
}

/** Image object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image
 * \lfield width The image width.
 * \lfield height The image height.
 */
static int
luaA_image_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    image_t **image = luaA_checkudata(L, 1, "image");
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_WIDTH:
        imlib_context_set_image((*image)->image);
        lua_pushnumber(L, imlib_image_get_width());
        break;
      case A_TK_HEIGHT:
        imlib_context_set_image((*image)->image);
        lua_pushnumber(L, imlib_image_get_height());
        break;
      case A_TK_ALPHA:
        imlib_context_set_image((*image)->image);
        lua_pushboolean(L, imlib_image_has_alpha());
        break;
      default:
        return 0;
    }

    return 1;
}

const struct luaL_reg awesome_image_methods[] =
{
    { "__call", luaA_image_new },
    { "argb32", luaA_image_argb32_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_image_meta[] =
{
    { "__index", luaA_image_index },
    { "rotate", luaA_image_rotate },
    { "orientate", luaA_image_orientate },
    { "crop", luaA_image_crop },
    { "crop_and_scale", luaA_image_crop_and_scale },
    { "save", luaA_image_save },
    { "__gc", luaA_image_gc },
    { "__eq", luaA_image_eq },
    { "__tostring", luaA_image_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
