/*
 * mouse.c - mouse managing
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

#include "mouse.h"
#include "screen.h"
#include "layout.h"
#include "tag.h"
#include "util.h"
#include "event.h"
#include "window.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

/** Move client with mouse
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_movemouse(int screen, char *arg __attribute__ ((unused)))
{
    int x1, y1, ocx, ocy, di, nx, ny;
    unsigned int dui;
    Window dummy;
    XEvent ev;
    Area area;
    Client *c = globalconf.focus->client;

    if(!c)
        return;

    if((get_current_layout(screen)->arrange != layout_floating)
        && !c->isfloating)
         uicb_client_togglefloating(screen, NULL);
     else
         restack(screen);

    area = get_screen_area(c->screen,
                           globalconf.screens[screen].statusbar,
                           &globalconf.screens[screen].padding);

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(c->display,
                    RootWindow(c->display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, globalconf.cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(c->display,
                  RootWindow(c->display, c->phys_screen),
                  &dummy, &dummy, &x1, &y1, &di, &di, &dui);
    for(;;)
    {
        XMaskEvent(c->display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(c->display, CurrentTime);
            return;
        case ConfigureRequest:
            handle_event_configurerequest(&ev);
            break;
        case Expose:
            handle_event_expose(&ev);
            break;
        case MapRequest:
            handle_event_maprequest(&ev);
            break;
        case MotionNotify:
            XSync(c->display, False);
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y1);
            if(abs(nx) < globalconf.screens[screen].snap + area.x && nx > area.x)
                nx = area.x;
            else if(abs((area.x + area.width) - (nx + c->w + 2 * c->border)) < globalconf.screens[screen].snap)
                nx = area.x + area.width - c->w - 2 * c->border;
            if(abs(ny) < globalconf.screens[screen].snap + area.y && ny > area.y)
                ny = area.y;
            else if(abs((area.y + area.height) - (ny + c->h + 2 * c->border)) < globalconf.screens[screen].snap)
                ny = area.y + area.height - c->h - 2 * c->border;
            client_resize(c, nx, ny, c->w, c->h, False, False);
            break;
        }
    }
}

/** Resize client with mouse
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_resizemouse(int screen, char *arg __attribute__ ((unused)))
{
    int ocx, ocy, nw, nh;
    XEvent ev;
    Client *c = globalconf.focus->client;

    if(!c)
        return;

    if((get_current_layout(screen)->arrange != layout_floating)
       && !c->isfloating)
        uicb_client_togglefloating(screen, NULL);
    else
        restack(screen);

    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(c->display, RootWindow(c->display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, globalconf.cursor[CurResize], CurrentTime) != GrabSuccess)
        return;
    c->ismax = False;
    XWarpPointer(c->display, None, c->win, 0, 0, 0, 0, c->w + c->border - 1, c->h + c->border - 1);
    for(;;)
    {
        XMaskEvent(c->display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XWarpPointer(c->display, None, c->win, 0, 0, 0, 0, c->w + c->border - 1, c->h + c->border - 1);
            XUngrabPointer(c->display, CurrentTime);
            while(XCheckMaskEvent(c->display, EnterWindowMask, &ev));
            return;
        case ConfigureRequest:
            handle_event_configurerequest(&ev);
            break;
        case Expose:
            handle_event_expose(&ev);
            break;
        case MapRequest:
            handle_event_maprequest(&ev);
            break;
        case MotionNotify:
            XSync(c->display, False);
            if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                nw = 1;
            if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                nh = 1;
            client_resize(c, c->x, c->y, nw, nh, True, False);
            break;
        }
    }
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
