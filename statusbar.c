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
#include "statusbar.h"
#include "draw.h"
#include "screen.h"
#include "util.h"

extern Client *clients, *sel, *stack;   /* global client list and stack */

/** Check if at least a client is tagged with tag number t and is on screen
 * screen
 * \param t tag number
 * \param screen screen number
 * \return True or False
 */
static Bool
isoccupied(unsigned int t, int screen)
{
    Client *c;

    for(c = clients; c; c = c->next)
        if(c->tags[t] && c->screen == screen)
            return True;
    return False;
}

void
drawstatusbar(Display *disp, int screen, DC *drawcontext, awesome_config * awesomeconf)
{
    int x, i;
    drawcontext->x = drawcontext->y = 0;
    for(i = 0; i < awesomeconf->ntags; i++)
    {
        drawcontext->w = textw(drawcontext->font.set, drawcontext->font.xfont, awesomeconf->tags[i], drawcontext->font.height);
        if(awesomeconf->selected_tags[i])
        {
            drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, awesomeconf->tags[i], drawcontext->sel);
            drawsquare(disp, *drawcontext, sel && sel->tags[i], isoccupied(i, screen), drawcontext->sel, &awesomeconf->statusbar);
        }
        else
        {
            drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, awesomeconf->tags[i], drawcontext->norm);
            drawsquare(disp, *drawcontext, sel && sel->tags[i], isoccupied(i, screen), drawcontext->norm, &awesomeconf->statusbar);
        }
        drawcontext->x += drawcontext->w;
    }
    drawcontext->w = awesomeconf->statusbar.width;
    drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, awesomeconf->current_layout->symbol, drawcontext->norm);
    x = drawcontext->x + drawcontext->w;
    drawcontext->w = textw(drawcontext->font.set, drawcontext->font.xfont, awesomeconf->statustext, drawcontext->font.height);
    drawcontext->x = DisplayWidth(disp, screen) - drawcontext->w;
    if(drawcontext->x < x)
    {
        drawcontext->x = x;
        drawcontext->w = DisplayWidth(disp, screen) - x;
    }
    drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, awesomeconf->statustext, drawcontext->norm);
    if((drawcontext->w = drawcontext->x - x) > awesomeconf->statusbar.height)
    {
        drawcontext->x = x;
        if(sel)
        {
            drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, sel->name, drawcontext->sel);
            drawsquare(disp, *drawcontext, sel->ismax, sel->isfloating, drawcontext->sel, &awesomeconf->statusbar);
        }
        else
            drawtext(disp, *drawcontext, awesomeconf->statusbar.drawable, NULL, drawcontext->norm);
    }
    XCopyArea(disp, awesomeconf->statusbar.drawable, awesomeconf->statusbar.window, drawcontext->gc, 0, 0, DisplayWidth(disp, screen), awesomeconf->statusbar.height, 0, 0);
    XSync(disp, False);
}

void
initstatusbar(Display *disp, int screen, DC *drawcontext, Statusbar *statusbar)
{
    XSetWindowAttributes wa;

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = drawcontext->cursor[CurNormal];
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    statusbar->window = XCreateWindow(disp, RootWindow(disp, screen), 0, 0, DisplayWidth(disp, screen),
                                      statusbar->height, 0, DefaultDepth(disp, screen), CopyFromParent,
                                      DefaultVisual(disp, screen), CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, statusbar->window, drawcontext->cursor[CurNormal]);
    updatebarpos(disp, *statusbar);
    XMapRaised(disp, statusbar->window);
    statusbar->drawable = XCreatePixmap(disp,
                                        RootWindow(disp, screen),
                                        DisplayWidth(disp, screen),
                                        statusbar->height,
                                        DefaultDepth(disp, screen));
}

void
updatebarpos(Display *disp, Statusbar statusbar)
{
    XEvent ev;
    ScreenInfo *si;

    switch (statusbar.position)
    {
      default:
        XMoveWindow(disp, statusbar.window, 0, 0);
        break;
      case BarBot:
        si = get_display_info(disp, statusbar.screen, statusbar);
        XMoveWindow(disp, statusbar.window, 0, si->height);
        XFree(si);
        break;
      case BarOff:
        XMoveWindow(disp, statusbar.window, 0, 0 - statusbar.height);
        break;
    }
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

void
uicb_togglebar(Display *disp,
               DC *drawcontext,
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    if(awesomeconf->statusbar.position == BarOff)
        awesomeconf->statusbar.position = (awesomeconf->statusbar.position == BarOff) ? BarTop : awesomeconf->statusbar_default_position;
    else
        awesomeconf->statusbar.position = BarOff;
    updatebarpos(disp, awesomeconf->statusbar);
    arrange(disp, drawcontext, awesomeconf);
}

