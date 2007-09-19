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
drawtext(Display *disp, DC drawcontext, Drawable drawable, const char *text, unsigned long col[ColLast])
{
    int x, y, w, h;
    static char buf[256];
    unsigned int len, olen;
    XRectangle r = { drawcontext.x, drawcontext.y, drawcontext.w, drawcontext.h };

    XSetForeground(disp, drawcontext.gc, col[ColBG]);
    XFillRectangles(disp, drawable, drawcontext.gc, &r, 1);
    if(!text)
        return;
    w = 0;
    olen = len = a_strlen(text);
    if(len >= sizeof buf)
        len = sizeof buf - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    h = drawcontext.font.ascent + drawcontext.font.descent;
    y = drawcontext.y + (drawcontext.h / 2) - (h / 2) + drawcontext.font.ascent;
    x = drawcontext.x + (h / 2);
    /* shorten text if necessary */
    while(len && (w = textnw(drawcontext.font.set, drawcontext.font.xfont, buf, len)) > drawcontext.w - h)
        buf[--len] = 0;
    if(w > drawcontext.w)
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
    XSetForeground(disp, drawcontext.gc, col[ColFG]);
    if(drawcontext.font.set)
        XmbDrawString(disp, drawable, drawcontext.font.set, drawcontext.gc, x, y, buf, len);
    else
        XDrawString(disp, drawable, drawcontext.gc, x, y, buf, len);
}

void
drawsquare(Display *disp, DC drawcontext, Bool filled, Bool empty, unsigned long col, Statusbar *statusbar)
{
    int x;
    XGCValues gcv;
    XRectangle r = { drawcontext.x, drawcontext.y, drawcontext.w, drawcontext.h };

    gcv.foreground = col;
    XChangeGC(disp, drawcontext.gc, GCForeground, &gcv);
    x = (drawcontext.font.ascent + drawcontext.font.descent + 2) / 4;
    r.x = drawcontext.x + 1;
    r.y = drawcontext.y + 1;
    if(filled)
    {
        r.width = r.height = x + 1;
        XFillRectangles(disp, statusbar->drawable, drawcontext.gc, &r, 1);
    }
    else if(empty)
    {
        r.width = r.height = x;
        XDrawRectangles(disp, statusbar->drawable, drawcontext.gc, &r, 1);
    }
}

unsigned int
textnw(XFontSet set, XFontStruct *xfont, const char *text, unsigned int len)
{
    XRectangle r;

    if(set)
    {
        XmbTextExtents(set, text, len, NULL, &r);
        return r.width;
    }
    return XTextWidth(xfont, text, len);
}

