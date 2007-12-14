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
#include <math.h>

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
isoccupied(Client *head, unsigned int t, int screen)
{
    Client *c;

    for(c = head; c; c = c->next)
        if(c->tags[t] && c->screen == screen)
            return True;
    return False;
}

void
drawstatusbar(awesome_config *awesomeconf, int screen)
{
    int z, i, x = 0, y = 0, w;
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;
    Drawable drawable;
    int phys_screen = get_phys_screen(awesomeconf->display, screen);

    /* don't waste our time */
    if(awesomeconf->screens[screen].statusbar.position == BarOff)
        return;

    drawable = XCreatePixmap(awesomeconf->display,
                             RootWindow(awesomeconf->display, phys_screen),
                             awesomeconf->screens[screen].statusbar.width,
                             awesomeconf->screens[screen].statusbar.height,
                             DefaultDepth(awesomeconf->display, phys_screen));

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
    {
        w = textwidth(awesomeconf->display, awesomeconf->screens[screen].font,
                      awesomeconf->screens[screen].tags[i].name);
        if(awesomeconf->screens[screen].tags[i].selected)
        {
            drawtext(awesomeconf->display, phys_screen,
                     x, y, w,
                     awesomeconf->screens[screen].statusbar.height,
                     drawable,
                     awesomeconf->screens[screen].statusbar.width,
                     awesomeconf->screens[screen].statusbar.height,
                     awesomeconf->screens[screen].font,
                     awesomeconf->screens[screen].tags[i].name, awesomeconf->screens[screen].colors_selected);
            if(isoccupied(awesomeconf->clients, i, screen))
                drawrectangle(awesomeconf->display, phys_screen,
                              x, y,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              drawable,
                              awesomeconf->screens[screen].statusbar.width,
                              awesomeconf->screens[screen].statusbar.height,
                              sel && sel->tags[i],
                              awesomeconf->screens[screen].colors_selected[ColFG]);
        }
        else
        {
            drawtext(awesomeconf->display, phys_screen,
                     x, y, w,
                     awesomeconf->screens[screen].statusbar.height,
                     drawable,
                     awesomeconf->screens[screen].statusbar.width,
                     awesomeconf->screens[screen].statusbar.height,
                     awesomeconf->screens[screen].font,
                     awesomeconf->screens[screen].tags[i].name, awesomeconf->screens[screen].colors_normal);
            if(isoccupied(awesomeconf->clients, i, screen))
                drawrectangle(awesomeconf->display, phys_screen,
                              x, y,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              drawable,
                              awesomeconf->screens[screen].statusbar.width,
                              awesomeconf->screens[screen].statusbar.height,
                              sel && sel->tags[i],
                              awesomeconf->screens[screen].colors_normal[ColFG]);
        }
        x += w;
    }
    drawtext(awesomeconf->display, phys_screen,
             x, y, awesomeconf->screens[screen].statusbar.txtlayoutwidth,
             awesomeconf->screens[screen].statusbar.height,
             drawable,
             awesomeconf->screens[screen].statusbar.width,
             awesomeconf->screens[screen].statusbar.height,
             awesomeconf->screens[screen].font,
             get_current_layout(awesomeconf->screens[screen])->symbol,
             awesomeconf->screens[screen].colors_normal);
    z = x + awesomeconf->screens[screen].statusbar.txtlayoutwidth;
    w = textwidth(awesomeconf->display, awesomeconf->screens[screen].font, awesomeconf->screens[screen].statustext);
    x = awesomeconf->screens[screen].statusbar.width - w;
    if(x < z)
    {
        x = z;
        w = awesomeconf->screens[screen].statusbar.width - z;
    }
    drawtext(awesomeconf->display, phys_screen,
             x, y, w,
             awesomeconf->screens[screen].statusbar.height,
             drawable,
             awesomeconf->screens[screen].statusbar.width,
             awesomeconf->screens[screen].statusbar.height,
             awesomeconf->screens[screen].font,
             awesomeconf->screens[screen].statustext, awesomeconf->screens[screen].colors_normal);
    if((w = x - z) > awesomeconf->screens[screen].statusbar.height)
    {
        x = z;
        if(sel && sel->screen == screen)
        {
            drawtext(awesomeconf->display, phys_screen,
                     x, y, w,
                     awesomeconf->screens[screen].statusbar.height,
                     drawable,
                     awesomeconf->screens[screen].statusbar.width,
                     awesomeconf->screens[screen].statusbar.height,
                     awesomeconf->screens[screen].font,
                     sel->name, awesomeconf->screens[screen].colors_selected);
            if(sel->isfloating)
                drawcircle(awesomeconf->display, phys_screen,
                           x, y,
                           (awesomeconf->screens[screen].font->height + 2) / 4,
                           drawable,
                           awesomeconf->screens[screen].statusbar.width,
                           awesomeconf->screens[screen].statusbar.height,
                           sel->ismax,
                           awesomeconf->screens[screen].colors_selected[ColFG]);
        }
        else
            drawtext(awesomeconf->display, phys_screen,
                     x, y, w,
                     awesomeconf->screens[screen].statusbar.height,
                     drawable,
                     awesomeconf->screens[screen].statusbar.width,
                     awesomeconf->screens[screen].statusbar.height,
                     awesomeconf->screens[screen].font,
                     NULL, awesomeconf->screens[screen].colors_normal);
    }
    if(awesomeconf->screens[screen].statusbar.position == BarRight
       || awesomeconf->screens[screen].statusbar.position == BarLeft)
    {
        Drawable d;
        if(awesomeconf->screens[screen].statusbar.position == BarRight)
            d = draw_rotate(awesomeconf->display, phys_screen, drawable,
                            awesomeconf->screens[screen].statusbar.width, awesomeconf->screens[screen].statusbar.height,
                            M_PI_2, awesomeconf->screens[screen].statusbar.height, 0);
        else
            d = draw_rotate(awesomeconf->display, phys_screen, drawable,
                            awesomeconf->screens[screen].statusbar.width, awesomeconf->screens[screen].statusbar.height,
                            - M_PI_2, 0, awesomeconf->screens[screen].statusbar.width);
        XCopyArea(awesomeconf->display, d,
                  awesomeconf->screens[screen].statusbar.window,
                  DefaultGC(awesomeconf->display, phys_screen), 0, 0,
                  awesomeconf->screens[screen].statusbar.height, awesomeconf->screens[screen].statusbar.width, 0, 0);
        XFreePixmap(awesomeconf->display, d);
    }
    else
        XCopyArea(awesomeconf->display, drawable,
                  awesomeconf->screens[screen].statusbar.window,
                  DefaultGC(awesomeconf->display, phys_screen), 0, 0,
                  awesomeconf->screens[screen].statusbar.width, awesomeconf->screens[screen].statusbar.height, 0, 0);
    XFreePixmap(awesomeconf->display, drawable);
    XSync(awesomeconf->display, False);
}

void
initstatusbar(Display *disp, int screen, Statusbar *statusbar, Cursor cursor, XftFont *font, Layout *layouts, int nlayouts, Padding *padding)
{
    XSetWindowAttributes wa;
    int i, phys_screen = get_phys_screen(disp, screen);
    ScreenInfo *si = get_screen_info(disp, screen, NULL, padding);

    statusbar->height = font->height * 1.5;

    if(statusbar->position == BarRight || statusbar->position == BarLeft)
        statusbar->width = si[screen].height;
    else
        statusbar->width = si[screen].width;

    p_delete(&si);

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = cursor;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    if(statusbar->dposition == BarRight || statusbar->dposition == BarLeft)
        statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0,
                                          statusbar->height,
                                          statusbar->width,
                                          0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                          DefaultVisual(disp, phys_screen),
                                          CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    else
        statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0,
                                          statusbar->width,
                                          statusbar->height,
                                          0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                          DefaultVisual(disp, phys_screen),
                                          CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, statusbar->window, cursor);
    updatebarpos(disp, *statusbar, padding);
    XMapRaised(disp, statusbar->window);

    for(i = 0; i < nlayouts; i++)
        statusbar->txtlayoutwidth = MAX(statusbar->txtlayoutwidth,
                                        (textwidth(disp, font, layouts[i].symbol)));
}

void
updatebarpos(Display *disp, Statusbar statusbar, Padding *padding)
{
    XEvent ev;
    ScreenInfo *si = get_screen_info(disp, statusbar.screen, NULL, padding);

    XMapRaised(disp, statusbar.window);
    switch (statusbar.position)
    {
      default:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].y_org);
        break;
      case BarRight:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org + (si[statusbar.screen].width - statusbar.height), si[statusbar.screen].y_org);
        break;
      case BarBot:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].height - statusbar.height);
        break;
      case BarOff:
        XUnmapWindow(disp, statusbar.window);
        break;
    }
    p_delete(&si);
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

void
uicb_togglebar(awesome_config *awesomeconf,
               int screen,
               const char *arg __attribute__ ((unused)))
{
    if(awesomeconf->screens[screen].statusbar.position == BarOff)
        awesomeconf->screens[screen].statusbar.position = (awesomeconf->screens[screen].statusbar.dposition == BarOff) ? BarTop : awesomeconf->screens[screen].statusbar.dposition;
    else
        awesomeconf->screens[screen].statusbar.position = BarOff;
    updatebarpos(awesomeconf->display, awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);
    arrange(awesomeconf, screen);
}


void
uicb_setstatustext(awesome_config *awesomeconf, int screen, const char *arg)
{
    if(!arg)
        return;
    a_strncpy(awesomeconf->screens[screen].statustext,
              sizeof(awesomeconf->screens[screen].statustext), arg, a_strlen(arg));

    drawstatusbar(awesomeconf, screen);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
