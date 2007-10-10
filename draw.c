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

#include "layout.h"
#include "util.h"
#include "draw.h"

void
drawtext(Display *disp, int screen, int x, int y, int w, int h, GC gc, Drawable drawable, XftFont *font, const char *text, XColor color[ColLast])
{
    int nw = 0;
    static char buf[256];
    size_t len, olen;
    XRectangle r = { x, y, w, h };
    XRenderColor xrcolor;
    XftColor xftcolor;
    XftDraw *xftdrawable;

    XSetForeground(disp, gc, color[ColBG].pixel);
    XFillRectangles(disp, drawable, gc, &r, 1);
    if(!text)
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
}

void
drawsquare(Display *disp, int x, int y, int h, GC gc, Drawable drawable, Bool filled, XColor color)
{
    XGCValues gcv;
    XRectangle r = { x, y, h, h };

    gcv.foreground = color.pixel;
    XChangeGC(disp, gc, GCForeground, &gcv);
    if(filled)
    {
        r.width++; r.height++;
        XFillRectangles(disp, drawable, gc, &r, 1);
    }
    else
        XDrawRectangles(disp, drawable, gc, &r, 1);
}


unsigned short
textwidth(Display *disp, XftFont *font, char *text, ssize_t len)
{
    XGlyphInfo gi;
    XftTextExtentsUtf8(disp, font, (FcChar8 *) text, len, &gi);
    return gi.width;
}
