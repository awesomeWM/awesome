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

extern Client *clients, *sel, *stack;   /* global client list and stack */

/* static */

void
drawtext(Display *disp, int screen, DC *drawcontext, Drawable drawable, const char *text, unsigned long col[ColLast], XColor textcolor)
{
    int x, y, w, h;
    static char buf[256];
    size_t len, olen;
    XRectangle r = { drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h };
    XRenderColor xrcolor;
    XftColor xftcolor;
    XftDraw *xftdrawable;

    XSetForeground(disp, drawcontext->gc, col[ColBG]);
    XFillRectangles(disp, drawable, drawcontext->gc, &r, 1);
    if(!text)
        return;
    w = 0;
    olen = len = a_strlen(text);
    if(len >= sizeof(buf))
        len = sizeof(buf) - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    h = drawcontext->font->ascent + drawcontext->font->descent;
    y = drawcontext->y + (drawcontext->h / 2) - (h / 2) + drawcontext->font->ascent;
    x = drawcontext->x + (h / 2);
    while(len && (w = textwidth(disp, drawcontext->font, buf, len)) > drawcontext->w - h)
        buf[--len] = 0;
    if(w > drawcontext->w)
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
    xrcolor.red = textcolor.red;
    xrcolor.green = textcolor.green;
    xrcolor.blue = textcolor.blue;
    XftColorAllocValue(disp, DefaultVisual(disp, screen), DefaultColormap(disp, screen), &xrcolor, &xftcolor);
    xftdrawable = XftDrawCreate(disp, drawable, DefaultVisual(disp, screen), DefaultColormap(disp, screen));
    XftDrawStringUtf8(xftdrawable, &xftcolor, drawcontext->font, x, y, (FcChar8 *) buf, len);
    XftColorFree(disp, DefaultVisual(disp, screen), DefaultColormap(disp, screen), &xftcolor);
}

void
drawsquare(Display *disp, DC drawcontext, Drawable drawable, Bool filled, unsigned long col)
{
    int x;
    XGCValues gcv;
    XRectangle r = { drawcontext.x, drawcontext.y, drawcontext.w, drawcontext.h };

    x = (drawcontext.font->ascent + drawcontext.font->descent + 2) / 4;
    gcv.foreground = col;
    XChangeGC(disp, drawcontext.gc, GCForeground, &gcv);
    r.x = drawcontext.x + 1;
    r.y = drawcontext.y + 1;
    r.width = r.height = x;
    if(filled)
    {
        r.width++; r.height++;
        XFillRectangles(disp, drawable, drawcontext.gc, &r, 1);
    }
    else
        XDrawRectangles(disp, drawable, drawcontext.gc, &r, 1);
}


unsigned short
textwidth(Display *disp, XftFont *font, char *text, ssize_t len)
{
    XGlyphInfo gi;
    XftTextExtentsUtf8(disp, font, (FcChar8 *) text, len, &gi);
    return gi.width;
}
