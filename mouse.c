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

#include <math.h>

#include "mouse.h"
#include "screen.h"
#include "layout.h"
#include "tag.h"
#include "util.h"
#include "event.h"
#include "window.h"
#include "statusbar.h"
#include "layouts/floating.h"
#include "layouts/tile.h"

extern AwesomeConf globalconf;

/** Move client with mouse
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_movemouse(int screen, char *arg __attribute__ ((unused)))
{
    int x1, y, ocx, ocy, di, nx, ny;
    unsigned int dui;
    Window dummy;
    XEvent ev;
    Area area;
    Client *c = globalconf.focus->client;
    Tag **curtags = get_current_tags(screen);

    if(!c)
        return;

    if((curtags[0]->layout->arrange != layout_floating)
        && !c->isfloating)
        uicb_client_togglefloating(screen, NULL);
    else
        restack(screen);

    p_delete(&curtags);

    area = get_screen_area(c->screen,
                           globalconf.screens[screen].statusbar,
                           &globalconf.screens[screen].padding);

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(globalconf.display,
                    RootWindow(globalconf.display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, globalconf.cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(globalconf.display,
                  RootWindow(globalconf.display, c->phys_screen),
                  &dummy, &dummy, &x1, &y, &di, &di, &dui);
    c->ismax = False;
    statusbar_draw(c->screen);
    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
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
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y);
            if(abs(nx) < globalconf.screens[screen].snap + area.x && nx > area.x)
                nx = area.x;
            else if(abs((area.x + area.width) - (nx + c->w + 2 * c->border)) < globalconf.screens[screen].snap)
                nx = area.x + area.width - c->w - 2 * c->border;
            if(abs(ny) < globalconf.screens[screen].snap + area.y && ny > area.y)
                ny = area.y;
            else if(abs((area.y + area.height) - (ny + c->h + 2 * c->border)) < globalconf.screens[screen].snap)
                ny = area.y + area.height - c->h - 2 * c->border;
            client_resize(c, nx, ny, c->w, c->h, False, False);
            while(XCheckMaskEvent(globalconf.display, PointerMotionMask, &ev));
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
    int ocx, ocy, nw, nh, n;
    XEvent ev;
    Client *c = globalconf.focus->client;
    Tag **curtags = get_current_tags(screen);
    Area area;
    double mwfact;

    /* only handle floating and tiled layouts */
    if(c)
    {
        if((curtags[0]->layout->arrange == layout_floating) || c->isfloating)
        {
            restack(screen);
            ocx = c->x;
            ocy = c->y;
            c->ismax = False;
        }
        else if (curtags[0]->layout->arrange == layout_tile
                 || curtags[0]->layout->arrange == layout_tileleft)
        {
            for(n = 0, c = globalconf.clients; c; c = c->next)
                if(IS_TILED(c, screen))
                    n++;

            if(n <= curtags[0]->nmaster) return;

            for(c = globalconf.clients; c && !IS_TILED(c, screen); c = c->next);
            if(!c) return;

            area = get_screen_area(screen,
                                   globalconf.screens[c->screen].statusbar,
                                   &globalconf.screens[c->screen].padding);
        }
    }
    else
        return;

    if(XGrabPointer(globalconf.display, RootWindow(globalconf.display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, globalconf.cursor[CurResize], CurrentTime) != GrabSuccess)
        return;

    if(curtags[0]->layout->arrange == layout_tileleft)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, 0, c->h + c->border - 1);
    else
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, c->w + c->border - 1, c->h + c->border - 1);

    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
            while(XCheckMaskEvent(globalconf.display, EnterWindowMask, &ev));
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
            if(curtags[0]->layout->arrange == layout_floating || c->isfloating)
            {
                if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                    nw = 1;
                if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                    nh = 1;
                client_resize(c, c->x, c->y, nw, nh, True, False);
            }
            else if(curtags[0]->layout->arrange == layout_tile
                    || curtags[0]->layout->arrange == layout_tileleft)
            {
                if(curtags[0]->layout->arrange == layout_tile)
                    mwfact = (double) (ev.xmotion.x - area.x) / area.width;
                else
                    mwfact = 1 - (double) (ev.xmotion.x - area.x) / area.width;
                if(mwfact < 0.1) mwfact = 0.1;
                else if(mwfact > 0.9) mwfact = 0.9;
                if(fabs(curtags[0]->mwfact - mwfact) >= 0.05)
                {
                    curtags[0]->mwfact = mwfact;
                    arrange(screen);
                    /* drop possibly arrived events while we where arrange()ing */
                    while(XCheckMaskEvent(globalconf.display, PointerMotionMask, &ev));
                }
            }

            break;
        }
    }
    p_delete(&curtags);
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
