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

#include "config.h"
#include "common/util.h"
#include "common/list.h"
#include "common/xutil.h"

typedef enum
{
    AlignLeft = 0,
    AlignRight,
    AlignCenter,
    AlignFlex,
    AlignAuto
} alignment_t;

typedef struct area_t area_t;
struct area_t
{
    /** Co-ords of upper left corner */
    int x;
    int y;
    int width;
    int height;
    area_t *prev, *next;
};

DO_SLIST(area_t, area, p_delete)

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
    xcb_drawable_t drawable;
    xcb_visualtype_t *visual;
    int width;
    int height;
    int phys_screen;
    int depth;
    cairo_t *cr;
    cairo_surface_t *surface;
    PangoLayout *layout;
} draw_context_t;

draw_context_t *draw_context_new(xcb_connection_t *, int, int, int, xcb_drawable_t);
/** Delete a draw context
 * \param ctx draw_context_t to delete
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

void draw_text(draw_context_t *, font_t *, xcolor_t *, area_t, const char *);
void draw_rectangle(draw_context_t *, area_t, float, bool, xcolor_t);
void draw_rectangle_gradient(draw_context_t *, area_t, float, bool, area_t, xcolor_t *, xcolor_t *, xcolor_t *);

void draw_graph_setup(draw_context_t *);
void draw_graph(draw_context_t *, area_t, int *, int *, int, position_t, area_t, xcolor_t *, xcolor_t *, xcolor_t *);
void draw_graph_line(draw_context_t *, area_t, int *, int, position_t, area_t, xcolor_t *, xcolor_t *, xcolor_t *);
void draw_circle(draw_context_t *, int, int, int, bool, xcolor_t);
void draw_image(draw_context_t *, int, int, int, const char *);
void draw_image_from_argb_data(draw_context_t *, int, int, int, int, int, unsigned char *);
area_t draw_get_image_size(const char *filename);
void draw_rotate(draw_context_t *, xcb_drawable_t, int, int, double, int, int);
area_t draw_text_extents(xcb_connection_t *, int, font_t *, const char *);
alignment_t draw_align_get_from_str(const char *);
bool draw_color_new(xcb_connection_t *, int, const char *, xcolor_t *);

void area_list_remove(area_t **, area_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
