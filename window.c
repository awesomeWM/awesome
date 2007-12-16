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
#include "util.h"

extern awesome_config globalconf;

/** Set client WM_STATE property
 * \param disp Display ref
 * \param win Window
 * \param state state
 */
int
window_setstate(Display *disp, Window win, long state)
{
    long data[] = { state, None };

    return XChangeProperty(disp, win, XInternAtom(disp, "WM_STATE", False),
                           XInternAtom(disp, "WM_STATE", False),  32,
                           PropModeReplace, (unsigned char *) data, 2);
}

/** Get a window state (WM_STATE)
 * \param disp Display ref
 * \param w Client window
 * \return state
 */
long
window_getstate(Display *disp, Window w)
{
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;
    if(XGetWindowProperty(disp, w, XInternAtom(disp, "WM_STATE", False),
                          0L, 2L, False, XInternAtom(disp, "WM_STATE", False),
                          &real, &format, &n, &extra, (unsigned char **) &p) != Success)
        return -1;
    if(n != 0)
        result = *p;
    p_delete(&p);
    return result;
}

Status
window_configure(Display *disp, Window win, int x, int y, int w, int h, int border)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = disp;
    ce.event = win;
    ce.window = win;
    ce.x = x;
    ce.y = y;
    ce.width = w;
    ce.height = h;
    ce.border_width = border;
    ce.above = None;
    ce.override_redirect = False;
    return XSendEvent(disp, win, False, StructureNotifyMask, (XEvent *) & ce);
}



/** Grab or ungrab buttons on a window
 * \param disp Display ref
 * \param focused True if client is focused
 * \param raised True if the client is above other clients
 * \param modkey Mod key mask
 */
void
window_grabbuttons(Display *disp, int screen, Window win, Bool focused, Bool raised)
{
    Button *b;

    XUngrabButton(disp, AnyButton, AnyModifier, win);

    if(focused)
    {
        if(!raised)
            XGrabButton(disp, Button1, NoSymbol, win, False,
                        BUTTONMASK, GrabModeSync, GrabModeAsync, None, None);

        for(b = globalconf.buttons.client; b; b = b->next)
        {
            XGrabButton(disp, b->button, b->mod, win, False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | LockMask, win, False,
                        BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | globalconf.numlockmask, win, False,
                        BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | globalconf.numlockmask | LockMask,
                        win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        }
        
        XUngrabButton(disp, AnyButton, AnyModifier, RootWindow(disp, screen));
    }
    else
    {
        XGrabButton(disp, AnyButton, AnyModifier, win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

        for(b = globalconf.buttons.root; b; b = b->next)
        {
            XGrabButton(disp, b->button, b->mod,
                        RootWindow(disp, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | LockMask,
                        RootWindow(disp, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | globalconf.numlockmask,
                        RootWindow(disp, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
            XGrabButton(disp, b->button, b->mod | globalconf.numlockmask | LockMask,
                        RootWindow(disp, screen), False, BUTTONMASK,
                        GrabModeAsync, GrabModeSync, None, None);
        }
    }
}

void
window_setshape(Display *disp, int screen, Window win)
{
    int bounding_shaped;
    int i, b;  unsigned int u;  /* dummies */
    /* Logic to decide if we have a shaped window cribbed from fvwm-2.5.10. */
    if(XShapeQueryExtents(disp, win, &bounding_shaped, &i, &i,
                          &u, &u, &b, &i, &i, &u, &u) && bounding_shaped)
        XShapeCombineShape(disp, RootWindow(disp, screen), ShapeBounding, 0, 0, win, ShapeBounding, ShapeSet);
}

void
window_settrans(Display *disp, Window win, double opacity)
{
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 100)
    {
        real_opacity = ((opacity / 100.0) * 0xffffffff);
        XChangeProperty(disp, win, XInternAtom(disp, "_NET_WM_WINDOW_OPACITY", False),
                        XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &real_opacity, 1L);
    }
    else
        XDeleteProperty(disp, win, XInternAtom(disp, "_NET_WM_WINDOW_OPACITY", False));

    XSync(disp, False);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
