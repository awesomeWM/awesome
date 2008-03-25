/*
 * mouse.c - mouse managing
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include "event.h"
#include "window.h"
#include "client.h"
#include "layouts/floating.h"
#include "layouts/tile.h"
#include "common/xscreen.h"

#define MOUSEMASK      (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)

extern AwesomeConf globalconf;

static area_t
mouse_snapclient(Client *c, area_t geometry)
{
    int snap = globalconf.screens[c->screen].snap;
    area_t screen_geometry =
        screen_get_area(c->screen,
                        globalconf.screens[c->screen].statusbar,
                        &globalconf.screens[c->screen].padding);

    if(abs(geometry.x) < snap + screen_geometry.x && geometry.x > screen_geometry.x)
        geometry.x = screen_geometry.x;
    else if(abs((screen_geometry.x + screen_geometry.width) - (geometry.x + c->geometry.width + 2 * c->border))
            < snap)
        geometry.x = screen_geometry.x + screen_geometry.width - c->geometry.width - 2 * c->border;
    if(abs(geometry.y) < snap + screen_geometry.y && geometry.y > screen_geometry.y)
        geometry.y = screen_geometry.y;
    else if(abs((screen_geometry.y + screen_geometry.height) - (geometry.y + c->geometry.height + 2 * c->border))
            < snap)
        geometry.y = screen_geometry.y + screen_geometry.height - c->geometry.height - 2 * c->border;

    return geometry;
}

/** Move client with mouse
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_movemouse(int screen, char *arg __attribute__ ((unused)))
{
    int x, y, ocx, ocy, di, newscreen;
    unsigned int dui;
    Window dummy, child;
    XEvent ev;
    area_t geometry;
    Client *c = globalconf.focus->client, *target;
    Layout *layout = layout_get_current(screen);

    if(!c)
        return;

    geometry = c->geometry;
    ocx = geometry.x;
    ocy = geometry.y;

    if(XGrabPointer(globalconf.display,
                    RootWindow(globalconf.display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    RootWindow(globalconf.display, c->phys_screen),
                    globalconf.cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(globalconf.display,
                  RootWindow(globalconf.display, c->phys_screen),
                  &dummy, &dummy, &x, &y, &di, &di, &dui);
    c->ismax = False;
    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
          case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
            return;
          case ConfigureRequest:
            event_handle_configurerequest(&ev);
            break;
          case Expose:
            event_handle_expose(&ev);
            break;
          case MapRequest:
            event_handle_maprequest(&ev);
            break;
          case EnterNotify:
            event_handle_enternotify(&ev);
            break;
          case MotionNotify:
            if(c->isfloating || layout->arrange == layout_floating)
            {
                geometry.x = ocx + (ev.xmotion.x - x);
                geometry.y = ocy + (ev.xmotion.y - y);

                geometry = mouse_snapclient(c, geometry);

                c->ismoving = True;
                client_resize(c, geometry, False);
                c->ismoving = False;
            }
            else
            {
                XQueryPointer(globalconf.display,
                              RootWindow(globalconf.display, c->phys_screen),
                              &dummy, &child, &x, &y, &di, &di, &dui);
                if((newscreen = screen_get_bycoord(globalconf.screens_info, c->screen, x, y)) != c->screen)
                {
                    move_client_to_screen(c, newscreen, True);
                    globalconf.screens[c->screen].need_arrange = True;
                    if(layout_get_current(newscreen)->arrange != layout_floating)
                        globalconf.screens[newscreen].need_arrange = True;
                    layout_refresh();
                }
                if((target = client_get_bywin(globalconf.clients, child))
                   && target != c && !target->isfloating)
                {
                    client_list_swap(&globalconf.clients, c, target);
                    globalconf.screens[c->screen].need_arrange = True;
                    layout_refresh();
                }
            }
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
    int ocx = 0, ocy = 0, n;
    XEvent ev;
    Client *c = globalconf.focus->client;
    Tag **curtags = tags_get_current(screen);
    Layout *layout = curtags[0]->layout;
    area_t area = { 0, 0, 0, 0, NULL, NULL }, geometry;
    double mwfact;

    /* only handle floating and tiled layouts */
    if(c && !c->isfixed)
    {
        if(layout->arrange == layout_floating || c->isfloating)
        {
            ocx = c->geometry.x;
            ocy = c->geometry.y;
            c->ismax = False;
        }
        else if (layout->arrange == layout_tile || layout->arrange == layout_tileleft
                 || layout->arrange == layout_tilebottom || layout->arrange == layout_tiletop)
        {
            for(n = 0, c = globalconf.clients; c; c = c->next)
                if(IS_TILED(c, screen))
                    n++;

            if(n <= curtags[0]->nmaster) return;

            for(c = globalconf.clients; c && !IS_TILED(c, screen); c = c->next);
            if(!c) return;

            area = screen_get_area(screen,
                                   globalconf.screens[c->screen].statusbar,
                                   &globalconf.screens[c->screen].padding);
        }
    }
    else
        return;

    if(XGrabPointer(globalconf.display,
                    RootWindow(globalconf.display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    RootWindow(globalconf.display, c->phys_screen),
                    globalconf.cursor[CurResize], CurrentTime) != GrabSuccess)
        return;

    if(curtags[0]->layout->arrange == layout_tileleft)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, 0, c->geometry.height + c->border - 1);
    else if(curtags[0]->layout->arrange == layout_tilebottom)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, c->geometry.width + c->border - 1, c->geometry.height + c->border - 1);
    else if(curtags[0]->layout->arrange == layout_tiletop)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, c->geometry.width + c->border - 1, 0);
    else
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, c->geometry.width + c->border - 1, c->geometry.height + c->border - 1);

    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
          case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
            return;
          case ConfigureRequest:
            event_handle_configurerequest(&ev);
            break;
          case Expose:
            event_handle_expose(&ev);
            break;
          case MapRequest:
            event_handle_maprequest(&ev);
            break;
          case MotionNotify:
            if(layout->arrange == layout_floating || c->isfloating)
            {
                if((geometry.width = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                    geometry.width = 1;
                if((geometry.height = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                    geometry.height = 1;
                geometry.x = c->geometry.x;
                geometry.y = c->geometry.y;
                client_resize(c, geometry, True);
            }
            else if(layout->arrange == layout_tile || layout->arrange == layout_tileleft
                    || layout->arrange == layout_tiletop || layout->arrange == layout_tilebottom)
            {
                if(layout->arrange == layout_tile)
                    mwfact = (double) (ev.xmotion.x - area.x) / area.width;
                else if(curtags[0]->layout->arrange == layout_tileleft)
                    mwfact = 1 - (double) (ev.xmotion.x - area.x) / area.width;
                else if(curtags[0]->layout->arrange == layout_tilebottom)
                    mwfact = (double) (ev.xmotion.y - area.y) / area.height;
                else
                    mwfact = 1 - (double) (ev.xmotion.y - area.y) / area.height;
                if(mwfact < 0.1) mwfact = 0.1;
                else if(mwfact > 0.9) mwfact = 0.9;
                if(fabs(curtags[0]->mwfact - mwfact) >= 0.01)
                {
                    curtags[0]->mwfact = mwfact;
                    globalconf.screens[screen].need_arrange = True;
                    layout_refresh();
                    while(XCheckMaskEvent(globalconf.display, PointerMotionMask, &ev));
                }
            }

            break;
        }
    }
    p_delete(&curtags);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
