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

#include <cairo.h>
#include <pango/pangocairo.h>

#include <xcb/xcb.h>

#include <oocairo.h>

#include "objects/image.h"
#include "color.h"
#include "common/array.h"

typedef enum
{
    AlignLeft   = (0),
    AlignRight  = (1),
    AlignCenter = (1 << 1),
    AlignTop    = (1 << 2),
    AlignBottom = (1 << 3),
    AlignMiddle = (1 << 5)
} alignment_t;

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

typedef struct
{
    xcb_pixmap_t pixmap;
    uint16_t width;
    uint16_t height;
    cairo_t *cr;
    cairo_surface_t *surface;
    PangoLayout *layout;
    xcolor_t fg;
    xcolor_t bg;
} draw_context_t;

void draw_context_init(draw_context_t *, int, int,
                       xcb_pixmap_t, const xcolor_t *, const xcolor_t *);

/** Wipe a draw context.
 * \param ctx The draw_context_t to wipe.
 */
static inline void
draw_context_wipe(draw_context_t *ctx)
{
    if(ctx->layout)
    {
        g_object_unref(ctx->layout);
        ctx->layout = NULL;
    }
    if(ctx->surface)
    {
        cairo_surface_destroy(ctx->surface);
        ctx->surface = NULL;
    }
    if(ctx->cr)
    {
        cairo_destroy(ctx->cr);
        ctx->cr = NULL;
    }
}

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

typedef struct
{
    PangoAttrList *attr_list;
    char *text;
    ssize_t len;
} draw_text_context_t;

bool draw_text_context_init(draw_text_context_t *, const char *, ssize_t);
void draw_text(draw_context_t *, draw_text_context_t *, PangoEllipsizeMode, PangoWrapMode, alignment_t, alignment_t, area_t);
void draw_rectangle(draw_context_t *, area_t, float, bool, const color_t *);
void draw_image(draw_context_t *, int, int, double, image_t *);
void draw_surface(draw_context_t *, int, int, double, cairo_surface_t *);
void draw_rotate(draw_context_t *, xcb_drawable_t, xcb_drawable_t, int, int, int, int, double, int, int);
area_t draw_text_extents(draw_text_context_t *);
alignment_t draw_align_fromstr(const char *);
const char *draw_align_tostr(alignment_t);
int luaA_surface_from_data(lua_State *L, int width, int height, uint32_t *data);
cairo_surface_t *draw_dup_image_surface(cairo_surface_t *surface);


static inline void
draw_text_context_wipe(draw_text_context_t *pdata)
{
    if(pdata)
    {
        if(pdata->attr_list)
            pango_attr_list_unref(pdata->attr_list);
        p_delete(&pdata->text);
        p_clear(pdata, 1);
    }
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
