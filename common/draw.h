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
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "common/util.h"
#include "common/list.h"

typedef enum
{
    AlignLeft,
    AlignRight,
    AlignFlex,
    AlignCenter
} Alignment;

typedef struct Area Area;
struct Area
{
    /** Co-ords of upper left corner */
    int x;
    int y;
    int width;
    int height;
    Area *next;
};

DO_SLIST(Area, area, p_delete);

#define AREA_LEFT(a)    ((a).x)
#define AREA_TOP(a)     ((a).y)
#define AREA_RIGHT(a)   ((a).x + (a).width)
#define AREA_BOTTOM(a)    ((a).y + (a).height)

static inline Bool
area_intersect_area(Area a, Area b)
{
    return (b.x < a.x + a.width
            && b.x + b.width > a.x
            && b.y < a.y + a.height
            && b.y + b.height > a.y);
}

static inline Area
area_get_intersect_area(Area a, Area b)
{
    Area g;

    g.x = MAX(a.x, b.x);
    g.y = MAX(a.y, b.y);
    g.width = MIN(a.x + a.width, b.x + b.width) - g.x;
    g.height = MIN(a.y + a.height, b.y + b.height) - g.y;

    return g;
}

typedef struct
{
    Display *display;
    Drawable drawable;
    Visual *visual;
    int width;
    int height;
    int phys_screen;
    int depth;
    cairo_t *cr;
    cairo_surface_t *surface;
} DrawCtx;

DrawCtx *draw_context_new(Display *, int, int, int, Drawable);
void draw_context_delete(DrawCtx *);

void draw_text(DrawCtx *, Area, Alignment, int, XftFont *, const char *, XColor fg, XColor bg);
void draw_rectangle(DrawCtx *, Area, Bool, XColor);
void draw_rectangle_gradient(DrawCtx *, Area, int, Bool, XColor, XColor);

void draw_graph_setup(DrawCtx *);
void draw_graph(DrawCtx *, int, int, int, int *, int *, int, XColor);
void draw_graph_line(DrawCtx *, int, int, int, int *, int, XColor);

void draw_circle(DrawCtx *, int, int, int, Bool, XColor);
void draw_image(DrawCtx *, int, int, int, const char *);
void draw_image_from_argb_data(DrawCtx *, int, int, int, int, int, unsigned char *);
Area draw_get_image_size(const char *filename);
Drawable draw_rotate(DrawCtx *, int, double, int, int);
unsigned short draw_textwidth(Display *, XftFont *, char *);
Alignment draw_get_align(const char *);
XColor draw_color_new(Display *, int, const char *);

void area_list_remove(Area **, Area *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
