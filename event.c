/*
 * event.c - event handlers
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

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include "screen.h"
#include "event.h"
#include "tag.h"
#include "statusbar.h"
#include "util.h"
#include "window.h"
#include "mouse.h"
#include "ewmh.h"
#include "client.h"
#include "layouts/tile.h"
#include "layouts/floating.h"


extern AwesomeConf globalconf;

static void
handle_mouse_button_press(int screen, unsigned int button, unsigned int state,
                          Button *buttons, char *arg)
{
    Button *b;

    for(b = buttons; b; b = b->next)
        if(button == b->button && CLEANMASK(state) == b->mod && b->func)
        {
            if(arg)
                b->func(screen, arg);
            else
                b->func(screen, b->arg);
            return;
        }
}

void
handle_event_buttonpress(XEvent *e)
{
    int i, screen, x = 0, y = 0;
    unsigned int udummy;
    Client *c;
    Window wdummy;
    Widget *widget;
    Statusbar *statusbar;
    XButtonPressedEvent *ev = &e->xbutton;

    for(screen = 0; screen < get_screen_count(); screen++)
        for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
            if(statusbar->window == ev->window)
            {
                if(statusbar->position == BarTop
                     || globalconf.screens[screen].statusbar->position == BarBot)
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->x >= widget->location && ev->x <= widget->location + widget->width)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }

                if(statusbar->position == BarRight)
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->y >= widget->location && ev->y <= widget->location + widget->width)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }
            
                if(statusbar->position == BarLeft)
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(statusbar->width - ev->y >= widget->location
                           && statusbar->width - ev->y <= widget->location + widget->width)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }
            }

    if((c = get_client_bywin(globalconf.clients, ev->window)))
    {
        focus(c, ev->same_screen, c->screen);
        if(CLEANMASK(ev->state) == NoSymbol
           && ev->button == Button1)
        {
            restack(c->screen);
            XAllowEvents(globalconf.display, ReplayPointer, CurrentTime);
            window_grabbuttons(get_phys_screen(c->screen), c->win, True, True);
        }
        else
            handle_mouse_button_press(c->screen, ev->button, ev->state, globalconf.buttons.client, NULL);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(RootWindow(e->xany.display, screen) == ev->window
               && XQueryPointer(e->xany.display,
                                ev->window, &wdummy,
                                &wdummy, &x, &y, &i,
                                &i, &udummy))
            {
                screen = get_screen_bycoord(x, y);
                handle_mouse_button_press(screen, ev->button, ev->state,
                                          globalconf.buttons.root, NULL);
                return;
            }
}

void
handle_event_configurerequest(XEvent * e)
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    int old_screen;

    if((c = get_client_bywin(globalconf.clients, ev->window)))
    {
        if(c->isfixed)
        {
            if(ev->value_mask & CWX)
                c->rx = c->x = ev->x - c->border;
            if(ev->value_mask & CWY)
                c->ry = c->y = ev->y - c->border;
            if(ev->value_mask & CWWidth)
                c->rw = c->w = ev->width;
            if(ev->value_mask & CWHeight)
                c->rh = c->h = ev->height;
            if((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                window_configure(c->win, c->x, c->y, c->w, c->h, c->border);
            /* recompute screen */
            old_screen = c->screen;
            c->screen = get_screen_bycoord(c->x, c->y);
            if(old_screen != c->screen)
            {
                move_client_to_screen(c, c->screen, False);
                statusbar_draw_all(old_screen);
                statusbar_draw_all(c->screen);
            }
            tag_client_with_rules(c);
            XMoveResizeWindow(e->xany.display, c->win, c->rx, c->ry, c->rw, c->rh);
            arrange(c->screen);
        }
        else
            window_configure(c->win, c->x, c->y, c->w, c->h, c->border);
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
handle_event_configurenotify(XEvent * e)
{
    XConfigureEvent *ev = &e->xconfigure;
    int screen;
    Area area;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if(ev->window == RootWindow(e->xany.display, screen)
           && (ev->width != DisplayWidth(e->xany.display, screen)
               || ev->height != DisplayHeight(e->xany.display, screen)))
        {
            DisplayWidth(e->xany.display, screen) = ev->width;
            DisplayHeight(e->xany.display, screen) = ev->height;

            /* update statusbar */
            area = get_screen_area(screen, NULL, &globalconf.screens[screen].padding);
            globalconf.screens[screen].statusbar->width = area.width;

            XResizeWindow(e->xany.display,
                          globalconf.screens[screen].statusbar->window,
                          globalconf.screens[screen].statusbar->width,
                          globalconf.screens[screen].statusbar->height);

            statusbar_draw_all(screen);
            arrange(screen);
        }
}

void
handle_event_destroynotify(XEvent * e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = get_client_bywin(globalconf.clients, ev->window)))
        client_unmanage(c, WithdrawnState);
}

void
handle_event_enternotify(XEvent * e)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;
    int screen;
    Tag **curtags;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = get_client_bywin(globalconf.clients, ev->window)))
    {
        curtags = get_current_tags(c->screen);
        focus(c, ev->same_screen, c->screen);
        if (c->isfloating || curtags[0]->layout->arrange == layout_floating)
            window_grabbuttons(get_phys_screen(c->screen), c->win, True, False);
        p_delete(&curtags);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(ev->window == RootWindow(e->xany.display, screen))
                focus(NULL, True, screen);
}

void
handle_event_expose(XEvent *e)
{
    XExposeEvent *ev = &e->xexpose;
    int screen;
    Statusbar *statusbar;

    if(!ev->count)
        for(screen = 0; screen < get_screen_count(); screen++)
            for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
                if(statusbar->window == ev->window)
                {
                    statusbar_display(statusbar);
                    return;
                }
}

void
handle_event_keypress(XEvent * e)
{
    int screen, x, y, d;
    unsigned int m;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;
    Window dummy;
    Key *k;

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
                screen = get_screen_bycoord(x, y);
            break;
        }

    for(k = globalconf.keys; k; k = k->next)
        if(keysym == k->keysym && k->func
           && CLEANMASK(k->mod) == CLEANMASK(ev->state))
        {
            k->func(screen, k->arg);
            break;
        }
}

void
handle_event_leavenotify(XEvent * e)
{
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if((ev->window == RootWindow(e->xany.display, screen)) && !ev->same_screen)
            focus(NULL, ev->same_screen, screen);
}

void
handle_event_mappingnotify(XEvent * e)
{
    XMappingEvent *ev = &e->xmapping;
    int screen;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            grabkeys(get_phys_screen(screen));
}

void
handle_event_maprequest(XEvent * e)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    int screen, x, y, d;
    unsigned int m;
    Window dummy;

    if(!XGetWindowAttributes(e->xany.display, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;
    if(!get_client_bywin(globalconf.clients, ev->window))
    {
        for(screen = 0; wa.screen != ScreenOfDisplay(e->xany.display, screen); screen++);
        if(screen == 0 && XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen),
                                        &dummy, &dummy, &x, &y, &d, &d, &m))
            screen = get_screen_bycoord(x, y);
        client_manage(ev->window, &wa, screen);
    }
}

void
handle_event_propertynotify(XEvent * e)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete)
        return;                 /* ignore */
    if((c = get_client_bywin(globalconf.clients, ev->window)))
    {
        switch (ev->atom)
        {
          case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (get_client_bywin(globalconf.clients, trans) != NULL)))
                arrange(c->screen);
            break;
          case XA_WM_NORMAL_HINTS:
            client_updatesizehints(c);
            break;
          case XA_WM_HINTS:
            client_updatewmhints(c);
            statusbar_draw_all(c->screen);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(globalconf.display, "_NET_WM_NAME", False))
        {
            client_updatetitle(c);
            if(c == globalconf.focus->client)
                statusbar_draw_all(c->screen);
        }
    }
}

void
handle_event_unmapnotify(XEvent * e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = get_client_bywin(globalconf.clients, ev->window))
       && ev->event == RootWindow(e->xany.display, get_phys_screen(c->screen))
       && ev->send_event && window_getstate(c->win) == NormalState)
        client_unmanage(c, WithdrawnState);
}

void
handle_event_shape(XEvent * e)
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = get_client_bywin(globalconf.clients, ev->window);

    if(c)
        window_setshape(get_phys_screen(c->screen), c->win);
}

void
handle_event_randr_screen_change_notify(XEvent *e)
{
    XRRUpdateConfiguration(e);
}

void
handle_event_clientmessage(XEvent *e)
{
    ewmh_process_client_message(&e->xclient);
}

void
grabkeys(int phys_screen)
{
    Key *k;
    KeyCode code;

    XUngrabKey(globalconf.display, AnyKey, AnyModifier, RootWindow(globalconf.display, phys_screen));
    for(k = globalconf.keys; k; k = k->next)
    {
        if((code = XKeysymToKeycode(globalconf.display, k->keysym)) == NoSymbol)
            continue;
        XGrabKey(globalconf.display, code, k->mod, RootWindow(globalconf.display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(globalconf.display, code, k->mod | LockMask, RootWindow(globalconf.display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(globalconf.display, code, k->mod | globalconf.numlockmask, RootWindow(globalconf.display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(globalconf.display, code, k->mod | globalconf.numlockmask | LockMask, RootWindow(globalconf.display, phys_screen), True, GrabModeAsync, GrabModeAsync);
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
