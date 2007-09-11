/* See LICENSE file for copyright and license details. */

#include "layout.h"

extern DC dc;                   /* global draw context */
extern Client *clients, *sel, *stack;   /* global client list and stack */

/* static */

static void
drawtext(Display *disp, const char *text, unsigned long col[ColLast])
{
    int x, y, w, h;
    static char buf[256];
    unsigned int len, olen;
    XRectangle r = { dc.x, dc.y, dc.w, dc.h };

    XSetForeground(disp, dc.gc, col[ColBG]);
    XFillRectangles(disp, dc.drawable, dc.gc, &r, 1);
    if(!text)
        return;
    w = 0;
    olen = len = a_strlen(text);
    if(len >= sizeof buf)
        len = sizeof buf - 1;
    memcpy(buf, text, len);
    buf[len] = 0;
    h = dc.font.ascent + dc.font.descent;
    y = dc.y + (dc.h / 2) - (h / 2) + dc.font.ascent;
    x = dc.x + (h / 2);
    /* shorten text if necessary */
    while(len && (w = textnw(buf, len)) > dc.w - h)
        buf[--len] = 0;
    if(len < olen)
    {
        if(len > 1)
            buf[len - 1] = '.';
        if(len > 2)
            buf[len - 2] = '.';
        if(len > 3)
            buf[len - 3] = '.';
    }
    if(w > dc.w)
        return;                 /* too long */
    XSetForeground(disp, dc.gc, col[ColFG]);
    if(dc.font.set)
        XmbDrawString(disp, dc.drawable, dc.font.set, dc.gc, x, y, buf, len);
    else
        XDrawString(disp, dc.drawable, dc.gc, x, y, buf, len);
}

static void
drawsquare(Bool filled, Bool empty, unsigned long col[ColLast], Display *disp)
{
    int x;
    XGCValues gcv;
    XRectangle r = { dc.x, dc.y, dc.w, dc.h };

    gcv.foreground = col[ColFG];
    XChangeGC(disp, dc.gc, GCForeground, &gcv);
    x = (dc.font.ascent + dc.font.descent + 2) / 4;
    r.x = dc.x + 1;
    r.y = dc.y + 1;
    if(filled)
    {
        r.width = r.height = x + 1;
        XFillRectangles(disp, dc.drawable, dc.gc, &r, 1);
    }
    else if(empty)
    {
        r.width = r.height = x;
        XDrawRectangles(disp, dc.drawable, dc.gc, &r, 1);
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
textnw(const char *text, unsigned int len)
{
    XRectangle r;

    if(dc.font.set)
    {
        XmbTextExtents(dc.font.set, text, len, NULL, &r);
        return r.width;
    }
    return XTextWidth(dc.font.xfont, text, len);
}

void
drawstatus(Display *disp, awesome_config * awesomeconf)
{
    int x, i;
    dc.x = dc.y = 0;
    for(i = 0; i < awesomeconf->ntags; i++)
    {
        dc.w = textw(awesomeconf->tags[i]);
        if(awesomeconf->selected_tags[i])
        {
            drawtext(disp, awesomeconf->tags[i], dc.sel);
            drawsquare(sel && sel->tags[i], isoccupied(i), dc.sel, disp);
        }
        else
        {
            drawtext(disp, awesomeconf->tags[i], dc.norm);
            drawsquare(sel && sel->tags[i], isoccupied(i), dc.norm, disp);
        }
        dc.x += dc.w;
    }
    dc.w = awesomeconf->statusbar.width;
    drawtext(disp, awesomeconf->current_layout->symbol, dc.norm);
    x = dc.x + dc.w;
    dc.w = textw(awesomeconf->statustext);
    dc.x = DisplayWidth(disp, DefaultScreen(disp)) - dc.w;
    if(dc.x < x)
    {
        dc.x = x;
        dc.w = DisplayWidth(disp, DefaultScreen(disp)) - x;
    }
    drawtext(disp, awesomeconf->statustext, dc.norm);
    if((dc.w = dc.x - x) > awesomeconf->statusbar.height)
    {
        dc.x = x;
        if(sel)
        {
            drawtext(disp, sel->name, dc.sel);
            drawsquare(sel->ismax, sel->isfloating, dc.sel, disp);
        }
        else
            drawtext(disp, NULL, dc.norm);
    }
    XCopyArea(disp, dc.drawable, awesomeconf->statusbar.window, dc.gc, 0, 0, DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, 0, 0);
    XSync(disp, False);
}
