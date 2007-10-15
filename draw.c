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
#include <cairo-xlib.h>
#include <math.h>
#include "layout.h"
#include "util.h"
#include "draw.h"

void
drawtext(Display *disp, int screen, int x, int y, int w, int h, Drawable drawable, int dw, int dh, XftFont *font, const char *text, XColor color[ColLast])
{
    int nw = 0;
    static char buf[256];
    size_t len, olen;
    XRenderColor xrcolor;
    XftColor xftcolor;
    XftDraw *xftdrawable;

    drawrectangle(disp, screen, x, y, w, h, drawable, dw, dh, True, color[ColBG]);
    if(!a_strlen(text))
        return;
    olen = len = a_strlen(text);
    if(len >= sizeof(buf))
        len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    while(len && (nw = textwidth(disp, font, buf, len)) > w - font->height)
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
    xrcolor.red = color[ColFG].red;
    xrcolor.green = color[ColFG].green;
    xrcolor.blue = color[ColFG].blue;
    XftColorAllocValue(disp, DefaultVisual(disp, screen), DefaultColormap(disp, screen), &xrcolor, &xftcolor);
    xftdrawable = XftDrawCreate(disp, drawable, DefaultVisual(disp, screen), DefaultColormap(disp, screen));
    XftDrawStringUtf8(xftdrawable, &xftcolor, font,
                      x + (font->height / 2),
                      y + (h / 2) - (font->height / 2) + font->ascent,
                      (FcChar8 *) buf, len);
    XftColorFree(disp, DefaultVisual(disp, screen), DefaultColormap(disp, screen), &xftcolor);
    XftDrawDestroy(xftdrawable);
}

void
drawrectangle(Display *disp, int screen, int x, int y, int w, int h, Drawable drawable, int dw, int dh, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(disp, drawable, DefaultVisual(disp, screen), dw, dh);
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
drawcircle(Display *disp, int screen, int x, int y, int r, Drawable drawable, int dw, int dh, Bool filled, XColor color)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(disp, drawable, DefaultVisual(disp, screen), dw, dh);
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

unsigned short
textwidth(Display *disp, XftFont *font, char *text, ssize_t len)
{
    XGlyphInfo gi;
    XftTextExtentsUtf8(disp, font, (FcChar8 *) text, len, &gi);
    return gi.width;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
