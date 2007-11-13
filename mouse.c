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

void
uicb_movemouse(awesome_config *awesomeconf, const char *arg __attribute__ ((unused)))
{
    int x1, y1, ocx, ocy, di, nx, ny;
    unsigned int dui;
    Window dummy;
    XEvent ev;
    ScreenInfo *si;
    Client *c = get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel;

    if(!c)
        return;

    if((get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange != layout_floating)
        && !c->isfloating)
         uicb_togglefloating(awesomeconf, "DUMMY");
     else
         restack(awesomeconf);

    si = get_screen_info(c->display, c->screen, &awesomeconf->statusbar);

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(c->display, RootWindow(c->display, c->phys_screen), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, awesomeconf->cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(c->display, RootWindow(c->display, c->phys_screen), &dummy, &dummy, &x1, &y1, &di, &di, &dui);
    for(;;)
    {
        XMaskEvent(c->display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(c->display, CurrentTime);
            p_delete(&si);
            return;
        case ConfigureRequest:
            handle_event_configurerequest(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case Expose:
            handle_event_expose(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case MapRequest:
            handle_event_maprequest(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case MotionNotify:
            XSync(c->display, False);
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y1);
            if(abs(nx) < awesomeconf->snap + si[c->screen].x_org && nx > si[c->screen].x_org)
                nx = si[c->screen].x_org;
            else if(abs((si[c->screen].x_org + si[c->screen].width) - (nx + c->w + 2 * c->border)) < awesomeconf->snap)
                nx = si[c->screen].x_org + si[c->screen].width - c->w - 2 * c->border;
            if(abs(ny) < awesomeconf->snap + si[c->screen].y_org && ny > si[c->screen].y_org)
                ny = si[c->screen].y_org;
            else if(abs((si[c->screen].y_org + si[c->screen].height) - (ny + c->h + 2 * c->border)) < awesomeconf[c->screen].snap)
                ny = si[c->screen].y_org + si[c->screen].height - c->h - 2 * c->border;
            client_resize(c, nx, ny, c->w, c->h, awesomeconf, False, False);
            break;
        }
    }
}

void
uicb_resizemouse(awesome_config *awesomeconf, const char *arg __attribute__ ((unused)))
{
    int ocx, ocy, nw, nh;
    XEvent ev;
    Client *c = get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel;

    if(!c)
        return;

    if((get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange != layout_floating)
       && !c->isfloating)
        uicb_togglefloating(awesomeconf, "DUMMY");
    else
        restack(awesomeconf);

    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(c->display, RootWindow(c->display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, awesomeconf->cursor[CurResize], CurrentTime) != GrabSuccess)
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
            handle_event_configurerequest(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case Expose:
            handle_event_expose(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case MapRequest:
            handle_event_maprequest(&ev, &awesomeconf[0 - awesomeconf->screen]);
            break;
        case MotionNotify:
            XSync(c->display, False);
            if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                nw = 1;
            if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                nh = 1;
            client_resize(c, c->x, c->y, nw, nh, awesomeconf, True, False);
            break;
        }
    }
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
