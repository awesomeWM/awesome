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

#include <confuse.h>

#include <X11/Xlib.h>
#include <pango/pangocairo.h>

#include "common/util.h"
#include "common/list.h"

typedef enum
{
    AlignLeft,
    AlignRight,
    AlignCenter,
    AlignFlex,
    AlignAuto
} Alignment;

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

static inline Bool
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
    /** Foreground color */
    XColor fg;
    /** Background color */
    XColor bg;
    /** Shadow color */
    XColor shadow;
    /** Border color */
    XColor border;
    /** Shadow offset */
    int shadow_offset;
    /** Font */
    font_t *font;
} style_t;

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
    PangoLayout *layout;
} DrawCtx;

DrawCtx *draw_context_new(Display *, int, int, int, Drawable);
void draw_context_delete(DrawCtx **);

font_t *draw_font_new(Display *disp, char *fontname);
void draw_font_delete(font_t **);

void draw_text(DrawCtx *, area_t, Alignment, int, char *, style_t);
void draw_rectangle(DrawCtx *, area_t, Bool, XColor);
void draw_rectangle_gradient(DrawCtx *, area_t, Bool, area_t, XColor *, XColor *, XColor *);

void draw_graph_setup(DrawCtx *);
void draw_graph(DrawCtx *, area_t, int *, int *, int, Position, area_t, XColor *, XColor *, XColor *);
void draw_graph_line(DrawCtx *, area_t, int *, int, Position, area_t, XColor *, XColor *, XColor *);
void draw_circle(DrawCtx *, int, int, int, Bool, XColor);
void draw_image(DrawCtx *, int, int, int, const char *);
void draw_image_from_argb_data(DrawCtx *, int, int, int, int, int, unsigned char *);
area_t draw_get_image_size(const char *filename);
void draw_rotate(DrawCtx *, Drawable, int, int, double, int, int);
unsigned short draw_textwidth(Display *, font_t *, char *);
Alignment draw_align_get_from_str(const char *);
Bool draw_color_new(Display *, int, const char *, XColor *);
void draw_style_init(Display *, int, cfg_t *, style_t *, style_t *);

void area_list_remove(area_t **, area_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
