/*
 * window.c - window handling functions
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

#include <X11/Xatom.h>
#include <X11/extensions/shape.h> 

#include "window.h"
#include "common/util.h"

extern AwesomeConf globalconf;

/** Mask shorthands, used in event.c and window.c */
#define BUTTONMASK     (ButtonPressMask | ButtonReleaseMask)

/** Set client WM_STATE property
 * \param win Window
 * \param state state
 * \return XChangeProperty result
 */
int
window_setstate(Window win, long state)
{
    long data[] = { state, None };

    return XChangeProperty(globalconf.display, win, XInternAtom(globalconf.display, "WM_STATE", False),
                           XInternAtom(globalconf.display, "WM_STATE", False),  32,
                           PropModeReplace, (unsigned char *) data, 2);
}

/** Get a window state (WM_STATE)
 * \param w Client window
 * \return state
 */
long
window_getstate(Window w)
{
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;
    if(XGetWindowProperty(globalconf.display, w, XInternAtom(globalconf.display, "WM_STATE", False),
                          0L, 2L, False, XInternAtom(globalconf.display, "WM_STATE", False),
                          &real, &format, &n, &extra, (unsigned char **) &p) != Success)
        return -1;
    if(n != 0)
        result = *p;
    p_delete(&p);
    return result;
}

Status
window_configure(Window win, Area geometry, int border)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = globalconf.display;
    ce.event = win;
    ce.window = win;
    ce.x = geometry.x;
    ce.y = geometry.y;
    ce.width = geometry.width;
    ce.height = geometry.height;
    ce.border_width = border;
    ce.above = None;
    ce.override_redirect = False;
    return XSendEvent(globalconf.display, win, False, StructureNotifyMask, (XEvent *) & ce);
}

/** Grab or ungrab buttons on a window
 * \param screen The screen
 * \param win The window
 * \param focused True if client is focused
 * \param raised True if the client is above other clients
 */
void
window_grabbuttons(int screen,
                   Window win,
                   Bool focused,
                   Bool raised)
{
    Button *b;

    XUngrabButton(globalconf.display, AnyButton, AnyModifier, win);

    if(focused)
    {
        if(!raised)
            XGrabButton(globalconf.display, Button1, NoSymbol, win, False,
                        BUTTONMASK, GrabModeSync, GrabModeAsync, None, None);

        for(b = globalconf.buttons.client; b; b = b->next)
        {
            XGrabButton(globalconf.display, b->button, b->mod, win, False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | LockMask, win, False,
                        BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | globalconf.numlockmask, win, False,
                        BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | globalconf.numlockmask | LockMask,
                        win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        }
        
        XUngrabButton(globalconf.display, AnyButton, AnyModifier, RootWindow(globalconf.display, screen));
    }
    else
    {
        XGrabButton(globalconf.display, AnyButton, AnyModifier, win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

        for(b = globalconf.buttons.root; b; b = b->next)
        {
            XGrabButton(globalconf.display, b->button, b->mod,
                        RootWindow(globalconf.display, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | LockMask,
                        RootWindow(globalconf.display, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | globalconf.numlockmask,
                        RootWindow(globalconf.display, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(globalconf.display, b->button, b->mod | globalconf.numlockmask | LockMask,
                        RootWindow(globalconf.display, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
        }
    }
}

void
window_setshape(int screen, Window win)
{
    int bounding_shaped;
    int i, b;  unsigned int u;  /* dummies */
    /* Logic to decide if we have a shaped window cribbed from fvwm-2.5.10. */
    if(XShapeQueryExtents(globalconf.display, win, &bounding_shaped, &i, &i,
                          &u, &u, &b, &i, &i, &u, &u) && bounding_shaped)
        XShapeCombineShape(globalconf.display, RootWindow(globalconf.display, screen), ShapeBounding, 0, 0, win, ShapeBounding, ShapeSet);
}

int
window_settrans(Window win, double opacity)
{
    int status;
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 100)
    {
        real_opacity = ((opacity / 100.0) * 0xffffffff);
        status = XChangeProperty(globalconf.display, win,
                                 XInternAtom(globalconf.display, "_NET_WM_WINDOW_OPACITY", False),
                                 XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &real_opacity, 1L);
    }
    else
        status = XDeleteProperty(globalconf.display, win,
                                 XInternAtom(globalconf.display, "_NET_WM_WINDOW_OPACITY", False));

    return status;
}

SimpleWindow *
simplewindow_new(int phys_screen, int x, int y, unsigned int w, unsigned int h,
                  unsigned int border_width, Bool rotate_drawable)
{
    XSetWindowAttributes wa;
    SimpleWindow *sw;

    sw = p_new(SimpleWindow, 1);

    sw->geometry.x = x;
    sw->geometry.y = y;
    sw->geometry.width = w;
    sw->geometry.height = h;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = globalconf.cursor[CurNormal];
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    sw->window = XCreateWindow(globalconf.display,
                               RootWindow(globalconf.display, phys_screen),
                               x, y, w, h,
                               border_width,
                               DefaultDepth(globalconf.display, phys_screen),
                               CopyFromParent,
                               DefaultVisual(globalconf.display, phys_screen),
                               CWOverrideRedirect | CWBackPixmap | CWEventMask,
                               &wa);

    if(rotate_drawable)
        sw->drawable = XCreatePixmap(globalconf.display,
                                     RootWindow(globalconf.display, phys_screen),
                                     h, w,
                                     DefaultDepth(globalconf.display, phys_screen));
    else
        sw->drawable = XCreatePixmap(globalconf.display,
                                     RootWindow(globalconf.display, phys_screen),
                                     w, h,
                                     DefaultDepth(globalconf.display, phys_screen));

    XDefineCursor(globalconf.display,
                  sw->window,
                  globalconf.cursor[CurNormal]);

    return sw;
}

void
simplewindow_delete(SimpleWindow *sw)
{
    XDestroyWindow(globalconf.display, sw->window);
    XFreePixmap(globalconf.display, sw->drawable);
    p_delete(&sw);
}

int
simplewindow_move(SimpleWindow *sw, int x, int y)
{
    sw->geometry.x = x;
    sw->geometry.y = y;
    return XMoveWindow(globalconf.display, sw->window, x, y);
}

int
simplewindow_refresh_drawable(SimpleWindow *sw, int phys_screen)
{
    return XCopyArea(globalconf.display, sw->drawable,
                     sw->window,
                     DefaultGC(globalconf.display, phys_screen), 0, 0,
                     sw->geometry.width,
                     sw->geometry.height,
                     0, 0);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
