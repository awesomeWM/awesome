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
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include "screen.h"
#include "event.h"
#include "tag.h"
#include "statusbar.h"
#include "window.h"
#include "mouse.h"
#include "ewmh.h"
#include "client.h"
#include "widget.h"
#include "rules.h"
#include "titlebar.h"
#include "layouts/tile.h"
#include "layouts/floating.h"
#include "common/xscreen.h"

extern AwesomeConf globalconf;

/** Handle mouse button click
 * \param screen screen number
 * \param button button number
 * \param state modkeys state
 * \param buttons buttons list to check for
 * \param arg optional arg passed to uicb, otherwise buttons' arg are used
 */
static void
event_handle_mouse_button_press(int screen, unsigned int button, unsigned int state,
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
        }
}

/** Handle XButtonPressed events
 * \param e XEvent
 */
void
event_handle_buttonpress(XEvent *e)
{
    int i, screen, x = 0, y = 0;
    unsigned int udummy;
    Client *c;
    Window wdummy;
    Widget *widget;
    Statusbar *statusbar;
    XButtonPressedEvent *ev = &e->xbutton;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
            if(statusbar->sw->window == ev->window || statusbar->sw->window == ev->subwindow)
            {
                switch(statusbar->position)
                {
                  case Top:
                  case Bottom:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->x >= widget->area.x && ev->x < widget->area.x + widget->area.width
                           && ev->y >= widget->area.y && ev->y < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }
                    break;
                  case Right:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(ev->y >= widget->area.x && ev->y < widget->area.x + widget->area.width
                           && statusbar->sw->geometry.width - ev->x >= widget->area.y
                           && statusbar->sw->geometry.width - ev->x
                              < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }
                    break;
                  case Left:
                    for(widget = statusbar->widgets; widget; widget = widget->next)
                        if(statusbar->sw->geometry.height - ev->y >= widget->area.x
                           && statusbar->sw->geometry.height - ev->y
                              < widget->area.x + widget->area.width
                           && ev->x >= widget->area.y && ev->x < widget->area.y + widget->area.height)
                        {
                            widget->button_press(widget, ev);
                            return;
                        }
                    break;
                  default:
                    break;
                }
                /* return if no widget match */
                return;
            }

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar.sw && c->titlebar.sw->window == ev->window)
        {
            if(!client_focus(c, c->screen, True))
                client_stack(c);
            if(CLEANMASK(ev->state) == NoSymbol
               && ev->button == Button1)
                window_grabbuttons(c->win, c->phys_screen);
            event_handle_mouse_button_press(c->screen, ev->button, ev->state,
                                            globalconf.buttons.titlebar, NULL);
            return;
        }

    if((c = client_get_bywin(globalconf.clients, ev->window)))
    {
        if(!client_focus(c, c->screen, True))
            client_stack(c);
        if(CLEANMASK(ev->state) == NoSymbol
           && ev->button == Button1)
        {
            XAllowEvents(globalconf.display, ReplayPointer, CurrentTime);
            window_grabbuttons(c->win, c->phys_screen);
        }
        else
            event_handle_mouse_button_press(c->screen, ev->button, ev->state, globalconf.buttons.client, NULL);
    }
    else
    {
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(RootWindow(e->xany.display, screen) == ev->window
               && XQueryPointer(e->xany.display,
                                ev->window, &wdummy,
                                &wdummy, &x, &y, &i,
                                &i, &udummy))
            {
                screen = screen_get_bycoord(globalconf.screens_info, screen, x, y);
                event_handle_mouse_button_press(screen, ev->button, ev->state,
                                          globalconf.buttons.root, NULL);
                return;
            }
    }
}

/** Handle XConfigureRequest events
 * \param e XEvent
 */
void
event_handle_configurerequest(XEvent * e)
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;
    area_t geometry;

    if((c = client_get_bywin(globalconf.clients, ev->window)))
    {
        geometry = c->geometry;

        if(ev->value_mask & CWX)
            geometry.x = ev->x;
        if(ev->value_mask & CWY)
            geometry.y = ev->y;
        if(ev->value_mask & CWWidth)
            geometry.width = ev->width;
        if(ev->value_mask & CWHeight)
            geometry.height = ev->height;

        if(geometry.x != c->geometry.x || geometry.y != c->geometry.y
           || geometry.width != c->geometry.width || geometry.height != c->geometry.height)
        {
            if(c->isfloating || layout_get_current(c->screen)->arrange == layout_floating)
                client_resize(c, geometry, False);
            else
            {
                globalconf.screens[c->screen].need_arrange = True;
                /* If we do not resize the client, at least tell it that it
                 * has its new configuration. That fixes at least
                 * gnome-terminal */
                window_configure(c->win, c->geometry, c->border);
            }
        }
        else
        {
            titlebar_update_geometry_floating(c);
            window_configure(c->win, geometry, c->border);
        }
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
}

/** Handle XConfigure events
 * \param e XEvent
 */
void
event_handle_configurenotify(XEvent * e)
{
    XConfigureEvent *ev = &e->xconfigure;
    int screen;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if(ev->window == RootWindow(e->xany.display, screen)
           && (ev->width != DisplayWidth(e->xany.display, screen)
               || ev->height != DisplayHeight(e->xany.display, screen)))
            /* it's not that we panic, but restart */
            uicb_restart(0, NULL);
}

/** Handle XDestroyWindow events
 * \param e XEvent
 */
void
event_handle_destroynotify(XEvent *e)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = client_get_bywin(globalconf.clients, ev->window)))
        client_unmanage(c);
}

/** Handle XCrossing events on enter
 * \param e XEvent
 */
void
event_handle_enternotify(XEvent *e)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    if(ev->mode != NotifyNormal
       || (ev->x_root == globalconf.pointer_x
           && ev->y_root == globalconf.pointer_y))
        return;

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar.sw && c->titlebar.sw->window == ev->window)
            break;

    if(c || (c = client_get_bywin(globalconf.clients, ev->window)))
    {
        window_grabbuttons(c->win, c->phys_screen);
        /* the idea behind saving pointer_x and pointer_y is Bob Marley powered
         * this will allow us top drop some EnterNotify events and thus not giving
         * focus to windows appering under the cursor without a cursor move */
        globalconf.pointer_x = ev->x_root;
        globalconf.pointer_y = ev->y_root;

        if(globalconf.screens[c->screen].sloppy_focus)
            client_focus(c, c->screen,
                         globalconf.screens[c->screen].sloppy_focus_raise);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(ev->window == RootWindow(e->xany.display, screen))
            {
                window_root_grabbuttons(screen);
                return;
            }
}

/** Handle XExpose events
 * \param e XEvent
 */
void
event_handle_expose(XEvent *e)
{
    XExposeEvent *ev = &e->xexpose;
    int screen;
    Statusbar *statusbar;
    Client *c;

    if(!ev->count)
    {
        for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
            for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
                if(statusbar->sw->window == ev->window)
                {
                    statusbar_display(statusbar);
                    return;
                }

        for(c = globalconf.clients; c; c = c->next)
            if(c->titlebar.sw && c->titlebar.sw->window == ev->window)
            {
                simplewindow_refresh_drawable(c->titlebar.sw, c->phys_screen);
                return;
            }
    }
}

/** Handle XKey events
 * \param e XEvent
 */
void
event_handle_keypress(XEvent *e)
{
    int screen, x, y, d;
    unsigned int m;
    XKeyEvent *ev = &e->xkey;
    KeySym keysym;
    Window dummy;
    Key *k;

    /* find the right screen for this event */
    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if(XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen), &dummy, &dummy, &x, &y, &d, &d, &m))
        {
            /* if screen is 0, we are on first Zaphod screen or on the
             * only screen in Xinerama, so we can ask for a better screen
             * number with screen_get_bycoord: we'll get 0 in Zaphod mode
             * so it's the same, or maybe the real Xinerama screen */
            screen = screen_get_bycoord(globalconf.screens_info, screen, x, y);
            break;
        }

    keysym = XkbKeycodeToKeysym(globalconf.display, (KeyCode) ev->keycode, 0, 0);

    for(k = globalconf.keys; k; k = k->next)
        if(((k->keycode && ev->keycode == k->keycode) || (k->keysym && keysym == k->keysym)) &&
	  k->func && CLEANMASK(k->mod) == CLEANMASK(ev->state))
            k->func(screen, k->arg);
}

/** Handle XMapping events
 * \param e XEvent
 */
void
event_handle_mappingnotify(XEvent *e)
{
    XMappingEvent *ev = &e->xmapping;
    int screen;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            window_root_grabkeys(screen);
}

/** Handle XMapRequest events
 * \param e XEvent
 */
void
event_handle_maprequest(XEvent *e)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    int screen = 0, x, y, d;
    unsigned int m;
    Window dummy;

    if(!XGetWindowAttributes(e->xany.display, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;
    if(!client_get_bywin(globalconf.clients, ev->window))
    {
        if(globalconf.screens_info->xinerama_is_active
           && XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen),
                            &dummy, &dummy, &x, &y, &d, &d, &m))
            screen = screen_get_bycoord(globalconf.screens_info, screen, x, y);
        else
             for(screen = 0; wa.screen != ScreenOfDisplay(e->xany.display, screen); screen++);

        client_manage(ev->window, &wa, screen);
    }
}

/** Handle XProperty events
 * \param e XEvent
 */
void
event_handle_propertynotify(XEvent * e)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete)
        return;                 /* ignore */
    if((c = client_get_bywin(globalconf.clients, ev->window)))
    {
        switch (ev->atom)
        {
          case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating
               && (c->isfloating = (client_get_bywin(globalconf.clients, trans) != NULL)))
                globalconf.screens[c->screen].need_arrange = True;
            break;
          case XA_WM_NORMAL_HINTS:
            client_updatesizehints(c);
            break;
          case XA_WM_HINTS:
            client_updatewmhints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(globalconf.display, "_NET_WM_NAME", False))
            client_updatetitle(c);
    }
}

/** Handle XUnmap events
 * \param e XEvent
 */
void
event_handle_unmapnotify(XEvent * e)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = client_get_bywin(globalconf.clients, ev->window))
       && ev->event == RootWindow(e->xany.display, c->phys_screen)
       && ev->send_event && window_getstate(c->win) == NormalState)
        client_unmanage(c);
}

/** Handle XShape events
 * \param e XEvent
 */
void
event_handle_shape(XEvent * e)
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = client_get_bywin(globalconf.clients, ev->window);

    if(c)
        window_setshape(c->win, c->phys_screen);
}

/** Handle XRandR events
 * \param e XEvent
 */
void
event_handle_randr_screen_change_notify(XEvent *e)
{
    XRRUpdateConfiguration(e);
    uicb_restart(0, NULL);
}

/** Handle XClientMessage events
 * \param e XEvent
 */
void
event_handle_clientmessage(XEvent *e)
{
    ewmh_process_client_message(&e->xclient);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
