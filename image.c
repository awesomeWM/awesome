/*
 * image.c - image object
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

#include "structs.h"
#include "common/tokenize.h"

DO_LUA_TOSTRING(image_t, image, "image");

static int
luaA_image_gc(lua_State *L)
{
    image_t *p = luaL_checkudata(L, 1, "image");
    luaA_ref_array_wipe(&p->refs);
    imlib_context_set_image(p->image);
    imlib_free_image();
    p_delete(&p->data);
    return 0;
}

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

    p_realloc(&image->data, size * 4);
    dataimg = image->data;

    for(i = 0; i < size; i++, dataimg += 4)
    {
#if AWESOME_IS_BIG_ENDIAN
        dataimg[0] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[0] / 255.0;
        dataimg[1] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[2] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[3] = (data[i]         & 0xff) * alpha; /* B */
#else
        dataimg[3] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[3] / 255.0;
        dataimg[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[0] = (data[i]         & 0xff) * alpha; /* B */
#endif
    }
}

/** Create a new image from ARGB32 data.
 * \param width The image width.
 * \param height The image height.
 * \param data The image data.
 * \return 1 if an image has been pushed on stack, 0 otherwise.
 */
int
image_new_from_argb32(int width, int height, uint32_t *data)
{
    Imlib_Image imimage;

    if((imimage = imlib_create_image_using_copied_data(width, height, data)))
    {
        imlib_context_set_image(imimage);
        imlib_image_set_has_alpha(true);
        image_t *image = image_new(globalconf.L);
        image->image = imimage;
        image_compute(image);
        return 1;
    }

    return 0;
}

/** Load an image from filename.
 * \param filename The image file to load.
 * \return 1 if image is loaded and on stack, 0 otherwise.
 */
static int
image_new_from_file(const char *filename)
{
    Imlib_Image imimage;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;
    image_t *image;

    if(!filename)
        return 0;

    if(!(imimage = imlib_load_image_with_error_return(filename, &e)))
    {
        warn("cannot load image %s: %s", filename, image_imlib_load_strerror(e));
        return 0;
    }

    image = image_new(globalconf.L);
    image->image = imimage;

    image_compute(image);

    return 1;
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
    const char *filename;

    if((filename = lua_tostring(L, 2)))
        return image_new_from_file(filename);
    return 0;
}

/** Create a new image object from ARGB32 data.
 * \param L The Lua stack.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The image width.
 * \lparam The image height.
 * \lparam The image data as a string in ARGB32 format, or nil to create an
 * empty image.
 * \lreturn An image object.
 */
static int
luaA_image_argb32_new(lua_State *L)
{
    size_t len;
    unsigned int width = luaL_checknumber(L, 1);
    unsigned int height = luaL_checknumber(L, 2);

    if(lua_isnil(L, 3))
    {
        uint32_t *data = p_new(uint32_t, width * height);
        return image_new_from_argb32(width, height, data);
    }

    const char *data = luaL_checklstring(L, 3, &len);

    if(width * height * 4 != len)
        luaL_error(L, "string size does not match image size");

    return image_new_from_argb32(width, height, (uint32_t *) data);
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
    image_t *image = luaL_checkudata(L, 1, "image");
    int orientation = luaL_checknumber(L, 2);

    imlib_context_set_image(image->image);
    imlib_image_orientate(orientation);

    image_compute(image);

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
    image_t *image = luaL_checkudata(L, 1, "image"), *new;
    double angle = luaL_checknumber(L, 2);

    new = image_new(L);

    imlib_context_set_image(image->image);
    new->image = imlib_create_rotated_image(angle);

    image_compute(new);

    return 1;
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
    image_t *image = luaL_checkudata(L, 1, "image"), *new;
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);

    new = image_new(L);

    imlib_context_set_image(image->image);
    new->image = imlib_create_cropped_image(x, y, w, h);

    image_compute(new);

    return 1;
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
    image_t *image = luaL_checkudata(L, 1, "image"), *new;
    int source_x = luaL_checkint(L, 2);
    int source_y = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);
    int dest_w = luaL_checkint(L, 6);
    int dest_h = luaL_checkint(L, 7);

    new = image_new(L);

    imlib_context_set_image(image->image);
    new->image = imlib_create_cropped_scaled_image(source_x,
                                                   source_y,
                                                   w, h,
                                                   dest_w, dest_h);

    image_compute(new);

    return 1;
}

/** Draw a pixel in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalua An image.
 * \lparam The x coordinate of the pixel to draw
 * \lparam The y coordinate of the pixel to draw
 * \lparam The color to draw the pixel in
 */
static int
luaA_image_draw_pixel(lua_State *L)
{
    size_t len;
    color_t color;
    color_init_cookie_t cookie;
    image_t *image = luaL_checkudata(L, 1, "image");
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    const char *buf = luaL_checklstring(L, 4, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    if((x > imlib_image_get_width()) || (y > imlib_image_get_height()))
        return 0;

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    imlib_image_draw_pixel(x, y, 1);
    image_compute(image);
    return 0;
}

/** Draw a line in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalua An image.
 * \lparam The x1 coordinate of the line to draw
 * \lparam The y1 coordinate of the line to draw
 * \lparam The x2 coordinate of the line to draw
 * \lparam The y2 coordinate of the line to draw
 * \lparam The color to draw the line in
 */
static int
luaA_image_draw_line(lua_State *L)
{
    size_t len;
    color_t color;
    color_init_cookie_t cookie;
    image_t *image = luaL_checkudata(L, 1, "image");
    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_checkint(L, 4);
    int y2 = luaL_checkint(L, 5);
    const char *buf = luaL_checklstring(L, 6, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    if((MAX(x1,x2) > imlib_image_get_width()) || (MAX(y1,y2) > imlib_image_get_height()))
        return 0;

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    imlib_image_draw_line(x1, y1, x2, y2, 0);
    image_compute(image);
    return 0;
}

/** Draw a rectangle in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalua An image.
 * \lparam The x coordinate of the rectangles top left corner
 * \lparam The y coordinate of the rectangles top left corner
 * \lparam The width of the rectangle
 * \lparam The height of the rectangle
 * \lparam True if the rectangle should be filled, False otherwise
 * \lparam The color to draw the rectangle in
 */
static int
luaA_image_draw_rectangle(lua_State *L)
{
    size_t len;
    color_t color;
    color_init_cookie_t cookie;
    image_t *image = luaL_checkudata(L, 1, "image");
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width  = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    int fill = luaA_checkboolean(L, 6);
    const char *buf = luaL_checklstring(L, 7, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    if((x > imlib_image_get_width()) || (x + width > imlib_image_get_width()))
        return 0;
    if((y > imlib_image_get_height()) || (y + height > imlib_image_get_height()))
        return 0;

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    if(!fill)
        imlib_image_draw_rectangle(x, y, width, height);
    else
        imlib_image_fill_rectangle(x, y, width, height);
    image_compute(image);
    return 0;
}

/** Draw a circle in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalua An image.
 * \lparam The x coordinate of the center of the circle
 * \lparam The y coordinate of the center of the circle
 * \lparam The horizontal amplitude (width)
 * \lparam The vertical amplitude (height)
 * \lparam True if the circle should be filled, False otherwise
 * \lparam The color to draw the circle in
 */
static int
luaA_image_draw_circle(lua_State *L)
{
    size_t len;
    color_t color;
    color_init_cookie_t cookie;
    image_t *image = luaL_checkudata(L, 1, "image");
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int ah = luaL_checkint(L, 4);
    int av = luaL_checkint(L, 5);
    int fill = luaA_checkboolean(L, 6);
    const char *buf = luaL_checklstring(L, 7, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    if((x > imlib_image_get_width()) || (x + ah > imlib_image_get_width()) || (x - ah < 0))
        return 0;
    if((y > imlib_image_get_height()) || (y + av > imlib_image_get_height()) || (y - av < 0))
        return 0;

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    if(!fill)
        imlib_image_draw_ellipse(x, y, ah, av);
    else
        imlib_image_fill_ellipse(x, y, ah, av);
    image_compute(image);
    return 0;
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
    image_t *image = luaL_checkudata(L, 1, "image");
    const char *path = luaL_checkstring(L, 2);
    Imlib_Load_Error err;

    imlib_context_set_image(image->image);
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

    image_t *image = luaL_checkudata(L, 1, "image");
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_WIDTH:
        imlib_context_set_image(image->image);
        lua_pushnumber(L, imlib_image_get_width());
        break;
      case A_TK_HEIGHT:
        imlib_context_set_image(image->image);
        lua_pushnumber(L, imlib_image_get_height());
        break;
      case A_TK_ALPHA:
        imlib_context_set_image(image->image);
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
    /* draw on images, whee! */
    { "draw_pixel", luaA_image_draw_pixel },
    { "draw_line",  luaA_image_draw_line  },
    { "draw_rectangle", luaA_image_draw_rectangle },
    { "draw_circle", luaA_image_draw_circle },
    /* */
    { "__gc", luaA_image_gc },
    { "__tostring", luaA_image_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
