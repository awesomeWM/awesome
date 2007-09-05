/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "jdwm.h"
#include "util.h"
#include "event.h"
#include "layout.h"
#include "tag.h"
#include "layouts/tile.h"
#include "layouts/floating.h"

/* extern */
extern int screen, sw, sh;      /* screen geometry */
extern int wax, way, wah, waw;  /* windowarea geometry */
extern int bh, blw;             /* bar height, bar layout label width */
extern Window barwin;
extern DC dc;                   /* global draw context */
extern Cursor cursor[CurLast];
extern Client *clients, *sel;   /* global client list */
extern Bool selscreen;
extern Atom netatom[NetLast];
void (*handler[LASTEvent]) (XEvent *, jdwm_config *);   /* event handler */

#define CLEANMASK(mask)		(mask & ~(jdwmconf->numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)

static Client *
getclient(Window w)
{
    Client *c;

    for(c = clients; c && c->win != w; c = c->next);
    return c;
}

static void
movemouse(Client * c, jdwm_config *jdwmconf)
{
    int x1, y1, ocx, ocy, di, nx, ny;
    unsigned int dui;
    Window dummy;
    XEvent ev;

    ocx = nx = c->x;
    ocy = ny = c->y;
    if(XGrabPointer(c->display, DefaultRootWindow(c->display), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, cursor[CurMove], CurrentTime) != GrabSuccess)
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
            handler[ev.type] (&ev, jdwmconf);
            break;
        case MotionNotify:
            XSync(c->display, False);
            nx = ocx + (ev.xmotion.x - x1);
            ny = ocy + (ev.xmotion.y - y1);
            if(abs(wax + nx) < jdwmconf->snap)
                nx = wax;
            else if(abs((wax + waw) - (nx + c->w + 2 * c->border)) < jdwmconf->snap)
                nx = wax + waw - c->w - 2 * c->border;
            if(abs(way - ny) < jdwmconf->snap)
                ny = way;
            else if(abs((way + wah) - (ny + c->h + 2 * c->border)) < jdwmconf->snap)
                ny = way + wah - c->h - 2 * c->border;
            resize(c, nx, ny, c->w, c->h, False);
            break;
        }
    }
}

static void
resizemouse(Client * c, jdwm_config *jdwmconf)
{
    int ocx, ocy;
    int nw, nh;
    XEvent ev;

    ocx = c->x;
    ocy = c->y;
    if(XGrabPointer(c->display, DefaultRootWindow(c->display), False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    None, cursor[CurResize], CurrentTime) != GrabSuccess)
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
            handler[ev.type] (&ev, jdwmconf);
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

static void
buttonpress(XEvent * e, jdwm_config *jdwmconf)
{
    int i, x;
    Client *c;
    XButtonPressedEvent *ev = &e->xbutton;

    if(barwin == ev->window)
    {
        x = 0;
        for(i = 0; i < jdwmconf->ntags; i++)
        {
            x += textw(jdwmconf->tags[i]);
            if(ev->x < x)
            {
                if(ev->button == Button1)
                {
                    if(ev->state & jdwmconf->modkey)
                        uicb_tag(e->xany.display, jdwmconf, jdwmconf->tags[i]);
                    else
                        uicb_view(e->xany.display, jdwmconf, jdwmconf->tags[i]);
                }
                else if(ev->button == Button3)
                {
                    if(ev->state & jdwmconf->modkey)
                        uicb_toggletag(e->xany.display, jdwmconf, jdwmconf->tags[i]);
                    else
                        uicb_toggleview(e->xany.display, jdwmconf, jdwmconf->tags[i]);
                }
                return;
            }
        }
        if((ev->x < x + blw) && ev->button == Button1)
            uicb_setlayout(e->xany.display, jdwmconf, NULL);
    }
    else if((c = getclient(ev->window)))
    {
        focus(c->display, &dc, c, jdwmconf);
        if(CLEANMASK(ev->state) != jdwmconf->modkey)
            return;
        if(ev->button == Button1 && (IS_ARRANGE(floating) || c->isfloating))
        {
            restack(e->xany.display, jdwmconf);
            movemouse(c, jdwmconf);
        }
        else if(ev->button == Button2)
            uicb_zoom(e->xany.display, jdwmconf, NULL);
        else if(ev->button == Button3 && (IS_ARRANGE(floating) || c->isfloating) && !c->isfixed)
        {
            restack(e->xany.display, jdwmconf);
            resizemouse(c, jdwmconf);
        }
    }
    else if(DefaultRootWindow(e->xany.display) == ev->window && !sel)
    {
        if(ev->button == Button4)
            uicb_tag_viewnext(e->xany.display, jdwmconf, NULL);
        else if(ev->button == Button5)
            uicb_tag_viewprev(e->xany.display, jdwmconf, NULL);
    }
}

static void
configurerequest(XEvent * e, jdwm_config *jdwmconf __attribute__ ((unused)))
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
            if((c->x + c->w) > sw && c->isfloating)
                c->x = sw / 2 - c->w / 2;       /* center in x direction */
            if((c->y + c->h) > sh && c->isfloating)
                c->y = sh / 2 - c->h / 2;       /* center in y direction */
            if((ev->value_mask & (CWX | CWY)) && !(ev->value_mask & (CWWidth | CWHeight)))
                configure(c);
            if(isvisible(c, jdwmconf->ntags))
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

static void
configurenotify(XEvent * e, jdwm_config *jdwmconf)
{
    XConfigureEvent *ev = &e->xconfigure;

    if(ev->window == DefaultRootWindow(e->xany.display) && (ev->width != sw || ev->height != sh))
    {
        sw = ev->width;
        sh = ev->height;
        XFreePixmap(e->xany.display, dc.drawable);
        dc.drawable = XCreatePixmap(e->xany.display, DefaultRootWindow(e->xany.display), sw, bh, DefaultDepth(e->xany.display, screen));
        XResizeWindow(e->xany.display, barwin, sw, bh);
        updatebarpos(e->xany.display);
        arrange(e->xany.display, jdwmconf);
    }
}

static void
destroynotify(XEvent * e, jdwm_config *jdwmconf)
{
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if((c = getclient(ev->window)))
        unmanage(c, &dc, WithdrawnState, jdwmconf);
}

static void
enternotify(XEvent * e, jdwm_config *jdwmconf)
{
    Client *c;
    XCrossingEvent *ev = &e->xcrossing;

    if(ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    if((c = getclient(ev->window)))
        focus(c->display, &dc, c, jdwmconf);
    else if(ev->window == DefaultRootWindow(e->xany.display))
    {
        selscreen = True;
        focus(e->xany.display, &dc, NULL, jdwmconf);
    }
}

static void
expose(XEvent * e, jdwm_config *jdwmconf)
{
    XExposeEvent *ev = &e->xexpose;

    if(!ev->count && barwin == ev->window)
        drawstatus(e->xany.display, jdwmconf);
}

static void
keypress(XEvent * e, jdwm_config *jdwmconf)
{
    int i;
    KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    keysym = XKeycodeToKeysym(e->xany.display, (KeyCode) ev->keycode, 0);
    for(i = 0; i < jdwmconf->nkeys; i++)
        if(keysym == jdwmconf->keys[i].keysym
           && CLEANMASK(jdwmconf->keys[i].mod) == CLEANMASK(ev->state) && jdwmconf->keys[i].func)
            jdwmconf->keys[i].func(e->xany.display, jdwmconf, jdwmconf->keys[i].arg);
}

static void
leavenotify(XEvent * e, jdwm_config *jdwmconf)
{
    XCrossingEvent *ev = &e->xcrossing;

    if((ev->window == DefaultRootWindow(e->xany.display)) && !ev->same_screen)
    {
        selscreen = False;
        focus(e->xany.display, &dc, NULL, jdwmconf);
    }
}

static void
mappingnotify(XEvent * e, jdwm_config *jdwmconf)
{
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if(ev->request == MappingKeyboard)
        grabkeys(e->xany.display, jdwmconf);
}

static void
maprequest(XEvent * e, jdwm_config *jdwmconf)
{
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;

    if(!XGetWindowAttributes(e->xany.display, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;
    if(!getclient(ev->window))
        manage(e->xany.display, &dc, ev->window, &wa, jdwmconf);
}

static void
propertynotify(XEvent * e, jdwm_config *jdwmconf)
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
        default:
            break;
        case XA_WM_TRANSIENT_FOR:
            XGetTransientForHint(e->xany.display, c->win, &trans);
            if(!c->isfloating && (c->isfloating = (getclient(trans) != NULL)))
                arrange(e->xany.display, jdwmconf);
            break;
        case XA_WM_NORMAL_HINTS:
            updatesizehints(c);
            break;
        }
        if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName])
        {
            updatetitle(c);
            if(c == sel)
                drawstatus(e->xany.display, jdwmconf);
        }
    }
}

static void
unmapnotify(XEvent * e, jdwm_config *jdwmconf)
{
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if((c = getclient(ev->window)) && ev->event == DefaultRootWindow(e->xany.display) && (ev->send_event || !c->unmapped--))
        unmanage(c, &dc, WithdrawnState, jdwmconf);
}

/* extern */

void (*handler[LASTEvent]) (XEvent *, jdwm_config *) =
{
[ButtonPress] = buttonpress,
        [ConfigureRequest] = configurerequest,
        [ConfigureNotify] = configurenotify,
        [DestroyNotify] = destroynotify,
        [EnterNotify] = enternotify,
        [LeaveNotify] = leavenotify,
        [Expose] = expose,
        [KeyPress] = keypress,
        [MappingNotify] = mappingnotify,
        [MapRequest] = maprequest,[PropertyNotify] = propertynotify,[UnmapNotify] = unmapnotify};

void
grabkeys(Display *disp, jdwm_config *jdwmconf)
{
    int i;
    KeyCode code;

    XUngrabKey(disp, AnyKey, AnyModifier, DefaultRootWindow(disp));
    for(i = 0; i < jdwmconf->nkeys; i++)
    {
        code = XKeysymToKeycode(disp, jdwmconf->keys[i].keysym);
        XGrabKey(disp, code, jdwmconf->keys[i].mod, DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, jdwmconf->keys[i].mod | LockMask, DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, jdwmconf->keys[i].mod | jdwmconf->numlockmask, DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
        XGrabKey(disp, code, jdwmconf->keys[i].mod | jdwmconf->numlockmask | LockMask, DefaultRootWindow(disp), True,
                 GrabModeAsync, GrabModeAsync);
    }
}
