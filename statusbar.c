/*
 * statusbar.c - statusbar functions
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

/** Check if at least a client is tagged with tag number t and is on screen
 * screen
 * \param t tag number
 * \param screen screen number
 * \return True or False
 */
static Bool
isoccupied(Client **head, unsigned int t, int screen)
{
    Client *c;

    for(c = *head; c; c = c->next)
        if(c->tags[t] && c->screen == screen)
            return True;
    return False;
}

void
drawstatusbar(awesome_config * awesomeconf)
{
    int z, i, x = 0, y = 0, w;

    for(i = 0; i < awesomeconf->ntags; i++)
    {
        w = textwidth(awesomeconf->display, awesomeconf->font,
                      awesomeconf->tags[i].name);
        if(awesomeconf->tags[i].selected)
        {
            drawtext(awesomeconf->display, awesomeconf->phys_screen,
                     x, y, w,
                     awesomeconf->statusbar.height,
                     awesomeconf->statusbar.drawable,
                     awesomeconf->statusbar.width,
                     awesomeconf->statusbar.height,
                     awesomeconf->font,
                     awesomeconf->tags[i].name, awesomeconf->colors_selected);
            if(isoccupied(awesomeconf->clients, i, awesomeconf->screen))
                drawrectangle(awesomeconf->display, awesomeconf->phys_screen,
                              x, y,
                              (awesomeconf->font->height + 2) / 4,
                              (awesomeconf->font->height + 2) / 4,
                              awesomeconf->statusbar.drawable,
                              awesomeconf->statusbar.width,
                              awesomeconf->statusbar.height,
                              *awesomeconf->client_sel && (*awesomeconf->client_sel)->tags[i],
                              awesomeconf->colors_selected[ColFG]);
        }
        else
        {
            drawtext(awesomeconf->display, awesomeconf->phys_screen,
                     x, y, w,
                     awesomeconf->statusbar.height,
                     awesomeconf->statusbar.drawable,
                     awesomeconf->statusbar.width,
                     awesomeconf->statusbar.height,
                     awesomeconf->font,
                     awesomeconf->tags[i].name, awesomeconf->colors_normal);
            if(isoccupied(awesomeconf->clients, i, awesomeconf->screen))
                drawrectangle(awesomeconf->display, awesomeconf->phys_screen,
                              x, y,
                              (awesomeconf->font->height + 2) / 4,
                              (awesomeconf->font->height + 2) / 4,
                              awesomeconf->statusbar.drawable,
                              awesomeconf->statusbar.width,
                              awesomeconf->statusbar.height,
                              *awesomeconf->client_sel && (*awesomeconf->client_sel)->tags[i],
                              awesomeconf->colors_normal[ColFG]);
        }
        x += w;
    }
    drawtext(awesomeconf->display, awesomeconf->phys_screen,
             x, y, awesomeconf->statusbar.txtlayoutwidth,
             awesomeconf->statusbar.height,
             awesomeconf->statusbar.drawable,
             awesomeconf->statusbar.width,
             awesomeconf->statusbar.height,
             awesomeconf->font,
             get_current_layout(awesomeconf->tags, awesomeconf->ntags)->symbol,
             awesomeconf->colors_normal);
    z = x + awesomeconf->statusbar.txtlayoutwidth;
    w = textwidth(awesomeconf->display, awesomeconf->font, awesomeconf->statustext);
    x = awesomeconf->statusbar.width - w;
    if(x < z)
    {
        x = z;
        w = awesomeconf->statusbar.width - z;
    }
    drawtext(awesomeconf->display, awesomeconf->phys_screen,
             x, y, w,
             awesomeconf->statusbar.height,
             awesomeconf->statusbar.drawable,
             awesomeconf->statusbar.width,
             awesomeconf->statusbar.height,
             awesomeconf->font,
             awesomeconf->statustext, awesomeconf->colors_normal);
    if((w = x - z) > awesomeconf->statusbar.height)
    {
        x = z;
        if(*awesomeconf->client_sel && (*awesomeconf->client_sel)->screen == awesomeconf->screen)
        {
            drawtext(awesomeconf->display, awesomeconf->phys_screen,
                     x, y, w,
                     awesomeconf->statusbar.height,
                     awesomeconf->statusbar.drawable,
                     awesomeconf->statusbar.width,
                     awesomeconf->statusbar.height,
                     awesomeconf->font,
                     (*awesomeconf->client_sel)->name, awesomeconf->colors_selected);
            if((*awesomeconf->client_sel)->isfloating)
                drawcircle(awesomeconf->display, awesomeconf->phys_screen,
                           x, y,
                           (awesomeconf->font->height + 2) / 4,
                           awesomeconf->statusbar.drawable,
                           awesomeconf->statusbar.width,
                           awesomeconf->statusbar.height,
                           (*awesomeconf->client_sel)->ismax,
                           awesomeconf->colors_selected[ColFG]);
        }
        else
            drawtext(awesomeconf->display, awesomeconf->phys_screen,
                     x, y, w,
                     awesomeconf->statusbar.height,
                     awesomeconf->statusbar.drawable,
                     awesomeconf->statusbar.width,
                     awesomeconf->statusbar.height,
                     awesomeconf->font,
                     NULL, awesomeconf->colors_normal);
    }
    XCopyArea(awesomeconf->display, awesomeconf->statusbar.drawable,
              awesomeconf->statusbar.window, DefaultGC(awesomeconf->display, awesomeconf->phys_screen), 0, 0,
              awesomeconf->statusbar.width, awesomeconf->statusbar.height, 0, 0);
    XSync(awesomeconf->display, False);
}

void
initstatusbar(Display *disp, int screen, Statusbar *statusbar, Cursor cursor, XftFont *font, Layout *layouts, int nlayouts)
{
    XSetWindowAttributes wa;
    int i, phys_screen = get_phys_screen(disp, screen);
    ScreenInfo *si = get_screen_info(disp, screen, NULL);

    statusbar->width = si[screen].width;
    statusbar->height = font->height * 1.5;
    p_delete(&si);

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = cursor;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0,
                                      statusbar->width,
                                      statusbar->height,
                                      0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                      DefaultVisual(disp, phys_screen),
                                      CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, statusbar->window, cursor);
    updatebarpos(disp, *statusbar);
    XMapRaised(disp, statusbar->window);
    statusbar->drawable = XCreatePixmap(disp,
                                        RootWindow(disp, phys_screen),
                                        statusbar->width,
                                        statusbar->height,
                                        DefaultDepth(disp, phys_screen));

    for(i = 0; i < nlayouts; i++)
        statusbar->txtlayoutwidth = MAX(statusbar->txtlayoutwidth,
                                        (textwidth(disp, font, layouts[i].symbol)));
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
    p_delete(&si);
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

void
uicb_togglebar(awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    if(awesomeconf->statusbar.position == BarOff)
        awesomeconf->statusbar.position = (awesomeconf->statusbar_default_position == BarOff) ? BarTop : awesomeconf->statusbar_default_position;
    else
        awesomeconf->statusbar.position = BarOff;
    updatebarpos(awesomeconf->display, awesomeconf->statusbar);
    arrange(awesomeconf);
}


void
uicb_setstatustext(awesome_config *awesomeconf, const char *arg)
{
    if(!arg)
        return;
    a_strncpy(awesomeconf->statustext, sizeof(awesomeconf->statustext), arg, a_strlen(arg));

    drawstatusbar(awesomeconf);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
