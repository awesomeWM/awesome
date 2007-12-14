/*
 * draw.c - draw functions
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

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-xlib.h>
#include <math.h>
#include "layout.h"
#include "util.h"
#include "draw.h"


DrawCtx *
draw_get_context(Display *display, int phys_screen, int width, int height)
{
    DrawCtx *d;
    d = p_new(DrawCtx, 1);
    d->phys_screen = phys_screen;
    d->display = display;
    d->width = width;
    d->height = height;
    d->depth = DefaultDepth(display, phys_screen);
    d->visual = DefaultVisual(display, phys_screen);
    d->drawable = XCreatePixmap(display,
                                RootWindow(display, phys_screen),
                                width, height, d->depth);
    return d;
};

void 
draw_free_context(DrawCtx *ctx)
{
    XFreePixmap(ctx->display, ctx->drawable);
    p_delete(&ctx);
}

void
drawtext(DrawCtx *ctx, int x, int y, int w, int h, XftFont *font, const char *text, XColor color[ColLast])
{
    int nw = 0;
    static char buf[256];
    size_t len, olen;
    cairo_font_face_t *font_face;
    cairo_surface_t *surface;
    cairo_t *cr;

    drawrectangle(ctx, x, y, w, h, True, color[ColBG]);
    if(!a_strlen(text))
        return;

    surface = cairo_xlib_surface_create(ctx->display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create(surface);
    font_face = cairo_ft_font_face_create_for_pattern(font->pattern);
    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, font->height);
    cairo_set_source_rgb(cr, color[ColFG].red / 65535.0, color[ColFG].green / 65535.0, color[ColFG].blue / 65535.0);

    olen = len = a_strlen(text);
    if(len >= sizeof(buf))
        len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    while(len && (nw = textwidth(ctx, font, buf)) > w)
        buf[--len] = 0;
    if(nw > w)
        return;                 /* too long */
    if(len < olen)
    {
        if(len > 1)
            buf[len - 1] = '.';
        if(len > 2)
            buf[len - 2] = '.';
        if(len > 3)
            buf[len - 3] = '.';
    }

    cairo_move_to(cr, x + font->height / 2, y + font->ascent + (ctx->height - font->height) / 2);
    cairo_show_text(cr, buf);

    cairo_font_face_destroy(font_face);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void
drawrectangle(DrawCtx *ctx, int x, int y, int w, int h, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(ctx->display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);
    if(filled)
    {
        cairo_rectangle(cr, x, y, w + 1, h + 1);
        cairo_fill(cr);
    }
    else
        cairo_rectangle(cr, x + 1, y, w, h);

    cairo_stroke(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void
drawcircle(DrawCtx *ctx, int x, int y, int r, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(ctx->display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);
    if(filled)
    {
        cairo_arc (cr, x + r, y + r, r, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    else
        cairo_arc (cr, x + r, y + r, r - 1, 0, 2 * M_PI);

    cairo_stroke(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

Drawable
draw_rotate(DrawCtx *ctx, int screen, double angle, int tx, int ty)
{
    cairo_surface_t *surface, *source;
    cairo_t *cr;
    Drawable newdrawable;

    newdrawable = XCreatePixmap(ctx->display,
                                RootWindow(ctx->display, screen),
                                ctx->height, ctx->width,
                                ctx->depth);
    surface = cairo_xlib_surface_create(ctx->display, newdrawable, ctx->visual, ctx->height, ctx->width);
    source = cairo_xlib_surface_create(ctx->display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);

    cairo_translate(cr, tx, ty);
    cairo_rotate(cr, angle);

    cairo_set_source_surface(cr, source, 0.0, 0.0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);

    return newdrawable;
}


unsigned short
textwidth_primitive(Display *display, XftFont *font, char *text)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_font_face_t *font_face;
    cairo_text_extents_t te;

    surface = cairo_xlib_surface_create(display, DefaultScreen(display),
                                        DefaultVisual(display, DefaultScreen(display)),
                                        DisplayWidth(display, DefaultScreen(display)),
                                        DisplayHeight(display, DefaultScreen(display)));
    cr = cairo_create(surface);
    font_face = cairo_ft_font_face_create_for_pattern(font->pattern);
    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, font->height);
    cairo_text_extents(cr, text, &te);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(font_face);

    return MAX(te.x_advance, te.width) + font->height;
}

unsigned short
textwidth(DrawCtx *ctx, XftFont *font, char *text)
{
    return textwidth_primitive(ctx->display, font, text);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
