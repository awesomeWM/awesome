/*
 * draw.h - draw functions header
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_DRAW_H
#define AWESOME_DRAW_H

#include "config.h"

typedef struct DrawCtx DrawCtx;
struct DrawCtx
{
    Display *display;
    Drawable drawable;
    Visual *visual;
    int width;
    int height;
    int phys_screen;
    int depth;
};

DrawCtx *draw_get_context(Display*, int, int, int);
void draw_free_context(DrawCtx*);
void drawtext(DrawCtx *, int, int, int, int, XftFont *, const char *, XColor fg, XColor bg);
void drawrectangle(DrawCtx *, int, int, int, int, Bool, XColor);
void drawcircle(DrawCtx *, int, int, int, Bool, XColor);
void drawimage(DrawCtx *, int, int, const char *);
void draw_image_from_argb_data(DrawCtx *, int, int, int, int, int, unsigned char *);
int draw_get_image_width(const char *filename);
Drawable draw_rotate(DrawCtx *, int, double, int, int);
unsigned short textwidth(DrawCtx *, XftFont *, char *);
unsigned short textwidth_primitive(Display*, XftFont*, char*);
#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
