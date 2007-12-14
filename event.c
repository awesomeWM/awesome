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
#include "window.h"
#include "mouse.h"
#include "layouts/tile.h"
#include "layouts/floating.h"

#define CLEANMASK(mask, acf)		(mask & ~(acf->numlockmask | LockMask))

static void
handle_mouse_button_press(awesome_config *awesomeconf, int screen,
                          unsigned int button, unsigned int state,
                          Button *buttons, char *arg)
{
    Button *b;

    for(b = buttons; b; b = b->next)
        if(button == b->button && CLEANMASK(state, awesomeconf) == b->mod && b->func)
        {
            if(arg)
                b->func(awesomeconf, screen, arg);
            else
                b->func(awesomeconf, screen, b->arg);
            return;
        }
}

void
handle_event_buttonpress(XEvent * e, awesome_config *awesomeconf)
{
    int i, screen, x = 0, y = 0;
    unsigned int udummy;
    Client *c;
    Window wdummy;
    char arg[256];
    XButtonPressedEvent *ev = &e->xbutton;

    for(screen = 0; screen < get_screen_count(e->xany.display); screen++)
        if(awesomeconf->screens[screen].statusbar.window == ev->window)
        {
            for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
            {
                x += textwidth(e->xany.display, awesomeconf->screens[screen].font, awesomeconf->screens[screen].tags[i].name);
                if(((awesomeconf->screens[screen].statusbar.position == BarTop
                     || awesomeconf->screens[screen].statusbar.position == BarBot)
                   && ev->x < x)
                   || (awesomeconf->screens[screen].statusbar.position == BarRight && ev->y < x)
                   || (awesomeconf->screens[screen].statusbar.position == BarLeft && ev->y > awesomeconf->screens[screen].statusbar.width - x))
                {
                    snprintf(arg, sizeof(arg), "%d", i + 1);
                    handle_mouse_button_press(awesomeconf, screen, ev->button, ev->state,
                                              awesomeconf->buttons.tag, arg);
                    return;
                }
            }
            x += awesomeconf->screens[screen].statusbar.txtlayoutwidth;
            if(((awesomeconf->screens[screen].statusbar.position == BarTop
                 || awesomeconf->screens[screen].statusbar.position == BarBot)
                && ev->x < x)
                || (awesomeconf->screens[screen].statusbar.position == BarRight && ev->y < x)
                || (awesomeconf->screens[screen].statusbar.position == BarLeft && ev->y > awesomeconf->screens[screen].statusbar.width - x))
                handle_mouse_button_press(awesomeconf, screen, ev->button, ev->state,
                                          awesomeconf->buttons.layout, NULL);
            else
                handle_mouse_button_press(awesomeconf, screen, ev->button, ev->state,
                                          awesomeconf->buttons.title, NULL);
            return;
        }

    if((c = get_client_bywin(awesomeconf->clients, ev->window)))
    {
        focus(c, ev->same_screen, awesomeconf, c->screen);
        if(CLEANMASK(ev->state, awesomeconf) == NoSymbol
           && ev->button == Button1)
        {
            restack(awesomeconf, c->screen);
            XAllowEvents(c->display, ReplayPointer, CurrentTime);
            window_grabbuttons(c->display, c->phys_screen, c->win,
                               True, True, awesomeconf->buttons.root,
                               awesomeconf->buttons.client, awesomeconf->numlockmask);
        }
        else
            handle_mouse_button_press(awesomeconf, c->screen, ev->button, ev->state,
                                      awesomeconf->buttons.client, NULL);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(RootWindow(e->xany.display, screen) == ev->window
               && XQueryPointer(e->xany.display, ev->window, &wdummy, &wdummy, &x, &y, &i, &i, &udummy))
            {
                screen = get_screen_bycoord(e->xany.display, x, y);
                handle_mouse_button_press(awesomeconf, screen, ev->button, ev->state,
                                          awesomeconf->buttons.root, NULL);
                return;
            }
}

void
handle_event_configurerequest(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    int old_screen;

    if((c = get_client_bywin(awesomeconf->clients, ev->window)))
    {
        c->ismax = False;
        if(ev->value_mask & CWBorderWidth)
            c->border = ev->border_width;
        if(c->isfixed || c->isfloating
           || get_current_layout(awesomeconf->screens[c->screen])->arrange == layout_floating)
        {
            if(ev->value_mask & CWX)
                c->rx = c->x = ev->x;
            if(ev->value_mask & CWY)
                c->ry = c->y = ev->y;
            if(ev->value_mask & CWWidth)
                c->rw = c->w = ev->width;
            if(ev->value_mask & CWHeight)
                c->rh = c->h = ev->height;
            if((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);
            /* recompute screen */
            old_screen = c->screen;
            c->screen = get_screen_bycoord(c->display, c->x, c->y);
            if(old_screen != c->screen)
            {
                move_client_to_screen(c, awesomeconf, c->screen, False);
                drawstatusbar(awesomeconf, old_screen);
                drawstatusbar(awesomeconf, c->screen);
            }
            tag_client_with_rules(c, awesomeconf);
            XMoveResizeWindow(e->xany.display, c->win, c->rx, c->ry, c->rw, c->rh);
            arrange(awesomeconf, c->screen);
        }
        else
            window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);
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

            /* update statusbar */
            si = get_screen_info(e->xany.display, screen, NULL, &awesomeconf->screens[screen].padding);
            awesomeconf->screens[screen].statusbar.width = si[screen].width;
            p_delete(&si);

            XResizeWindow(e->xany.display,
                          awesomeconf->screens[screen].statusbar.window,
                          awesomeconf->screens[screen].statusbar.width,
                          awesomeconf->screens[screen].statusbar.height);

            updatebarpos(e->xany.display, awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);
            arrange(awesomeconf, screen);
        }
}

void
handle_event_destroynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = get_client_bywin(awesomeconf->clients, ev->window)))
        client_unmanage(c, WithdrawnState, awesomeconf);
}

void
handle_event_enternotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = get_client_bywin(awesomeconf->clients, ev->window)))
    {
        focus(c, ev->same_screen, awesomeconf, c->screen);
        if (c->isfloating
                || get_current_layout(awesomeconf->screens[c->screen])->arrange == layout_floating)
            window_grabbuttons(c->display, c->phys_screen, c->win,
                               True, False, awesomeconf->buttons.root,
                               awesomeconf->buttons.client, awesomeconf->numlockmask);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(ev->window == RootWindow(e->xany.display, screen))
                focus(NULL, True, awesomeconf, screen);
}

void
handle_event_expose(XEvent * e, awesome_config *awesomeconf)
{
    XExposeEvent *ev = &e->xexpose;
    int screen;

    if(!ev->count)
        for(screen = 0; screen < get_screen_count(e->xany.display); screen++)
            if(awesomeconf->screens[screen].statusbar.window == ev->window)
                drawstatusbar(awesomeconf, screen);
}

void
handle_event_keypress(XEvent * e, awesome_config *awesomeconf)
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
                screen = get_screen_bycoord(e->xany.display, x, y);
            break;
        }

    for(k = awesomeconf->keys; k; k = k->next)
        if(keysym == k->keysym && k->func
           && CLEANMASK(k->mod, awesomeconf) == CLEANMASK(ev->state, awesomeconf))
        {
            k->func(awesomeconf, screen, k->arg);
            break;
        }
}

void
handle_event_leavenotify(XEvent * e, awesome_config *awesomeconf)
{
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if((ev->window == RootWindow(e->xany.display, screen)) && !ev->same_screen)
            focus(NULL, ev->same_screen, awesomeconf, screen);
}

void
handle_event_mappingnotify(XEvent * e, awesome_config *awesomeconf)
{
    XMappingEvent *ev = &e->xmapping;
    int screen;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            grabkeys(awesomeconf, get_phys_screen(awesomeconf->display, screen));
}

void
handle_event_maprequest(XEvent * e, awesome_config *awesomeconf)
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
    if(!get_client_bywin(awesomeconf->clients, ev->window))
    {
        for(screen = 0; wa.screen != ScreenOfDisplay(e->xany.display, screen); screen++);
        if(screen == 0 && XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen),
                                        &dummy, &dummy, &x, &y, &d, &d, &m))
            screen = get_screen_bycoord(e->xany.display, x, y);
        client_manage(ev->window, &wa, awesomeconf, screen);
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
    if((c = get_client_bywin(awesomeconf->clients, ev->window)))
    {
        switch (ev->atom)
        {
          case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (get_client_bywin(awesomeconf->clients, trans) != NULL)))
                arrange(awesomeconf, c->screen);
            break;
          case XA_WM_NORMAL_HINTS:
            client_updatesizehints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(c->display, "_NET_WM_NAME", False))
        {
            client_updatetitle(c);
            if(c == get_current_tag(awesomeconf->screens[c->screen])->client_sel)
                drawstatusbar(awesomeconf, c->screen);
        }
    }
}

void
handle_event_unmapnotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = get_client_bywin(awesomeconf->clients, ev->window))
       && ev->event == RootWindow(e->xany.display, c->phys_screen)
       && ev->send_event && window_getstate(c->display, c->win) == NormalState)
        client_unmanage(c, WithdrawnState, awesomeconf);
}

void
handle_event_shape(XEvent * e,
                   awesome_config *awesomeconf __attribute__ ((unused)))
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = get_client_bywin(awesomeconf->clients, ev->window);

    if(c)
        window_setshape(c->display, c->phys_screen, c->win);
}

void
handle_event_randr_screen_change_notify(XEvent *e,
                                        awesome_config *awesomeconf __attribute__ ((unused)))
{
    XRRUpdateConfiguration(e);
}

void
grabkeys(awesome_config *awesomeconf, int phys_screen)
{
    Key *k;
    KeyCode code;

    XUngrabKey(awesomeconf->display, AnyKey, AnyModifier, RootWindow(awesomeconf->display, phys_screen));
    for(k = awesomeconf->keys; k; k = k->next)
    {
        if((code = XKeysymToKeycode(awesomeconf->display, k->keysym)) == NoSymbol)
            continue;
        XGrabKey(awesomeconf->display, code, k->mod, RootWindow(awesomeconf->display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, k->mod | LockMask, RootWindow(awesomeconf->display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, k->mod | awesomeconf->numlockmask, RootWindow(awesomeconf->display, phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, k->mod | awesomeconf->numlockmask | LockMask, RootWindow(awesomeconf->display, phys_screen), True, GrabModeAsync, GrabModeAsync);
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
