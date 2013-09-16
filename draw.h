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

#include "common/util.h"

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

bool draw_iso2utf8(const char *, size_t, char **, ssize_t *);

/** Convert a string to UTF-8.
 * \param str The string to convert.
 * \param len The string length.
 * \param dest The destination string that will be allocated.
 * \param dlen The destination string length allocated, can be NULL.
 * \return True if the conversion happened, false otherwise. In both case, dest
 * and dlen will have value and dest have to be free().
 */
static inline bool
a_iso2utf8(const char *str, ssize_t len, char **dest, ssize_t *dlen)
{
    if(draw_iso2utf8(str, len, dest, dlen))
        return true;

    *dest = a_strdup(str);
    if(dlen)
        *dlen = len;

    return false;
}

cairo_surface_t *draw_surface_from_data(int width, int height, uint32_t *data);
cairo_surface_t *draw_dup_image_surface(cairo_surface_t *surface);
cairo_surface_t *draw_load_image(lua_State *L, const char *path);

xcb_visualtype_t *draw_find_visual(const xcb_screen_t *s, xcb_visualid_t visual);
xcb_visualtype_t *draw_default_visual(const xcb_screen_t *s);
xcb_visualtype_t *draw_argb_visual(const xcb_screen_t *s);
uint8_t draw_visual_depth(const xcb_screen_t *s, xcb_visualid_t vis);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
