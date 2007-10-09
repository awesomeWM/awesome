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

#include <stdio.h>

#include "layout.h"
#include "statusbar.h"
#include "draw.h"
#include "screen.h"
#include "util.h"
#include "layouts/tile.h"

extern Client *clients, *sel;   /* global client list */

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
drawstatusbar(Display *disp, DC *drawcontext, awesome_config * awesomeconf)
{
    int x, i;
    ScreenInfo *si = get_screen_info(disp, awesomeconf->screen, NULL);

    drawcontext->x = drawcontext->y = 0;
    for(i = 0; i < awesomeconf->ntags; i++)
    {
        drawcontext->w = textwidth(disp, drawcontext->font, awesomeconf->tags[i].name, a_strlen(awesomeconf->tags[i].name)) + drawcontext->font->height;
        if(awesomeconf->tags[i].selected)
        {
            drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, awesomeconf->tags[i].name, drawcontext->sel, drawcontext->text_selected);
            if(isoccupied(i, awesomeconf->screen))
                drawsquare(disp, *drawcontext, awesomeconf->statusbar.drawable, sel && sel->tags[i], drawcontext->sel[ColFG]);
        }
        else
        {
            drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, awesomeconf->tags[i].name, drawcontext->norm, drawcontext->text_normal);
            if(isoccupied(i, awesomeconf->screen))
                drawsquare(disp, *drawcontext, awesomeconf->statusbar.drawable, sel && sel->tags[i], drawcontext->norm[ColFG]);
        }
        drawcontext->x += drawcontext->w;
    }
    drawcontext->w = awesomeconf->statusbar.width;
    drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, awesomeconf->current_layout->symbol, drawcontext->norm, drawcontext->text_normal);
    x = drawcontext->x + drawcontext->w;
    drawcontext->w = textwidth(disp, drawcontext->font, awesomeconf->statustext, a_strlen(awesomeconf->statustext)) + drawcontext->font->height;
    drawcontext->x = si[awesomeconf->screen].width - drawcontext->w;
    if(drawcontext->x < x)
    {
        drawcontext->x = x;
        drawcontext->w = si[awesomeconf->screen].width - x;
    }
    drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, awesomeconf->statustext, drawcontext->norm, drawcontext->text_normal);
    if((drawcontext->w = drawcontext->x - x) > awesomeconf->statusbar.height)
    {
        drawcontext->x = x;
        if(sel)
        {
            drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, sel->name, drawcontext->sel, drawcontext->text_selected);
            if(sel->isfloating)
                drawsquare(disp, *drawcontext, awesomeconf->statusbar.drawable, sel->ismax, drawcontext->sel[ColFG]);
        }
        else if(IS_ARRANGE(layout_tile) || IS_ARRANGE(layout_tileleft))
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "nmaster: %d ncol: %d mwfact: %.2lf", awesomeconf->nmaster, awesomeconf->ncol, awesomeconf->mwfact);
            drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, buf, drawcontext->norm, drawcontext->text_normal);
        }
        else
            drawtext(disp, awesomeconf->phys_screen, drawcontext->x, drawcontext->y, drawcontext->w, drawcontext->h, drawcontext->gc, awesomeconf->statusbar.drawable, drawcontext->font, NULL, drawcontext->norm, drawcontext->text_normal);
    }
    XCopyArea(disp, awesomeconf->statusbar.drawable, awesomeconf->statusbar.window, drawcontext->gc, 0, 0, si[awesomeconf->screen].width, awesomeconf->statusbar.height, 0, 0);
    XSync(disp, False);
}

void
initstatusbar(Display *disp, int screen, DC *drawcontext, Statusbar *statusbar)
{
    XSetWindowAttributes wa;
    int phys_screen;
    ScreenInfo *si;

    phys_screen = get_phys_screen(disp, screen);

    statusbar->screen = screen;

    si = get_screen_info(disp, screen, NULL);

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = drawcontext->cursor[CurNormal];
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0, si[screen].width,
                                      statusbar->height, 0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                      DefaultVisual(disp, phys_screen), CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, statusbar->window, drawcontext->cursor[CurNormal]);
    updatebarpos(disp, *statusbar);
    XMapRaised(disp, statusbar->window);
    statusbar->drawable = XCreatePixmap(disp,
                                        RootWindow(disp, phys_screen),
                                        si[screen].width,
                                        statusbar->height,
                                        DefaultDepth(disp, phys_screen));
}

void
updatebarpos(Display *disp, Statusbar statusbar)
{
    XEvent ev;
    ScreenInfo *si = get_screen_info(disp, statusbar.screen, NULL);

    switch (statusbar.position)
    {
      default:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].y_org);
        break;
      case BarBot:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].height - statusbar.height);
        break;
      case BarOff:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].y_org - statusbar.height);
        break;
    }
    XFree(si);
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
        awesomeconf->statusbar.position = (awesomeconf->statusbar_default_position == BarOff) ? BarTop : awesomeconf->statusbar_default_position;
    else
        awesomeconf->statusbar.position = BarOff;
    updatebarpos(disp, awesomeconf->statusbar);
    arrange(disp, drawcontext, awesomeconf);
}

