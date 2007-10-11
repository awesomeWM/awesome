/*
 * event.c - event handlers
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

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include "screen.h"
#include "event.h"
#include "layout.h"
#include "tag.h"
#include "draw.h"
#include "statusbar.h"
#include "util.h"
#include "layouts/tile.h"
#include "layouts/floating.h"

/* extern */
extern Client *sel;

#define CLEANMASK(mask, screen)		(mask & ~(awesomeconf[screen].numlockmask | LockMask))
#define MOUSEMASK	                (BUTTONMASK | PointerMotionMask)

static Client *
getclient(Client **list, Window w)
{
    Client *c;

    for(c = *list; c && c->win != w; c = c->next);
    return c;
}

static void
movemouse(Client * c, awesome_config *awesomeconf)
{
    int x1, y1, ocx, ocy, di, nx, ny;
    unsigned int dui;
    Window dummy;
    XEvent ev;
    ScreenInfo *si;

    si = get_screen_info(c->display, c->screen, &awesomeconf[c->screen].statusbar);

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(c->display, RootWindow(c->display, c->phys_screen), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, awesomeconf[c->screen].cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(c->display, RootWindow(c->display, c->phys_screen), &dummy, &dummy, &x1, &y1, &di, &di, &dui);
    for(;;)
    {
        XMaskEvent(c->display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(c->display, CurrentTime);
            return;
        case ConfigureRequest:
            handle_event_configurerequest(&ev,awesomeconf);
            break;
        case Expose:
            handle_event_expose(&ev, awesomeconf);
            break;
        case MapRequest:
            handle_event_maprequest(&ev, awesomeconf);
            break;
        case MotionNotify:
            XSync(c->display, False);
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y1);
            if(abs(nx) < awesomeconf[c->screen].snap + si[c->screen].x_org && nx > si[c->screen].x_org)
                nx = si[c->screen].x_org;
            else if(abs((si[c->screen].x_org + si[c->screen].width) - (nx + c->w + 2 * c->border)) < awesomeconf[c->screen].snap)
                nx = si[c->screen].x_org + si[c->screen].width - c->w - 2 * c->border;
            if(abs(ny) < awesomeconf[c->screen].snap + si[c->screen].y_org && ny > si[c->screen].y_org)
                ny = si[c->screen].y_org;
            else if(abs((si[c->screen].y_org + si[c->screen].height) - (ny + c->h + 2 * c->border)) < awesomeconf[c->screen].snap)
                ny = si[c->screen].y_org + si[c->screen].height - c->h - 2 * c->border;
            resize(c, nx, ny, c->w, c->h, &awesomeconf[c->screen], False);
            break;
        }
    }
    XFree(si);
}

static void
resizemouse(Client * c, awesome_config *awesomeconf)
{
    int ocx, ocy, nw, nh;
    XEvent ev;

    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(c->display, RootWindow(c->display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, awesomeconf[c->screen].cursor[CurResize], CurrentTime) != GrabSuccess)
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
            handle_event_configurerequest(&ev,awesomeconf);
            break;
        case Expose:
            handle_event_expose(&ev, awesomeconf);
            break;
        case MapRequest:
            handle_event_maprequest(&ev, awesomeconf);
            break;
        case MotionNotify:
            XSync(c->display, False);
            if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                nw = 1;
            if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                nh = 1;
            resize(c, c->x, c->y, nw, nh, &awesomeconf[c->screen], True);
            break;
        }
    }
}

void
handle_event_buttonpress(XEvent * e, awesome_config *awesomeconf)
{
    int i, screen, x = 0, y = 0;
    unsigned int udummy;
    Client *c;
    Window wdummy;
    XButtonPressedEvent *ev = &e->xbutton;

    for(screen = 0; screen < get_screen_count(e->xany.display); screen++)
        if(awesomeconf[screen].statusbar.window == ev->window)
        {
            for(i = 0; i < awesomeconf[screen].ntags; i++)
            {
                x += textwidth(e->xany.display, awesomeconf[screen].font, awesomeconf[screen].tags[i].name, a_strlen(awesomeconf[screen].tags[i].name)) + awesomeconf[screen].font->height;
                if(ev->x < x)
                {
                    if(ev->button == Button1)
                    {
                        if(ev->state & awesomeconf[screen].modkey)
                            uicb_tag(e->xany.display, &awesomeconf[screen], awesomeconf[screen].tags[i].name);
                        else
                            uicb_view(e->xany.display, &awesomeconf[screen], awesomeconf[screen].tags[i].name);
                    }
                    else if(ev->button == Button3)
                    {
                        if(ev->state & awesomeconf[screen].modkey)
                            uicb_toggletag(e->xany.display, &awesomeconf[screen], awesomeconf[screen].tags[i].name);
                        else
                            uicb_toggleview(e->xany.display, &awesomeconf[screen], awesomeconf[screen].tags[i].name);
                    }
                    else if(ev->button == Button4)
                        uicb_tag_viewnext(e->xany.display, &awesomeconf[screen], NULL);
                    else if(ev->button == Button5)
                        uicb_tag_viewprev(e->xany.display, &awesomeconf[screen], NULL);
                    return;
                }
            }
            x += awesomeconf[screen].statusbar.width;
            if(ev->x < x && (ev->button == Button1 || ev->button == Button4))
                uicb_setlayout(e->xany.display, &awesomeconf[screen], "+1");
            else if(ev->x < x && (ev->button == Button3 || ev->button == Button5))
                uicb_setlayout(e->xany.display, &awesomeconf[screen], "-1");
            else if(ev->button == Button4)
                uicb_focusnext(e->xany.display, &awesomeconf[screen], NULL);
            else if(ev->button == Button5)
                uicb_focusprev(e->xany.display, &awesomeconf[screen], NULL);
            return;
        }

    if((c = getclient(awesomeconf->clients, ev->window)))
    {
        focus(c->display, c, ev->same_screen, &awesomeconf[c->screen]);
        if(CLEANMASK(ev->state, c->screen) != awesomeconf[c->screen].modkey)
        {
           if (ev->button == Button1)
           {
               restack(c->display, &awesomeconf[c->screen]);
               grabbuttons(c, True, True, awesomeconf->modkey, awesomeconf->numlockmask);
           }
        }
        else if(ev->button == Button1)
        {
            if(!IS_ARRANGE(c->screen, layout_floating) && !c->isfloating)
                uicb_togglefloating(e->xany.display, &awesomeconf[c->screen], NULL);
            else
                restack(e->xany.display, &awesomeconf[c->screen]);
            movemouse(c, awesomeconf);
        }
        else if(ev->button == Button2)
        {
            if(!IS_ARRANGE(c->screen, layout_floating) && !c->isfixed && c->isfloating)
                uicb_togglefloating(e->xany.display, &awesomeconf[c->screen], NULL);
            else
                uicb_zoom(e->xany.display, &awesomeconf[c->screen], NULL);
        }
        else if(ev->button == Button3)
        {
            if(!IS_ARRANGE(c->screen, layout_floating) && !c->isfloating)
                uicb_togglefloating(e->xany.display, &awesomeconf[c->screen], NULL);
            else
                restack(e->xany.display, &awesomeconf[c->screen]);
            resizemouse(c, awesomeconf);
        }
        else if(ev->button == Button4)
            uicb_settrans(e->xany.display, &awesomeconf[c->screen], "+5");
        else if(ev->button == Button5)
            uicb_settrans(e->xany.display, &awesomeconf[c->screen], "-5");
        }
    else if(!sel)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(RootWindow(e->xany.display, screen) == ev->window
               && XQueryPointer(e->xany.display, ev->window, &wdummy, &wdummy, &x, &y, &i, &i, &udummy))
            {
                screen = get_screen_bycoord(e->xany.display, x, y);
                if(ev->button == Button4)
                    uicb_tag_viewnext(e->xany.display, &awesomeconf[screen], NULL);
                else if(ev->button == Button5)
                    uicb_tag_viewprev(e->xany.display, &awesomeconf[screen], NULL);
                break;
            }
}

void
handle_event_configurerequest(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if((c = getclient(awesomeconf->clients, ev->window)))
    {
        c->ismax = False;
        if(ev->value_mask & CWBorderWidth)
            c->border = ev->border_width;
        if(c->isfixed || c->isfloating || IS_ARRANGE(c->screen, layout_floating))
        {
            if(ev->value_mask & CWX)
                c->x = ev->x;
            if(ev->value_mask & CWY)
                c->y = ev->y;
            if(ev->value_mask & CWWidth)
                c->w = ev->width;
            if(ev->value_mask & CWHeight)
                c->h = ev->height;
            if((c->x + c->w) > DisplayWidth(c->display, c->phys_screen) && c->isfloating)
                c->x = DisplayWidth(c->display, c->phys_screen) / 2 - c->w / 2;       /* center in x direction */
            if((c->y + c->h) > DisplayHeight(c->display, c->phys_screen) && c->isfloating)
                c->y = DisplayHeight(c->display, c->phys_screen) / 2 - c->h / 2;       /* center in y direction */
            if((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                configure(c);
            if(isvisible(c, c->screen, awesomeconf[c->screen].tags, awesomeconf[c->screen].ntags))
                XMoveResizeWindow(e->xany.display, c->win, c->x, c->y, c->w, c->h);
        }
        else
            configure(c);
    }
    else
    {
        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(e->xany.display, ev->window, ev->value_mask, &wc);
    }
    XSync(e->xany.display, False);
}

void
handle_event_configurenotify(XEvent * e, awesome_config *awesomeconf)
{
    XConfigureEvent *ev = &e->xconfigure;
    int screen;
    ScreenInfo *si;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if(ev->window == RootWindow(e->xany.display, screen)
           && (ev->width != DisplayWidth(e->xany.display, screen)
               || ev->height != DisplayHeight(e->xany.display, screen)))
        {
            DisplayWidth(e->xany.display, screen) = ev->width;
            DisplayHeight(e->xany.display, screen) = ev->height;

            si = get_screen_info(e->xany.display, screen, NULL);
            XFreePixmap(e->xany.display, awesomeconf[screen].statusbar.drawable);
            awesomeconf[screen].statusbar.drawable = XCreatePixmap(e->xany.display, RootWindow(e->xany.display, screen),
                                                                   si[screen].width,
                                                                   awesomeconf[screen].statusbar.height,
                                                                   DefaultDepth(e->xany.display, screen));
            XResizeWindow(e->xany.display, awesomeconf[screen].statusbar.window,
                          si[screen].width, awesomeconf[screen].statusbar.height);
            XFree(si);
            updatebarpos(e->xany.display, awesomeconf[screen].statusbar);
            arrange(e->xany.display, &awesomeconf[screen]);
        }
}

void
handle_event_destroynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = getclient(awesomeconf->clients, ev->window)))
        unmanage(c, WithdrawnState, &awesomeconf[c->screen]);
}

void
handle_event_enternotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = getclient(awesomeconf->clients, ev->window)))
    {
        if(!sel || sel != c)
        {
            focus(c->display, c, ev->same_screen, &awesomeconf[c->screen]);
            if (sel && (sel->isfloating || IS_ARRANGE(sel->screen, layout_floating)))
                grabbuttons(sel, True, False, awesomeconf->modkey, awesomeconf->numlockmask);
        }
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(ev->window == RootWindow(e->xany.display, screen))
                focus(e->xany.display, NULL, True, &awesomeconf[screen]);
}

void
handle_event_expose(XEvent * e, awesome_config *awesomeconf)
{
    XExposeEvent *ev = &e->xexpose;
    int screen;

    if(!ev->count)
        for(screen = 0; screen < get_screen_count(e->xany.display); screen++)
            if(awesomeconf[screen].statusbar.window == ev->window)
                drawstatusbar(e->xany.display, &awesomeconf[screen]);
}

void
handle_event_keypress(XEvent * e, awesome_config *awesomeconf)
{
    int i, screen, x, y, d;
    unsigned int m;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;
    Window dummy;

    keysym = XKeycodeToKeysym(e->xany.display, (KeyCode) ev->keycode, 0);

    /* find the right screen for this event */
    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if(XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen), &dummy, &dummy, &x, &y, &d, &d, &m))
        {
            /* if screen is 0, we are on first Zaphod screen or on the
             * only screen in Xinerama, so we can ask for a better screen
             * number with get_screen_bycoord: we'll get 0 in Zaphod mode
             * so it's the same, or maybe the real Xinerama screen */
            if(screen == 0)
                screen = get_screen_bycoord(e->xany.display, x, y);
            break;
        }

    for(i = 0; i < awesomeconf[screen].nkeys; i++)
        if(keysym == awesomeconf[screen].keys[i].keysym
           && CLEANMASK(awesomeconf[screen].keys[i].mod, screen) == CLEANMASK(ev->state, screen) && awesomeconf[screen].keys[i].func)
            awesomeconf[screen].keys[i].func(e->xany.display, &awesomeconf[screen], awesomeconf[screen].keys[i].arg);
}

void
handle_event_leavenotify(XEvent * e, awesome_config *awesomeconf)
{
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if((ev->window == RootWindow(e->xany.display, screen)) && !ev->same_screen)
            focus(e->xany.display, NULL, ev->same_screen, &awesomeconf[screen]);
}

void
handle_event_mappingnotify(XEvent * e, awesome_config *awesomeconf)
{
    XMappingEvent *ev = &e->xmapping;
    int screen;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            grabkeys(e->xany.display, screen, &awesomeconf[screen]);
}

void
handle_event_maprequest(XEvent * e, awesome_config *awesomeconf)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    int screen;

    if(!XGetWindowAttributes(e->xany.display, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;
    if(!getclient(awesomeconf->clients, ev->window))
    {
        for(screen = 0; wa.screen != ScreenOfDisplay(e->xany.display, screen); screen++);
        if(screen == 0)
            screen = get_screen_bycoord(e->xany.display, wa.x, wa.y);
        manage(e->xany.display, ev->window, &wa, &awesomeconf[screen]);
    }
}

void
handle_event_propertynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete)
        return;                 /* ignore */
    if((c = getclient(awesomeconf->clients, ev->window)))
    {
        switch (ev->atom)
        {
        case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (getclient(awesomeconf->clients, trans) != NULL)))
                arrange(e->xany.display, &awesomeconf[c->screen]);
            break;
        case XA_WM_NORMAL_HINTS:
            updatesizehints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(c->display, "_NET_WM_NAME", False))
        {
            updatetitle(c);
            if(c == sel)
                drawstatusbar(e->xany.display, &awesomeconf[c->screen]);
        }
    }
}

void
handle_event_unmapnotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = getclient(awesomeconf->clients, ev->window))
       && ev->event == RootWindow(e->xany.display, c->phys_screen) && (ev->send_event || !c->unmapped))
        unmanage(c, WithdrawnState, &awesomeconf[c->screen]);
}

void
handle_event_shape(XEvent * e,
                   awesome_config *awesomeconf __attribute__ ((unused)))
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = getclient(awesomeconf->clients, ev->window);

    if(c)
        set_shape(c);
}

void
handle_event_randr_screen_change_notify(XEvent *e,
                                        awesome_config *awesomeconf __attribute__ ((unused)))
{
    XRRUpdateConfiguration(e);
}

void
grabkeys(Display *disp, int screen, awesome_config *awesomeconf)
{
    int i;
    KeyCode code;

    XUngrabKey(disp, AnyKey, AnyModifier, RootWindow(disp, screen));
    for(i = 0; i < awesomeconf->nkeys; i++)
    {
        if((code = XKeysymToKeycode(disp, awesomeconf->keys[i].keysym)) == NoSymbol)
            continue;
        XGrabKey(disp, code, awesomeconf->keys[i].mod, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | LockMask, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask | LockMask, RootWindow(disp, screen), True,
                 GrabModeAsync, GrabModeAsync);
    }
}
