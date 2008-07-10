/*
 * draw.h - draw functions header
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include "common/array.h"
#include "common/list.h"
#include "common/buffer.h"

typedef struct
{
    unsigned initialized : 1;
    uint32_t pixel;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t alpha;
} xcolor_t;

typedef enum
{
    AlignLeft = 0,
    AlignRight,
    AlignCenter,
    AlignFlex,
    AlignAuto
} alignment_t;

typedef struct vector_t vector_t;
struct vector_t
{
    /** Co-ords of starting point */
    int16_t x;
    int16_t y;
    /** Offset to starting point */
    int16_t x_offset;
    int16_t y_offset;
};

typedef struct area_t area_t;
struct area_t
{
    /** Co-ords of upper left corner */
    int16_t  x;
    int16_t  y;
    uint16_t width;
    uint16_t height;
};

DO_ARRAY(area_t, area, DO_NOTHING);

#define AREA_LEFT(a)    ((a).x)
#define AREA_TOP(a)     ((a).y)
#define AREA_RIGHT(a)   ((a).x + (a).width)
#define AREA_BOTTOM(a)    ((a).y + (a).height)

static inline bool
area_intersect_area(area_t a, area_t b)
{
    return (b.x < a.x + a.width
            && b.x + b.width > a.x
            && b.y < a.y + a.height
            && b.y + b.height > a.y);
}

static inline area_t
area_get_intersect_area(area_t a, area_t b)
{
    area_t g;

    g.x = MAX(a.x, b.x);
    g.y = MAX(a.y, b.y);
    g.width = MIN(a.x + a.width, b.x + b.width) - g.x;
    g.height = MIN(a.y + a.height, b.y + b.height) - g.y;

    return g;
}

typedef struct
{
    PangoFontDescription *desc;
    int height;
} font_t;

typedef struct
{
    xcb_connection_t *connection;
    xcb_pixmap_t pixmap;
    xcb_visualtype_t *visual;
    int width;
    int height;
    int phys_screen;
    int depth;
    cairo_t *cr;
    cairo_surface_t *surface;
    PangoLayout *layout;
    xcolor_t fg;
    xcolor_t bg;
} draw_context_t;

typedef struct
{
    void   *data;
    size_t width;
    size_t height;
} draw_image_t;

draw_context_t *
draw_context_new(xcb_connection_t *, int, int, int, xcb_drawable_t,
                 const xcolor_t *, const xcolor_t*);

/** Delete a draw context.
 * \param ctx The draw_context_t to delete.
 */
static inline void
draw_context_delete(draw_context_t **ctx)
{
    if(*ctx)
    {
        if((*ctx)->layout)
            g_object_unref((*ctx)->layout);
        if((*ctx)->surface)
            cairo_surface_destroy((*ctx)->surface);
        if((*ctx)->cr)
            cairo_destroy((*ctx)->cr);
        p_delete(ctx);
    }
}

font_t *draw_font_new(xcb_connection_t *, int, const char *);
void draw_font_delete(font_t **);

char * draw_iso2utf8(const char *, size_t);

static inline bool
a_iso2utf8(char **dest, const char *str, ssize_t len)
{
    char *utf8;
    if((utf8 = draw_iso2utf8(str, len)))
    {
        *dest = utf8;
        return true;
    }
    *dest = a_strdup(str);
    return false;
}

typedef struct
{
    xcb_connection_t *connection;
    int phys_screen;
    buffer_t text;
    alignment_t align;
    struct
    {
        int left, right;
    } margin;
    bool has_bg_color;
    xcolor_t bg_color;
    draw_image_t *bg_image;
    alignment_t bg_align;
    bool bg_resize;
    struct
    {
        int offset;
        xcolor_t color;
    } shadow;
    struct
    {
        int width;
        xcolor_t color;
    } border;
} draw_parser_data_t;

void draw_parser_data_init(draw_parser_data_t *);
void draw_parser_data_wipe(draw_parser_data_t *);

bool draw_text_markup_expand(draw_parser_data_t *, const char *, ssize_t);

void draw_text(draw_context_t *, font_t *, area_t, const char *, ssize_t len, draw_parser_data_t *);
void draw_rectangle(draw_context_t *, area_t, float, bool, const xcolor_t *);
void draw_rectangle_gradient(draw_context_t *, area_t, float, bool, vector_t,
                             const xcolor_t *, const xcolor_t *, const xcolor_t *);

void draw_graph_setup(draw_context_t *);
void draw_graph(draw_context_t *, area_t, int *, int *, int, position_t, vector_t,
                const xcolor_t *, const xcolor_t *, const xcolor_t *);
void draw_graph_line(draw_context_t *, area_t, int *, int, position_t, vector_t,
                     const xcolor_t *, const xcolor_t *, const xcolor_t *);
draw_image_t *draw_image_new(const char *);
void draw_image_delete(draw_image_t **);
void draw_image(draw_context_t *, int, int, int, draw_image_t *);
void draw_image_from_argb_data(draw_context_t *, int, int, int, int, int, unsigned char *);
void draw_rotate(draw_context_t *, xcb_drawable_t, xcb_drawable_t, int, int, int, int, double, int, int);
area_t draw_text_extents(xcb_connection_t *, int, font_t *, const char *, ssize_t, draw_parser_data_t *);
alignment_t draw_align_fromstr(const char *, ssize_t);
const char *draw_align_tostr(alignment_t);

bool xcolor_init(xcolor_t *c, xcb_connection_t *, int, const char *, ssize_t);

void area_array_remove(area_array_t *, area_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
