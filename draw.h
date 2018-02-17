/*
 * draw.h - draw functions header
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

#ifndef AWESOME_COMMON_DRAW_H
#define AWESOME_COMMON_DRAW_H

#include <xcb/xcb.h>
#include <cairo.h>
#include <lua.h>
#include <glib.h> /* for GError */

#include "common/array.h"
#include "common/util.h"

/* Forward definition */
typedef struct _GdkPixbuf GdkPixbuf;

typedef struct area_t area_t;
struct area_t
{
    /** Co-ords of upper left corner */
    int16_t  x;
    int16_t  y;
    uint16_t width;
    uint16_t height;
};

#define AREA_LEFT(a)    ((a).x)
#define AREA_TOP(a)     ((a).y)
#define AREA_RIGHT(a)   ((a).x + (a).width)
#define AREA_BOTTOM(a)    ((a).y + (a).height)
#define AREA_EQUAL(a, b) ((a).x == (b).x && (a).y == (b).y && \
        (a).width == (b).width && (a).height == (b).height)

static inline void
cairo_surface_array_destroy_surface(cairo_surface_t **s)
{
    cairo_surface_destroy(*s);
}
DO_ARRAY(cairo_surface_t *, cairo_surface, cairo_surface_array_destroy_surface)

cairo_surface_t *draw_surface_from_data(int width, int height, uint32_t *data);
cairo_surface_t *draw_dup_image_surface(cairo_surface_t *surface);
cairo_surface_t *draw_load_image(lua_State *L, const char *path, GError **error);
cairo_surface_t *draw_surface_from_pixbuf(GdkPixbuf *buf);

xcb_visualtype_t *draw_find_visual(const xcb_screen_t *s, xcb_visualid_t visual);
xcb_visualtype_t *draw_default_visual(const xcb_screen_t *s);
xcb_visualtype_t *draw_argb_visual(const xcb_screen_t *s);
uint8_t draw_visual_depth(const xcb_screen_t *s, xcb_visualid_t vis);

void draw_test_cairo_xcb(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
