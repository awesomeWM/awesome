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
#include "layouts/tile.h"
#include "layouts/floating.h"

#define CLEANMASK(mask, acf)		(mask & ~(acf.numlockmask | LockMask))
#define MOUSEMASK	                (BUTTONMASK | PointerMotionMask)

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
         uicb_togglefloating(&awesomeconf[c->screen], "DUMMY");
     else
         restack(awesomeconf);

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
            p_delete(&si);
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
            client_resize(c, nx, ny, c->w, c->h, &awesomeconf[c->screen], False, False);
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
        restack(&awesomeconf[c->screen]);

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
            client_resize(c, c->x, c->y, nw, nh, &awesomeconf[c->screen], True, False);
            break;
        }
    }
}


static void
handle_mouse_button_press(awesome_config *awesomeconf,
                          unsigned int button, unsigned int state,
                          Button *buttons, char *arg)
{
    Button *b;

    for(b = buttons; b; b = b->next)
        if(button == b->button && CLEANMASK(state, awesomeconf[0]) == b->mod && b->func)
        {
            if(arg)
                b->func(awesomeconf, arg);
            else
                b->func(awesomeconf, b->arg);
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
        if(awesomeconf[screen].statusbar.window == ev->window)
        {
            for(i = 0; i < awesomeconf[screen].ntags; i++)
            {
                x += textwidth(e->xany.display, awesomeconf[screen].font, awesomeconf[screen].tags[i].name);
                if(((awesomeconf[screen].statusbar.position == BarTop
                     || awesomeconf[screen].statusbar.position == BarBot)
                   && ev->x < x)
                   || (awesomeconf[screen].statusbar.position == BarRight && ev->y < x)
                   || (awesomeconf[screen].statusbar.position == BarLeft && ev->y > awesomeconf[screen].statusbar.width - x))
                {
                    snprintf(arg, sizeof(arg), "%d", i + 1);
                    handle_mouse_button_press(&awesomeconf[screen], ev->button, ev->state,
                                              awesomeconf[screen].buttons.tag, arg);
                    return;
                }
            }
            x += awesomeconf[screen].statusbar.txtlayoutwidth;
            if(((awesomeconf[screen].statusbar.position == BarTop
                 || awesomeconf[screen].statusbar.position == BarBot)
                && ev->x < x)
                || (awesomeconf[screen].statusbar.position == BarRight && ev->y < x)
                || (awesomeconf[screen].statusbar.position == BarLeft && ev->y > awesomeconf[screen].statusbar.width - x))
                handle_mouse_button_press(&awesomeconf[screen], ev->button, ev->state,
                                          awesomeconf[screen].buttons.layout, NULL);
            else
                handle_mouse_button_press(&awesomeconf[screen], ev->button, ev->state,
                                          awesomeconf[screen].buttons.title, NULL);
            return;
        }

    if((c = get_client_bywin(*awesomeconf->clients, ev->window)))
    {
        XAllowEvents(c->display, ReplayPointer, CurrentTime);
        focus(c, ev->same_screen, &awesomeconf[c->screen]);
        if(CLEANMASK(ev->state, awesomeconf[c->screen]) != awesomeconf[c->screen].modkey)
        {
           if (ev->button == Button1)
           {
               restack(&awesomeconf[c->screen]);
               window_grabbuttons(c->display, c->phys_screen, c->win,
                                  True, True, awesomeconf->buttons.root,
                                  awesomeconf->modkey, awesomeconf->numlockmask);
           }
        }
        else if(ev->button == Button1)
            uicb_movemouse(&awesomeconf[c->screen], NULL);
        else if(ev->button == Button2)
        {
            if((get_current_layout(awesomeconf[c->screen].tags,
                                   awesomeconf[c->screen].ntags)->arrange != layout_floating)
                && !c->isfixed && c->isfloating)
                uicb_togglefloating(&awesomeconf[c->screen], NULL);
            else
                uicb_zoom(&awesomeconf[c->screen], NULL);
        }
        else if(ev->button == Button3)
            uicb_resizemouse(&awesomeconf[c->screen], NULL);
        else if(ev->button == Button4)
            uicb_settrans(&awesomeconf[c->screen], "+5");
        else if(ev->button == Button5)
            uicb_settrans(&awesomeconf[c->screen], "-5");
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(RootWindow(e->xany.display, screen) == ev->window
               && XQueryPointer(e->xany.display, ev->window, &wdummy, &wdummy, &x, &y, &i, &i, &udummy))
            {
                screen = get_screen_bycoord(e->xany.display, x, y);
                handle_mouse_button_press(&awesomeconf[screen], ev->button, ev->state,
                                          awesomeconf[screen].buttons.root, NULL);
                return;
            }
}

void
handle_event_configurerequest(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if((c = get_client_bywin(*awesomeconf->clients, ev->window)))
    {
        c->ismax = False;
        if(ev->value_mask & CWBorderWidth)
            c->border = ev->border_width;
        if(c->isfixed || c->isfloating
           || get_current_layout(awesomeconf[c->screen].tags, 
                                 awesomeconf[c->screen].ntags)->arrange == layout_floating)
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
                window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);
            if(isvisible(c, c->screen, awesomeconf[c->screen].tags, awesomeconf[c->screen].ntags))
                XMoveResizeWindow(e->xany.display, c->win, c->x, c->y, c->w, c->h);
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
            si = get_screen_info(e->xany.display, screen, NULL);
            awesomeconf[screen].statusbar.width = si[screen].width;
            p_delete(&si);

            XResizeWindow(e->xany.display,
                          awesomeconf[screen].statusbar.window,
                          awesomeconf[screen].statusbar.width,
                          awesomeconf[screen].statusbar.height);

            updatebarpos(e->xany.display, awesomeconf[screen].statusbar);
            arrange(&awesomeconf[screen]);
        }
}

void
handle_event_destroynotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = get_client_bywin(*awesomeconf->clients, ev->window)))
        client_unmanage(c, WithdrawnState, &awesomeconf[c->screen]);
}

void
handle_event_enternotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = get_client_bywin(*awesomeconf->clients, ev->window)))
    {
        focus(c, ev->same_screen, &awesomeconf[c->screen]);
        if (c->isfloating
                || get_current_layout(awesomeconf[c->screen].tags,
                                    awesomeconf[c->screen].ntags)->arrange == layout_floating)
            window_grabbuttons(c->display, c->phys_screen, c->win,
                               True, False, awesomeconf->buttons.root,
                               awesomeconf->modkey, awesomeconf->numlockmask);
    }
    else
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            if(ev->window == RootWindow(e->xany.display, screen))
                focus(NULL, True, &awesomeconf[screen]);
}

void
handle_event_expose(XEvent * e, awesome_config *awesomeconf)
{
    XExposeEvent *ev = &e->xexpose;
    int screen;

    if(!ev->count)
        for(screen = 0; screen < get_screen_count(e->xany.display); screen++)
            if(awesomeconf[screen].statusbar.window == ev->window)
                drawstatusbar(&awesomeconf[screen]);
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
           && CLEANMASK(awesomeconf[screen].keys[i].mod, awesomeconf[screen])
           == CLEANMASK(ev->state, awesomeconf[screen]) && awesomeconf[screen].keys[i].func)
            awesomeconf[screen].keys[i].func(&awesomeconf[screen], awesomeconf[screen].keys[i].arg);
}

void
handle_event_leavenotify(XEvent * e, awesome_config *awesomeconf)
{
    XCrossingEvent *ev = &e->xcrossing;
    int screen;

    for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
        if((ev->window == RootWindow(e->xany.display, screen)) && !ev->same_screen)
            focus(NULL, ev->same_screen, &awesomeconf[screen]);
}

void
handle_event_mappingnotify(XEvent * e, awesome_config *awesomeconf)
{
    XMappingEvent *ev = &e->xmapping;
    int screen;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        for(screen = 0; screen < ScreenCount(e->xany.display); screen++)
            grabkeys(&awesomeconf[screen]);
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
    if(!get_client_bywin(*awesomeconf->clients, ev->window))
    {
        for(screen = 0; wa.screen != ScreenOfDisplay(e->xany.display, screen); screen++);
        if(screen == 0 && XQueryPointer(e->xany.display, RootWindow(e->xany.display, screen),
                                        &dummy, &dummy, &x, &y, &d, &d, &m))
            screen = get_screen_bycoord(e->xany.display, x, y);
        client_manage(ev->window, &wa, &awesomeconf[screen]);
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
    if((c = get_client_bywin(*awesomeconf->clients, ev->window)))
    {
        switch (ev->atom)
        {
          case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (get_client_bywin(*awesomeconf->clients, trans) != NULL)))
                arrange(&awesomeconf[c->screen]);
            break;
          case XA_WM_NORMAL_HINTS:
            updatesizehints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == XInternAtom(c->display, "_NET_WM_NAME", False))
        {
            updatetitle(c);
            if(c == get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel)
                drawstatusbar(&awesomeconf[c->screen]);
        }
    }
}

void
handle_event_unmapnotify(XEvent * e, awesome_config *awesomeconf)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = get_client_bywin(*awesomeconf->clients, ev->window))
       && ev->event == RootWindow(e->xany.display, c->phys_screen)
       && ev->send_event && window_getstate(c->display, c->win) == NormalState)
        client_unmanage(c, WithdrawnState, &awesomeconf[c->screen]);
}

void
handle_event_shape(XEvent * e,
                   awesome_config *awesomeconf __attribute__ ((unused)))
{
    XShapeEvent *ev = (XShapeEvent *) e;
    Client *c = get_client_bywin(*awesomeconf->clients, ev->window);

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
grabkeys(awesome_config *awesomeconf)
{
    int i;
    KeyCode code;

    XUngrabKey(awesomeconf->display, AnyKey, AnyModifier, RootWindow(awesomeconf->display, awesomeconf->phys_screen));
    for(i = 0; i < awesomeconf->nkeys; i++)
    {
        if((code = XKeysymToKeycode(awesomeconf->display, awesomeconf->keys[i].keysym)) == NoSymbol)
            continue;
        XGrabKey(awesomeconf->display, code, awesomeconf->keys[i].mod, RootWindow(awesomeconf->display, awesomeconf->phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, awesomeconf->keys[i].mod | LockMask, RootWindow(awesomeconf->display, awesomeconf->phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask, RootWindow(awesomeconf->display, awesomeconf->phys_screen), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(awesomeconf->display, code, awesomeconf->keys[i].mod | awesomeconf->numlockmask | LockMask, RootWindow(awesomeconf->display, awesomeconf->phys_screen), True,
                 GrabModeAsync, GrabModeAsync);
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
