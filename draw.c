/*
 * draw.c - draw functions
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include <cairo-xcb.h>

#include "config.h"

#include <langinfo.h>
#include <iconv.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include <Imlib2.h>

#include "globalconf.h"
#include "screen.h"

#include "common/xutil.h"

/** Convert text from any charset to UTF-8 using iconv.
 * \param iso The ISO string to convert.
 * \param len The string size.
 * \param dest The destination pointer. Memory will be allocated, up to you to
 * free, like any char *.
 * \param dlen The destination length, can be NULL.
 * \return True if conversion was done.
 */
bool
draw_iso2utf8(const char *iso, size_t len, char **dest, ssize_t *dlen)
{
    static iconv_t iso2utf8 = (iconv_t) -1;
    static int8_t dont_need_convert = -1;

    if(dont_need_convert == -1)
        dont_need_convert = A_STREQ(nl_langinfo(CODESET), "UTF-8");

    if(!len || dont_need_convert)
        return false;

    if(iso2utf8 == (iconv_t) -1)
    {
        iso2utf8 = iconv_open("UTF-8", nl_langinfo(CODESET));
        if(iso2utf8 == (iconv_t) -1)
        {
            if(errno == EINVAL)
                warn("unable to convert text from %s to UTF-8, not available",
                     nl_langinfo(CODESET));
            else
                warn("unable to convert text: %s", strerror(errno));

            return false;
        }
    }

    size_t orig_utf8len, utf8len;
    char *utf8;

    orig_utf8len = utf8len = 2 * len + 1;
    utf8 = *dest = p_new(char, utf8len);

    if(iconv(iso2utf8, (char **) &iso, &len, &utf8, &utf8len) == (size_t) -1)
    {
        warn("text conversion failed: %s", strerror(errno));
        p_delete(dest);
        return false;
    }

    if(dlen)
        *dlen = orig_utf8len - utf8len;

    return true;
}

static cairo_user_data_key_t data_key;

static inline void
free_data(void *data)
{
    p_delete(&data);
}

/** Create a surface object on the lua stack from this image data.
 * \param L The lua stack.
 * \param width The width of the image.
 * \param height The height of the image
 * \param data The image's data in ARGB format, will be copied by this function.
 * \return Number of items pushed on the lua stack.
 */
int
luaA_surface_from_data(lua_State *L, int width, int height, uint32_t *data)
{
    unsigned long int len = width * height;
    unsigned long int i;
    uint32_t *buffer = p_new(uint32_t, len);
    cairo_surface_t *surface;

    /* Cairo wants premultiplied alpha, meh :( */
    for(i = 0; i < len; i++)
    {
        uint8_t a = (data[i] >> 24) & 0xff;
        double alpha = a / 255.0;
        uint8_t r = ((data[i] >> 16) & 0xff) * alpha;
        uint8_t g = ((data[i] >>  8) & 0xff) * alpha;
        uint8_t b = ((data[i] >>  0) & 0xff) * alpha;
        buffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    surface =
        cairo_image_surface_create_for_data((unsigned char *) buffer,
                                            CAIRO_FORMAT_ARGB32,
                                            width,
                                            height,
                                            width*4);
    /* This makes sure that buffer will be freed */
    cairo_surface_set_user_data(surface, &data_key, buffer, &free_data);

    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surface);

    return 1;
}

/** Duplicate the specified image surface.
 * \param surface The surface to copy
 * \return A pointer to a new cairo image surface.
 */
cairo_surface_t *
draw_dup_image_surface(cairo_surface_t *surface)
{
    cairo_surface_t *res = cairo_image_surface_create(
            cairo_image_surface_get_format(surface),
            cairo_image_surface_get_width(surface),
            cairo_image_surface_get_height(surface));

    cairo_t *cr = cairo_create(res);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);

    return res;
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

/** Load the specified path into a cairo surface
 * \param L Lua state
 * \param path file to load
 * \return A cairo image surface or NULL on error.
 */
int
draw_load_image(lua_State *L, const char *path)
{
    Imlib_Image imimage;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;
    int ret;

    imimage = imlib_load_image_with_error_return(path, &e);
    if (!imimage) {
        luaL_error(L, "Cannot load image '%s': %s", path, image_imlib_load_strerror(e));
        return 0;
    }

    imlib_context_set_image(imimage);
    ret = luaA_surface_from_data(L, imlib_image_get_width(),
            imlib_image_get_height(), imlib_image_get_data_for_reading_only());
    imlib_free_image_and_decache();
    return ret;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
