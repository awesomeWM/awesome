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

extern AwesomeConf globalconf;

DrawCtx *
draw_get_context(int phys_screen, int width, int height)
{
    DrawCtx *d = p_new(DrawCtx, 1);

    d->phys_screen = phys_screen;
    d->width = width;
    d->height = height;
    d->depth = DefaultDepth(globalconf.display, phys_screen);
    d->visual = DefaultVisual(globalconf.display, phys_screen);
    d->drawable = XCreatePixmap(globalconf.display,
                                RootWindow(globalconf.display, phys_screen),
                                width, height, d->depth);

    return d;
};

void
draw_free_context(DrawCtx *ctx)
{
    XFreePixmap(globalconf.display, ctx->drawable);
    p_delete(&ctx);
}

void
draw_text(DrawCtx *ctx, int x, int y, int w, int h, XftFont *font, const char *text, XColor fg, XColor bg)
{
    int nw = 0;
    static char buf[256];
    size_t len, olen;
    cairo_font_face_t *font_face;
    cairo_surface_t *surface;
    cairo_t *cr;

    draw_rectangle(ctx, x, y, w, h, True, bg);
    if(!a_strlen(text))
        return;

    surface = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create(surface);
    font_face = cairo_ft_font_face_create_for_pattern(font->pattern);
    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, font->height);
    cairo_set_source_rgb(cr, fg.red / 65535.0, fg.green / 65535.0, fg.blue / 65535.0);

    olen = len = a_strlen(text);
    if(len >= sizeof(buf))
        len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    while(len && (nw = textwidth(font, buf)) > w)
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
draw_rectangle(DrawCtx *ctx, int x, int y, int w, int h, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);
    if(filled)
    {
        cairo_rectangle(cr, x, y, w, h);
        cairo_fill(cr);
    }
    else
        cairo_rectangle(cr, x + 1, y, w - 1, h - 1);

    cairo_stroke(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void
draw_circle(DrawCtx *ctx, int x, int y, int r, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
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

void draw_image_from_argb_data(DrawCtx *ctx, int x, int y, int w, int h,
                               int wanted_h, unsigned char *data)
{
    double ratio;
    cairo_surface_t *surface, *source;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    source = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, w, h, 0);
    cr = cairo_create (surface);
    if(wanted_h > 0 && h > 0)
    {
        ratio = (double) wanted_h / (double) h;
        cairo_scale(cr, ratio, ratio);
        cairo_set_source_surface(cr, source, x / ratio, y / ratio);
    }
    else
        cairo_set_source_surface(cr, source, x, y);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);
}

void
draw_image(DrawCtx *ctx, int x, int y, int wanted_h, const char *filename)
{
    double ratio;
    int h;
    cairo_surface_t *surface, *source;
    cairo_t *cr;

    source = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    surface = cairo_image_surface_create_from_png(filename);
    cr = cairo_create (source);
    if(wanted_h > 0 && (h = cairo_image_surface_get_height(surface)) > 0)
    {
        ratio = (double) wanted_h / (double) h;
        cairo_scale(cr, ratio, ratio);
        cairo_set_source_surface(cr, surface, x / ratio, y / ratio);
    }
    else
        cairo_set_source_surface(cr, surface, x, y);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);


}

Area
draw_get_image_size(const char *filename)
{
    Area size;
    cairo_surface_t *surface;

    surface = cairo_image_surface_create_from_png(filename);
    cairo_image_surface_get_width(surface);
    size.width = cairo_image_surface_get_width(surface);
    size.height = cairo_image_surface_get_height(surface);
    cairo_surface_destroy(surface);

    return size;
}

Drawable
draw_rotate(DrawCtx *ctx, int screen, double angle, int tx, int ty)
{
    cairo_surface_t *surface, *source;
    cairo_t *cr;
    Drawable newdrawable;

    newdrawable = XCreatePixmap(globalconf.display,
                                RootWindow(globalconf.display, screen),
                                ctx->height, ctx->width,
                                ctx->depth);
    surface = cairo_xlib_surface_create(globalconf.display, newdrawable, ctx->visual, ctx->height, ctx->width);
    source = cairo_xlib_surface_create(globalconf.display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
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

static unsigned short
textwidth_primitive(XftFont *font, char *text)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_font_face_t *font_face;
    cairo_text_extents_t te;

    surface = cairo_xlib_surface_create(globalconf.display, DefaultScreen(globalconf.display),
                                        DefaultVisual(globalconf.display, DefaultScreen(globalconf.display)),
                                        DisplayWidth(globalconf.display, DefaultScreen(globalconf.display)),
                                        DisplayHeight(globalconf.display, DefaultScreen(globalconf.display)));
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
textwidth(XftFont *font, char *text)
{
    if (!a_strlen(text))
        return 0;
    return textwidth_primitive(font, text);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
