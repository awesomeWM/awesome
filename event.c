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
extern DC dc;                   /* global draw context */
extern Client *clients, *sel;   /* global client list */

#define CLEANMASK(mask)		(mask & ~(awesomeconf->numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)

static Client *
getclient(Window w)
{
    Client *c;

    for(c = clients; c && c->win != w; c = c->next);
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

    si = get_display_info(c->display, c->screen, awesomeconf->statusbar);

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(c->display, DefaultRootWindow(c->display), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, dc.cursor[CurMove], CurrentTime) != GrabSuccess)
        return;
    XQueryPointer(c->display, DefaultRootWindow(c->display), &dummy, &dummy, &x1, &y1, &di, &di, &dui);
    for(;;)
    {
        XMaskEvent(c->display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
        case ButtonRelease:
            XUngrabPointer(c->display, CurrentTime);
            return;
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handle_event_maprequest(&ev, awesomeconf);
            break;
        case MotionNotify:
            XSync(c->display, False);
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y1);
            if(abs(si->x_org + nx) < awesomeconf->snap)
                nx = si->x_org;
            else if(abs((si->x_org + si->width) - (nx + c->w + 2 * c->border)) < awesomeconf->snap)
                nx = si->x_org + si->width - c->w - 2 * c->border;
            if(abs(si->y_org - ny) < awesomeconf->snap)
                ny = si->y_org;
            else if(abs((si->y_org + si->height) - (ny + c->h + 2 * c->border)) < awesomeconf->snap)
                ny = si->y_org + si->height - c->h - 2 * c->border;
            resize(c, nx, ny, c->w, c->h, False);
            break;
        }
    }
    XFree(si);
}

static void
resizemouse(Client * c, awesome_config *awesomeconf)
{
    int ocx, ocy;
    int nw, nh;
    XEvent ev;

    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(c->display, DefaultRootWindow(c->display), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, dc.cursor[CurResize], CurrentTime) != GrabSuccess)
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
        case Expose:
        case MapRequest:
            handle_event_maprequest(&ev, awesomeconf);
            break;
        case MotionNotify:
            XSync(c->display, False);
            if((nw = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                nw = 1;
            if((nh = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                nh = 1;
            resize(c, c->x, c->y, nw, nh, True);
            break;
        }
    }
}

void
handle_event_buttonpress(XEvent * e, awesome_config *awesomeconf)
{
    int i;
    Client *c;
    XButtonPressedEvent *ev = &e->xbutton;

    if(awesomeconf->statusbar.window == ev->window)
    {
        int x = 0;
        for(i = 0; i < awesomeconf->ntags; i++)
        {
            x += textw(dc.font.set, dc.font.xfont, awesomeconf->tags[i], dc.font.height);
            if(ev->x < x)
            {
                if(ev->button == Button1)
                {
                    if(ev->state & awesomeconf->modkey)
                        uicb_tag(e->xany.display, &dc, awesomeconf, awesomeconf->tags[i]);
                    else
                        uicb_view(e->xany.display, &dc, awesomeconf, awesomeconf->tags[i]);
                }
                else if(ev->button == Button3)
                {
                    if(ev->state & awesomeconf->modkey)
                        uicb_toggletag(e->xany.display, &dc, awesomeconf, awesomeconf->tags[i]);
                    else
                        uicb_toggleview(e->xany.display, &dc, awesomeconf, awesomeconf->tags[i]);
                }
                return;
            }
        }
        if((ev->x < x + awesomeconf->statusbar.width) && ev->button == Button1)
            uicb_setlayout(e->xany.display, &dc, awesomeconf, NULL);
    }
    else if((c = getclient(ev->window)))
    {
        focus(c->display, c->screen, &dc, c, ev->same_screen, awesomeconf);
        if(CLEANMASK(ev->state) != awesomeconf->modkey)
            return;
        if(ev->button == Button1 && (IS_ARRANGE(floating) || c->isfloating))
        {
            restack(e->xany.display, &dc, awesomeconf);
            movemouse(c, awesomeconf);
        }
        else if(ev->button == Button2)
            uicb_zoom(e->xany.display, &dc, awesomeconf, NULL);
        else if(ev->button == Button3 && (IS_ARRANGE(floating) || c->isfloating) && !c->isfixed)
        {
            restack(e->xany.display, &dc, awesomeconf);
            resizemouse(c, awesomeconf);
        }
    }
    else if(DefaultRootWindow(e->xany.display) == ev->window && !sel)
    {
        if(ev->button == Button4)
            uicb_tag_viewnext(e->xany.display, &dc, awesomeconf, NULL);
        else if(ev->button == Button5)
            uicb_tag_viewprev(e->xany.display, &dc, awesomeconf, NULL);
    }
}

void
handle_event_configurerequest(XEvent * e, awesome_config *awesomeconf __attribute__ ((unused)))
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if((c = getclient(ev->window)))
    {
        c->ismax = False;
        if(ev->value_mask & CWBorderWidth)
            c->border = ev->border_width;
        if(c->isfixed || c->isfloating || IS_ARRANGE(floating))
        {
            if(ev->value_mask & CWX)
                c->x = ev->x;
            if(ev->value_mask & CWY)
                c->y = ev->y;
            if(ev->value_mask & CWWidth)
                c->w = ev->width;
            if(ev->value_mask & CWHeight)
                c->h = ev->height;
            if((c->x + c->w) > DisplayWidth(c->display, DefaultScreen(c->display)) && c->isfloating)
                c->x = DisplayWidth(c->display, DefaultScreen(c->display)) / 2 - c->w / 2;       /* center in x direction */
            if((c->y + c->h) > DisplayHeight(c->display, DefaultScreen(c->display)) && c->isfloating)
                c->y = DisplayHeight(c->display, DefaultScreen(c->display)) / 2 - c->h / 2;       /* center in y direction */
            if((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                configure(c);
            if(isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags))
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

    if(ev->window == DefaultRootWindow(e->xany.display)
       && (ev->width != DisplayWidth(e->xany.display, DefaultScreen(e->xany.display))
           || ev->height != DisplayHeight(e->xany.display, DefaultScreen(e->xany.display))))
    {
        DisplayWidth(e->xany.display, DefaultScreen(e->xany.display)) = ev->width;
        DisplayHeight(e->xany.display, DefaultScreen(e->xany.display)) = ev->height;
        XFreePixmap(e->xany.display, awesomeconf->statusbar.drawable);
        awesomeconf->statusbar.drawable = XCreatePixmap(e->xany.display, DefaultRootWindow(e->xany.display),
                                                        DisplayWidth(e->xany.display, DefaultScreen(e->xany.display)),
                                                        awesomeconf->statusbar.height,
                                                        DefaultDepth(e->xany.display, DefaultScreen(e->xany.display)));
        XResizeWindow(e->xany.display, awesomeconf->statusbar.window,
                      DisplayWidth(e->xany.display, DefaultScreen(e->xany.display)), awesomeconf->statusbar.height);
        updatebarpos(e->xany.display, awesomeconf->statusbar);
        arrange(e->xany.display, &dc, awesomeconf);
    }
}

void
handle_event_destroynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = getclient(ev->window)))
        unmanage(c, &dc, WithdrawnState, awesomeconf);
}

void
handle_event_enternotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = getclient(ev->window)))
        focus(c->display, c->screen, &dc, c, ev->same_screen, awesomeconf);
    else if(ev->window == DefaultRootWindow(e->xany.display))
        focus(e->xany.display, DefaultScreen(e->xany.display), &dc, NULL, True, awesomeconf);
}

void
handle_event_expose(XEvent * e, awesome_config *awesomeconf)
{
    XExposeEvent *ev = &e->xexpose;

    if(!ev->count && awesomeconf->statusbar.window == ev->window)
        drawstatusbar(e->xany.display,  DefaultScreen(e->xany.display), &dc, awesomeconf);
}

void
handle_event_keypress(XEvent * e, awesome_config *awesomeconf)
{
    int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XKeycodeToKeysym(e->xany.display, (KeyCode) ev->keycode, 0);
    for(i = 0; i < awesomeconf->nkeys; i++)
        if(keysym == awesomeconf->keys[i].keysym
           && CLEANMASK(awesomeconf->keys[i].mod) == CLEANMASK(ev->state) && awesomeconf->keys[i].func)
            awesomeconf->keys[i].func(e->xany.display, &dc, awesomeconf, awesomeconf->keys[i].arg);
}

void
handle_event_leavenotify(XEvent * e, awesome_config *awesomeconf)
{
    XCrossingEvent *ev = &e->xcrossing;

    if((ev->window == DefaultRootWindow(e->xany.display)) && !ev->same_screen)
        focus(e->xany.display, DefaultScreen(e->xany.display), &dc, NULL, ev->same_screen, awesomeconf);
}

void
handle_event_mappingnotify(XEvent * e, awesome_config *awesomeconf)
{
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        grabkeys(e->xany.display, DefaultScreen(e->xany.display), awesomeconf);
}

void
handle_event_maprequest(XEvent * e, awesome_config *awesomeconf)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;

    if(!XGetWindowAttributes(e->xany.display, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;
    if(!getclient(ev->window))
        manage(e->xany.display, DefaultScreen(e->xany.display), &dc, ev->window, &wa, awesomeconf);
}

void
handle_event_propertynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if(ev->state == PropertyDelete)
        return;                 /* ignore */
    if((c = getclient(ev->window)))
    {
        switch (ev->atom)
        {
        case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (getclient(trans) != NULL)))
                arrange(e->xany.display, &dc, awesomeconf);
            break;
        case XA_WM_NORMAL_HINTS:
            updatesizehints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(c->display, "_NET_WM_NAME", False))
        {
            updatetitle(c);
            if(c == sel)
                drawstatusbar(e->xany.display, c->screen, &dc, awesomeconf);
        }
    }
}

void
handle_event_unmapnotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = getclient(ev->window))
       && ev->event == RootWindow(e->xany.display, c->screen) && (ev->send_event || !c->unmapped--))
        unmanage(c, &dc, WithdrawnState, awesomeconf);
}

void
handle_event_shape(XEvent * e,
                   awesome_config *awesomeconf __attribute__ ((unused)))
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = getclient(ev->window);

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
        code = XKeysymToKeycode(disp, awesomeconf->keys[i].keysym);
        XGrabKey(disp, code, awesomeconf->keys[i].mod, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | LockMask, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask, RootWindow(disp, screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask | LockMask, RootWindow(disp, screen), True,
                 GrabModeAsync, GrabModeAsync);
    }
}
