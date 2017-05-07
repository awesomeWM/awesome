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

#include "config.h"
#include "draw.h"
#include "globalconf.h"

#include <langinfo.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo-xcb.h>
#include <lauxlib.h>

static cairo_user_data_key_t data_key;

static inline void
free_data(void *data)
{
    p_delete(&data);
}

/** Create a surface object from this image data.
 * \param width The width of the image.
 * \param height The height of the image
 * \param data The image's data in ARGB format, will be copied by this function.
 * \return Number of items pushed on the lua stack.
 */
cairo_surface_t *
draw_surface_from_data(int width, int height, uint32_t *data)
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

    return surface;
}

/** Create a surface object from this pixbuf
 * \param buf The pixbuf
 * \return Number of items pushed on the lua stack.
 */
static cairo_surface_t *
draw_surface_from_pixbuf(GdkPixbuf *buf)
{
    int width = gdk_pixbuf_get_width(buf);
    int height = gdk_pixbuf_get_height(buf);
    int pix_stride = gdk_pixbuf_get_rowstride(buf);
    guchar *pixels = gdk_pixbuf_get_pixels(buf);
    int channels = gdk_pixbuf_get_n_channels(buf);
    cairo_surface_t *surface;
    int cairo_stride;
    unsigned char *cairo_pixels;

    cairo_format_t format = CAIRO_FORMAT_ARGB32;
    if (channels == 3)
        format = CAIRO_FORMAT_RGB24;

    surface = cairo_image_surface_create(format, width, height);
    cairo_surface_flush(surface);
    cairo_stride = cairo_image_surface_get_stride(surface);
    cairo_pixels = cairo_image_surface_get_data(surface);

    for (int y = 0; y < height; y++)
    {
        guchar *row = pixels;
        uint32_t *cairo = (uint32_t *) cairo_pixels;
        for (int x = 0; x < width; x++) {
            if (channels == 3)
            {
                uint8_t r = *row++;
                uint8_t g = *row++;
                uint8_t b = *row++;
                *cairo++ = (r << 16) | (g << 8) | b;
            } else {
                uint8_t r = *row++;
                uint8_t g = *row++;
                uint8_t b = *row++;
                uint8_t a = *row++;
                double alpha = a / 255.0;
                r = r * alpha;
                g = g * alpha;
                b = b * alpha;
                *cairo++ = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
        pixels += pix_stride;
        cairo_pixels += cairo_stride;
    }

    cairo_surface_mark_dirty(surface);
    return surface;
}

static void
get_surface_size(cairo_surface_t *surface, int *width, int *height)
{
    double x1, y1, x2, y2;
    cairo_t *cr = cairo_create(surface);

    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    cairo_destroy(cr);
    *width = x2 - x1;
    *height = y2 - y1;
}

/** Duplicate the specified image surface.
 * \param surface The surface to copy
 * \return A pointer to a new cairo image surface.
 */
cairo_surface_t *
draw_dup_image_surface(cairo_surface_t *surface)
{
    cairo_surface_t *res;
    int width, height;

    get_surface_size(surface, &width, &height);
#if CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR > 12
    res = cairo_surface_create_similar_image(surface, CAIRO_FORMAT_ARGB32, width, height);
#else
    res = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
#endif

    cairo_t *cr = cairo_create(res);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);

    return res;
}

/** Load the specified path into a cairo surface
 * \param L Lua state
 * \param path file to load
 * \param error A place to store an error message, if needed
 * \return A cairo image surface or NULL on error.
 */
cairo_surface_t *
draw_load_image(lua_State *L, const char *path, GError **error)
{
    cairo_surface_t *ret;
    GdkPixbuf *buf = gdk_pixbuf_new_from_file(path, error);

    if (!buf)
        /* error was set above */
        return NULL;

    ret = draw_surface_from_pixbuf(buf);
    g_object_unref(buf);
    return ret;
}

xcb_visualtype_t *draw_find_visual(const xcb_screen_t *s, xcb_visualid_t visual)
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(s);

    if(depth_iter.data)
        for(; depth_iter.rem; xcb_depth_next (&depth_iter))
            for(xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
                visual_iter.rem; xcb_visualtype_next (&visual_iter))
                if(visual == visual_iter.data->visual_id)
                    return visual_iter.data;

    return NULL;
}

xcb_visualtype_t *draw_default_visual(const xcb_screen_t *s)
{
    return draw_find_visual(s, s->root_visual);
}

xcb_visualtype_t *draw_argb_visual(const xcb_screen_t *s)
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(s);

    if(depth_iter.data)
        for(; depth_iter.rem; xcb_depth_next (&depth_iter))
            if(depth_iter.data->depth == 32)
                for(xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
                    visual_iter.rem; xcb_visualtype_next (&visual_iter))
                    return visual_iter.data;

    return NULL;
}

uint8_t draw_visual_depth(const xcb_screen_t *s, xcb_visualid_t vis)
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(s);

    if(depth_iter.data)
        for(; depth_iter.rem; xcb_depth_next (&depth_iter))
            for(xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
                visual_iter.rem; xcb_visualtype_next (&visual_iter))
                if(vis == visual_iter.data->visual_id)
                    return depth_iter.data->depth;

    fatal("Could not find a visual's depth");
}

void draw_test_cairo_xcb(void)
{
    xcb_pixmap_t pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, globalconf.default_depth, pixmap,
                      globalconf.screen->root, 1, 1);
    cairo_surface_t *surface = cairo_xcb_surface_create(globalconf.connection,
                                          pixmap, globalconf.visual, 1, 1);
    if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        fatal("Could not set up display: got cairo surface with status %s",
                cairo_status_to_string(cairo_surface_status(surface)));
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);
    xcb_free_pixmap(globalconf.connection, pixmap);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
