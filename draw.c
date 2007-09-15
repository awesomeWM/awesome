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

extern Client *clients, *sel, *stack;   /* global client list and stack */

/* static */

static void
drawtext(Display *disp, DC drawcontext, Statusbar * statusbar, const char *text, unsigned long col[ColLast])
{
    int x, y, w, h;
    static char buf[256];
    unsigned int len, olen;
    XRectangle r = { drawcontext.x, drawcontext.y, drawcontext.w, drawcontext.h };

    XSetForeground(disp, drawcontext.gc, col[ColBG]);
    XFillRectangles(disp, statusbar->drawable, drawcontext.gc, &r, 1);
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
        XmbDrawString(disp, statusbar->drawable, drawcontext.font.set, drawcontext.gc, x, y, buf, len);
    else
        XDrawString(disp, statusbar->drawable, drawcontext.gc, x, y, buf, len);
}

static void
drawsquare(Bool filled, Bool empty, unsigned long col[ColLast], Display *disp, DC drawcontext, Statusbar *statusbar)
{
    int x;
    XGCValues gcv;
    XRectangle r = { drawcontext.x, drawcontext.y, drawcontext.w, drawcontext.h };

    gcv.foreground = col[ColFG];
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

/** Check if at least a client is tagged with tag number t
 * \param t tag number
 * \return True or False
 */
static Bool
isoccupied(unsigned int t)
{
    Client *c;

    for(c = clients; c; c = c->next)
        if(c->tags[t])
            return True;
    return False;
}

/* extern */

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

void
drawstatusbar(Display *disp, DC *drawcontext, awesome_config * awesomeconf)
{
    int x, i;
    drawcontext->x = drawcontext->y = 0;
    for(i = 0; i < awesomeconf->ntags; i++)
    {
        drawcontext->w = textw(drawcontext->font.set, drawcontext->font.xfont, awesomeconf->tags[i], drawcontext->font.height);
        if(awesomeconf->selected_tags[i])
        {
            drawtext(disp, *drawcontext, &awesomeconf->statusbar, awesomeconf->tags[i], drawcontext->sel);
            drawsquare(sel && sel->tags[i], isoccupied(i), drawcontext->sel, disp, *drawcontext, &awesomeconf->statusbar);
        }
        else
        {
            drawtext(disp, *drawcontext, &awesomeconf->statusbar, awesomeconf->tags[i], drawcontext->norm);
            drawsquare(sel && sel->tags[i], isoccupied(i), drawcontext->norm, disp, *drawcontext, &awesomeconf->statusbar);
        }
        drawcontext->x += drawcontext->w;
    }
    drawcontext->w = awesomeconf->statusbar.width;
    drawtext(disp, *drawcontext, &awesomeconf->statusbar, awesomeconf->current_layout->symbol, drawcontext->norm);
    x = drawcontext->x + drawcontext->w;
    drawcontext->w = textw(drawcontext->font.set, drawcontext->font.xfont, awesomeconf->statustext, drawcontext->font.height);
    drawcontext->x = DisplayWidth(disp, DefaultScreen(disp)) - drawcontext->w;
    if(drawcontext->x < x)
    {
        drawcontext->x = x;
        drawcontext->w = DisplayWidth(disp, DefaultScreen(disp)) - x;
    }
    drawtext(disp, *drawcontext, &awesomeconf->statusbar, awesomeconf->statustext, drawcontext->norm);
    if((drawcontext->w = drawcontext->x - x) > awesomeconf->statusbar.height)
    {
        drawcontext->x = x;
        if(sel)
        {
            drawtext(disp, *drawcontext, &awesomeconf->statusbar, sel->name, drawcontext->sel);
            drawsquare(sel->ismax, sel->isfloating, drawcontext->sel, disp, *drawcontext, &awesomeconf->statusbar);
        }
        else
            drawtext(disp, *drawcontext, &awesomeconf->statusbar, NULL, drawcontext->norm);
    }
    XCopyArea(disp, awesomeconf->statusbar.drawable, awesomeconf->statusbar.window, drawcontext->gc, 0, 0, DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, 0, 0);
    XSync(disp, False);
}
