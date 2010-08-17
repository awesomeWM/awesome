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

#include <xcb/xcb_image.h>

#include <Imlib2.h>

#include "globalconf.h"
#include "config.h"
#include "luaa.h"
#include "common/luaobject.h"

struct image
{
    LUA_OBJECT_HEADER
    /** Imlib2 image */
    Imlib_Image image;
    /** Image data */
    uint8_t *data;
    /** Flag telling if the image is up to date or needs computing before
     * drawing */
    bool isupdated;
};

LUA_OBJECT_FUNCS(image_class, image_t, image)

static void
image_wipe(image_t *image)
{
    imlib_context_set_image(image->image);
    imlib_free_image();
    p_delete(&image->data);
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
        return "path component non existent";
      case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
        return "path component not a directory";
      case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
        return "path points outside address space";
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

/** Get image width.
 * \param image The image.
 * \return The image width in pixel.
 */
int
image_getwidth(image_t *image)
{
    imlib_context_set_image(image->image);
    return imlib_image_get_width();
}

/** Get image height.
 * \param image The image.
 * \return The image height in pixel.
 */
int
image_getheight(image_t *image)
{
    imlib_context_set_image(image->image);
    return imlib_image_get_height();
}

/** Get the ARGB32 data from an image.
 * \param image The image.
 * \return Data.
 */
uint8_t *
image_getdata(image_t *image)
{
    int size, i;
    uint32_t *data;
    double alpha;
    uint8_t *dataimg;
#if AWESOME_IS_BIG_ENDIAN
    const int index_a = 0, index_r = 1, index_g = 2, index_b = 3;
#else
    const int index_a = 3, index_r = 2, index_g = 1, index_b = 0;
#endif

    if(image->isupdated)
        return image->data;

    imlib_context_set_image(image->image);

    data = imlib_image_get_data_for_reading_only();

    size = imlib_image_get_width() * imlib_image_get_height();

    p_realloc(&image->data, size * 4);
    dataimg = image->data;

    for(i = 0; i < size; i++, dataimg += 4)
    {
        dataimg[index_a] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[index_a] / 255.0;
        dataimg[index_r] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[index_g] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[index_b] = (data[i]         & 0xff) * alpha; /* B */
    }

    image->isupdated = true;

    return image->data;

}

static void
image_draw_to_1bit_ximage(image_t *image, xcb_image_t *img)
{
    imlib_context_set_image(image->image);

    uint32_t *data = imlib_image_get_data_for_reading_only();

    int width = imlib_image_get_width();
    int height = imlib_image_get_height();

    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++)
        {
            int i, pixel, tmp;

            i = y * width + x;

            // Sum up all color components ignoring alpha
            tmp  = (data[i] >> 16) & 0xff;
            tmp += (data[i] >>  8) & 0xff;
            tmp +=  data[i]        & 0xff;

            pixel = (tmp / 3 < 127) ? 0 : 1;

            xcb_image_put_pixel(img, x, y, pixel);
        }
}

// Convert an image to a 1bit pixmap
xcb_pixmap_t
image_to_1bit_pixmap(image_t *image, xcb_drawable_t d)
{
    xcb_pixmap_t pixmap;
    xcb_image_t *img;
    uint16_t width, height;

    width = image_getwidth(image);
    height = image_getheight(image);

    /* Prepare the pixmap and gc */
    pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, 1, pixmap, d, width, height);

    /* Prepare the image */
    img = xcb_image_create_native(globalconf.connection, width, height,
            XCB_IMAGE_FORMAT_XY_BITMAP, 1, NULL, 0, NULL);
    image_draw_to_1bit_ximage(image, img);

    /* Paint the image to the pixmap */
    xcb_image_put(globalconf.connection, pixmap, globalconf.gc, img, 0, 0, 0);

    xcb_image_destroy(img);

    return pixmap;
}

/** Create a new image from ARGB32 data.
 * \param L The Lua VM state.
 * \param width The image width.
 * \param height The image height.
 * \param data The image data.
 * \return 1 if an image has been pushed on stack, 0 otherwise.
 */
int
image_new_from_argb32(lua_State *L, int width, int height, uint32_t *data)
{
    Imlib_Image imimage;

    if((imimage = imlib_create_image_using_copied_data(width, height, data)))
    {
        imlib_context_set_image(imimage);
        imlib_image_set_has_alpha(true);
        image_t *image = image_new(L);
        image->image = imimage;
        return 1;
    }

    return 0;
}

/** Create a new, completely black image.
 * \param L The Lua VM state.
 * \param width The image width.
 * \param height The image height.
 * \return 1 if an image has been pushed on stack, 0 otherwise.
 */
static int
image_new_blank(lua_State *L, int width, int height)
{
    Imlib_Image imimage;

    if((imimage = imlib_create_image(width, height)))
    {
        imlib_context_set_image(imimage);
        imlib_image_set_has_alpha(true);
        /* After creation, an image has undefined content. Fix that up. */
        imlib_context_set_color(0, 0, 0, 0xff);
        imlib_image_fill_rectangle(0, 0, width, height);
        image_t *image = image_new(L);
        image->image = imimage;
        return 1;
    }

    return 0;
}

/** Load an image from filename.
 * \param L The Lua VM state.
 * \param filename The image file to load.
 * \return 1 if image is loaded and on stack, 0 otherwise.
 */
static int
image_new_from_file(lua_State *L, const char *filename)
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

    /* Make imlib check if the file changed on disk if it's later opened by the
     * same file name again before using its cache.
     */
    imlib_context_set_image(imimage);
    imlib_image_set_changes_on_disk();

    image = image_new(L);
    image->image = imimage;

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
        return image_new_from_file(L, filename);
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

    if (width == 0)
        luaL_error(L, "image.argb32() called with zero width");
    if (height == 0)
        luaL_error(L, "image.argb32() called with zero height");

    if(lua_isnil(L, 3))
    {
        return image_new_blank(L, width, height);
    }

    const char *data = luaL_checklstring(L, 3, &len);

    if(width * height * 4 != len)
        luaL_error(L, "string size does not match image size");

    return image_new_from_argb32(L, width, height, (uint32_t *) data);
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
    int orientation = luaL_checknumber(L, 2);

    imlib_context_set_image(image->image);
    imlib_image_orientate(orientation);

    image->isupdated = false;

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
    image_t *image = luaA_checkudata(L, 1, &image_class), *new;
    double angle = luaL_checknumber(L, 2);

    new = image_new(L);

    imlib_context_set_image(image->image);
    new->image = imlib_create_rotated_image(angle);

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
    image_t *image = luaA_checkudata(L, 1, &image_class), *new;
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int w = luaL_checkint(L, 4);
    int h = luaL_checkint(L, 5);

    new = image_new(L);

    imlib_context_set_image(image->image);
    new->image = imlib_create_cropped_image(x, y, w, h);

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
    image_t *image = luaA_checkudata(L, 1, &image_class), *new;
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

    return 1;
}

/** Draw a pixel in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalue An image.
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
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
    image->isupdated = false;
    return 0;
}

/** Draw a line in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalue An image.
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_checkint(L, 4);
    int y2 = luaL_checkint(L, 5);
    const char *buf = luaL_checklstring(L, 6, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    imlib_image_draw_line(x1, y1, x2, y2, 0);
    image->isupdated = false;
    return 0;
}

/** Draw a rectangle in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalue An image.
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width  = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    int fill = luaA_checkboolean(L, 6);
    const char *buf = luaL_checklstring(L, 7, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    if(!fill)
        imlib_image_draw_rectangle(x, y, width, height);
    else
        imlib_image_fill_rectangle(x, y, width, height);
    image->isupdated = false;
    return 0;
}

/** Convert a table to a color range and set it as current color range.
 * \param L The Lua VM state.
 * \param ud The index of the table.
 * \return A color range that you are responsible to free.
 */
static Imlib_Color_Range
luaA_table_to_color_range(lua_State *L, int ud)
{
    Imlib_Color_Range range = imlib_create_color_range();

    imlib_context_set_color_range(range);

    for(size_t i = 1; i <= lua_objlen(L, ud); i++)
    {
        /* get t[i] */
        lua_pushnumber(L, i);
        lua_gettable(L, ud);

        size_t len;
        const char *colstr = lua_tolstring(L, -1, &len);

        if(colstr)
        {
            color_t color;

            color_init_cookie_t cookie = color_init_unchecked(&color, colstr, len);

            /* get value with colstr as key in table */
            lua_pushvalue(L, -1);
            lua_gettable(L, ud);

            /* convert the distance, if any, to number, or set 1 */
            int distance = lua_tonumber(L, -1);
            /* remove distance */
            lua_pop(L, 1);

            color_init_reply(cookie);

            imlib_context_set_color(color.red, color.green, color.blue, color.alpha);

            imlib_add_color_to_color_range(distance);
        }

        /* remove color string (value) */
        lua_pop(L, 1);
    }

    return range;
}

/** Draw a rectangle in an image with gradient color.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The x coordinate of the rectangles top left corner.
 * \lparam The y coordinate of the rectangles top left corner.
 * \lparam The width of the rectangle.
 * \lparam The height of the rectangle.
 * \lparam A table with the color to draw the rectangle. You can specified the
 * color distance from the previous one by setting t[color] = distance.
 * \lparam The angle of gradient.
 */
static int
luaA_image_draw_rectangle_gradient(lua_State *L)
{
    image_t *image = luaA_checkudata(L, 1, &image_class);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width  = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    luaA_checktable(L, 6);
    double angle = luaL_checknumber(L, 7);

    imlib_context_set_image(image->image);

    luaA_table_to_color_range(L, 6);

    imlib_image_fill_color_range_rectangle(x, y, width, height, angle);

    imlib_free_color_range();

    image->isupdated = false;

    return 0;
}

/** Draw a circle in an image
 * \param L The Lua VM state
 * \luastack
 * \lvalue An image.
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int ah = luaL_checkint(L, 4);
    int av = luaL_checkint(L, 5);
    int fill = luaA_checkboolean(L, 6);
    const char *buf = luaL_checklstring(L, 7, &len);

    cookie = color_init_unchecked(&color, buf, len);
    imlib_context_set_image(image->image);
    color_init_reply(cookie);

    imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
    if(!fill)
        imlib_image_draw_ellipse(x, y, ah, av);
    else
        imlib_image_fill_ellipse(x, y, ah, av);
    image->isupdated = false;
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
    image_t *image = luaA_checkudata(L, 1, &image_class);
    const char *path = luaL_checkstring(L, 2);
    Imlib_Load_Error err;

    imlib_context_set_image(image->image);
    imlib_save_image_with_error_return(path, &err);

    if(err != IMLIB_LOAD_ERROR_NONE)
        warn("cannot save image %s: %s", path, image_imlib_load_strerror(err));

    return 0;
}

/** Insert one image into another.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue An image.
 * \lparam The image to insert.
 * \lparam The X offset of the image to insert (optional).
 * \lparam The Y offset of the image to insert (optional).
 * \lparam The horizontal offset of the upper right image corner (optional).
 * \lparam The vertical offset of the upper right image corner (optional).
 * \lparam The horizontal offset of the lower left image corner (optional).
 * \lparam The vertical offset of the lower left image corner (optional).
 * \lparam The X coordinate of the source rectangle (optional).
 * \lparam The Y coordinate of the source rectangle (optional).
 * \lparam The width of the source rectangle (optional).
 * \lparam The height of the source rectangle (optional).
 */
static int
luaA_image_insert(lua_State *L)
{
    image_t *image_target = luaA_checkudata(L, 1, &image_class);
    image_t *image_source = luaA_checkudata(L, 2, &image_class);
    int xoff = luaL_optnumber(L, 3, 0);
    int yoff = luaL_optnumber(L, 4, 0);

    int xsrc = luaL_optnumber(L, 5, 0);
    int ysrc = luaL_optnumber(L, 6, 0);
    int wsrc = luaL_optnumber(L, 7, image_getwidth(image_source));
    int hsrc = luaL_optnumber(L, 8, image_getheight(image_source));

    int hxoff = luaL_optnumber(L, 9, image_getwidth(image_source));
    int hyoff = luaL_optnumber(L, 10, 0);

    int vxoff = luaL_optnumber(L, 11, 0);
    int vyoff = luaL_optnumber(L, 12, image_getheight(image_source));

    imlib_context_set_image(image_target->image);

    imlib_blend_image_onto_image_skewed(image_source->image, 0,
                                        /* source rectangle */
                                        xsrc, ysrc, wsrc, hsrc,
                                        /* position of the source image in the target image */
                                        xoff, yoff,
                                        /* axis offsets for the source image, (w|0|0|h)
                                         * is the default */
                                        hxoff, hyoff, vxoff, vyoff);

    image_target->isupdated = false;

    return 0;
}

static int
luaA_image_get_width(lua_State *L, image_t *image)
{
    lua_pushnumber(L, image_getwidth(image));
    return 1;
}

static int
luaA_image_get_height(lua_State *L, image_t *image)
{
    lua_pushnumber(L, image_getheight(image));
    return 1;
}

static int
luaA_image_get_alpha(lua_State *L, image_t *image)
{
    imlib_context_set_image(image->image);
    lua_pushboolean(L, imlib_image_has_alpha());
    return 1;
}

void
image_class_setup(lua_State *L)
{
    static const struct luaL_reg image_methods[] =
    {
        LUA_CLASS_METHODS(image)
        { "__call", luaA_image_new },
        { "argb32", luaA_image_argb32_new },
        { NULL, NULL }
    };

    static const struct luaL_reg image_meta[] =
    {
        LUA_OBJECT_META(image)
        LUA_CLASS_META
        { "rotate", luaA_image_rotate },
        { "orientate", luaA_image_orientate },
        { "crop", luaA_image_crop },
        { "crop_and_scale", luaA_image_crop_and_scale },
        { "save", luaA_image_save },
        { "insert", luaA_image_insert },
        /* draw on images, whee! */
        { "draw_pixel", luaA_image_draw_pixel },
        { "draw_line",  luaA_image_draw_line  },
        { "draw_rectangle", luaA_image_draw_rectangle },
        { "draw_rectangle_gradient", luaA_image_draw_rectangle_gradient },
        { "draw_circle", luaA_image_draw_circle },
        { NULL, NULL }
    };

    luaA_class_setup(L, &image_class, "image", NULL,
                     (lua_class_allocator_t) image_new,
                     (lua_class_collector_t) image_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     image_methods, image_meta);
    luaA_class_add_property(&image_class, A_TK_WIDTH,
                            NULL,
                            (lua_class_propfunc_t) luaA_image_get_width,
                            NULL);
    luaA_class_add_property(&image_class, A_TK_HEIGHT,
                            NULL,
                            (lua_class_propfunc_t) luaA_image_get_height,
                            NULL);
    luaA_class_add_property(&image_class, A_TK_ALPHA,
                            NULL,
                            (lua_class_propfunc_t) luaA_image_get_alpha,
                            NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
