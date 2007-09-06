/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "jdwm.h"
#include "util.h"
#include "layout.h"
#include "tag.h"

#include "layouts/floating.h"

/* extern */
extern int sx, sy, sw, sh;      /* screen geometry */
extern int wax, way, wah, waw;  /* windowarea geometry */
extern Client *clients, *sel, *stack;   /* global client list and stack */
extern Bool selscreen;
extern Atom jdwmprops, wmatom[WMLast], netatom[NetLast];

/** Attach client stack to clients stacks
 * \param c the client
 */ 
static inline void
attachstack(Client * c)
{
    c->snext = stack;
    stack = c;
}

/** Detach client from stack
 * \param c the client
 */
static inline void
detachstack(Client * c)
{
    Client **tc;

    for(tc = &stack; *tc && *tc != c; tc = &(*tc)->snext);
    *tc = c->snext;
}

/** Grab or ungrab buttons when a client is focused
 * \param c client
 * \param focused True if client is focused
 * \param modkey Mod key mask
 * \param numlockmask Numlock mask
 */
static void
grabbuttons(Client * c, Bool focused, KeySym modkey, unsigned int numlockmask)
{
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);

    if(focused)
    {
        XGrabButton(c->display, Button1, modkey, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button1, modkey | LockMask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button1, modkey | numlockmask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button1, modkey | numlockmask | LockMask,
                    c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button2, modkey, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button2, modkey | LockMask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button2, modkey | numlockmask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button2, modkey | numlockmask | LockMask,
                    c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button3, modkey, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button3, modkey | LockMask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button3, modkey | numlockmask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button3, modkey | numlockmask | LockMask,
                    c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);

        XUngrabButton(c->display, AnyButton, AnyModifier, DefaultRootWindow(c->display));
    }
    else
    {
        XGrabButton(c->display, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, NoSymbol, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, LockMask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, numlockmask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, numlockmask | LockMask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button5, NoSymbol, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, LockMask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, numlockmask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, numlockmask | LockMask, DefaultRootWindow(c->display), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

    }
}

/** XXX: No idea
 * \param c the client
 * \return True if atom has WMDelete
 */
static Bool
isprotodel(Client * c)
{
    int i, n;
    Atom *protocols;
    Bool ret = False;

    if(XGetWMProtocols(c->display, c->win, &protocols, &n))
    {
        for(i = 0; !ret && i < n; i++)
            if(protocols[i] == wmatom[WMDelete])
                ret = True;
        XFree(protocols);
    }
    return ret;
}

/** XXX: No idea
 * \param c the client
 * \param state no idea
 */
static void
setclientstate(Client * c, long state)
{
    long data[] = { state, None };

    XChangeProperty(c->display, c->win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *) data, 2);
}

/* extern */

/** Attach client to the beginning of the clients stack
 * \param c the client
 */
inline void
attach(Client * c)
{
    if(clients)
        clients->prev = c;
    c->next = clients;
    clients = c;
}

inline void
updatetitle(Client * c)
{
    if(!gettextprop(c->display, c->win, netatom[NetWMName], c->name, sizeof c->name))
        gettextprop(c->display, c->win, wmatom[WMName], c->name, sizeof c->name);
}

/** Ban client
 * \param c the client
 */
void
ban(Client * c)
{
    if(c->isbanned)
        return;
    XUnmapWindow(c->display, c->win);
    setclientstate(c, IconicState);
    c->isbanned = True;
    c->unmapped++;
}


/** Configure client
 * \param c the client
 */
void
configure(Client * c)
{
    XConfigureEvent ce;

    ce.type = ConfigureNotify;
    ce.display = c->display;
    ce.event = c->win;
    ce.window = c->win;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->w;
    ce.height = c->h;
    ce.border_width = c->border;
    ce.above = None;
    ce.override_redirect = False;
    XSendEvent(c->display, c->win, False, StructureNotifyMask, (XEvent *) & ce);
}

void
detach(Client * c)
{
    if(c->prev)
        c->prev->next = c->next;
    if(c->next)
        c->next->prev = c->prev;
    if(c == clients)
        clients = c->next;
    c->next = c->prev = NULL;
}

/** Give focus to client, or to first client if c is NULL
 * \param disp Display ref
 * \param drawcontext drawcontext ref
 * \param c client
 * \param jdwmconf jdwm config
 */
void
focus(Display *disp, DC *drawcontext, Client * c, jdwm_config *jdwmconf)
{
    /* if c is NULL or invisible, take next client in the stack */
    if((!c && selscreen) || (c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags)))
        for(c = stack; c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags); c = c->snext);
    
    /* if a client was selected but it's not the current client, unfocus it */
    if(sel && sel != c)
    {
        grabbuttons(sel, False, jdwmconf->modkey, jdwmconf->numlockmask);
        XSetWindowBorder(sel->display, sel->win, drawcontext->norm[ColBorder]);
        setclienttrans(sel, jdwmconf->opacity_unfocused, 0);
    }
    if(c)
    {
        detachstack(c);
        attachstack(c);
        grabbuttons(c, True, jdwmconf->modkey, jdwmconf->numlockmask);
    }
    if(!selscreen)
        return;
    sel = c;
    drawstatus(disp, jdwmconf);
    if(sel)
    {
        XSetWindowBorder(sel->display, sel->win, drawcontext->sel[ColBorder]);
        XSetInputFocus(sel->display, sel->win, RevertToPointerRoot, CurrentTime);
        for(c = stack; c; c = c->snext)
            if(c != sel)
                setclienttrans(c, jdwmconf->opacity_unfocused, 0);
        setclienttrans(sel, -1, 0);
    }
    else
        XSetInputFocus(disp, DefaultRootWindow(disp), RevertToPointerRoot, CurrentTime);
}

/** Kill selected client
 * \param disp Display ref
 * \param jdwmconf jdwm config
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_killclient(Display *disp __attribute__ ((unused)),
                jdwm_config *jdwmconf __attribute__ ((unused)),
                const char *arg __attribute__ ((unused)))
{
    XEvent ev;

    if(!sel)
        return;
    if(isprotodel(sel))
    {
        ev.type = ClientMessage;
        ev.xclient.window = sel->win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = wmatom[WMDelete];
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(sel->display, sel->win, False, NoEventMask, &ev);
    }
    else
        XKillClient(sel->display, sel->win);
}

static Bool
loadprops(Client * c, int ntags)
{
    int i;
    char prop[128];
    Bool result = False;

    if(gettextprop(c->display, c->win, jdwmprops, prop, sizeof(prop)))
    {
        for(i = 0; i < ntags && i < ssizeof(prop) - 1 && prop[i] != '\0'; i++)
            if((c->tags[i] = prop[i] == '1'))
                result = True;
        if(i < ssizeof(prop) - 1 && prop[i] != '\0')
            c->isfloating = prop[i] == '1';
    }
    return result;
}

void
manage(Display * disp, DC *drawcontext, Window w, XWindowAttributes * wa, jdwm_config *jdwmconf)
{
    int i;
    Client *c, *t = NULL;
    Window trans;
    Status rettrans;
    XWindowChanges wc;

    c = p_new(Client, 1);
    c->tags = p_new(Bool, jdwmconf->ntags);
    c->win = w;
    c->ftview = True;
    c->x = c->rw = wa->x;
    c->y = c->ry = wa->y;
    c->w = c->rw = wa->width;
    c->h = c->rh = wa->height;
    c->oldborder = wa->border_width;
    c->display = disp;
    if(c->w == sw && c->h == sh)
    {
        c->x = sx;
        c->y = sy;
        c->border = wa->border_width;
    }
    else
    {
        if(c->x + c->w + 2 * c->border > wax + waw)
            c->x = c->rx = wax + waw - c->w - 2 * c->border;
        if(c->y + c->h + 2 * c->border > way + wah)
            c->y = c->ry = way + wah - c->h - 2 * c->border;
        if(c->x < wax)
            c->x = c->rx = wax;
        if(c->y < way)
            c->y = c->ry = way;
        c->border = jdwmconf->borderpx;
    }
    wc.border_width = c->border;
    XConfigureWindow(disp, w, CWBorderWidth, &wc);
    XSetWindowBorder(disp, w, drawcontext->norm[ColBorder]);
    configure(c);               /* propagates border_width, if size doesn't change */
    updatesizehints(c);
    XSelectInput(disp, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
    grabbuttons(c, False, jdwmconf->modkey, jdwmconf->numlockmask);
    updatetitle(c);
    if((rettrans = XGetTransientForHint(disp, w, &trans) == Success))
        for(t = clients; t && t->win != trans; t = t->next);
    if(t)
        for(i = 0; i < jdwmconf->ntags; i++)
            c->tags[i] = t->tags[i];
    if(!loadprops(c, jdwmconf->ntags))
        applyrules(c, jdwmconf);
    if(!c->isfloating)
        c->isfloating = (rettrans == Success) || c->isfixed;
    saveprops(c, jdwmconf->ntags);
    attach(c);
    attachstack(c);
    XMoveResizeWindow(disp, c->win, c->x, c->y, c->w, c->h);     /* some windows require this */
    ban(c);
    arrange(disp, jdwmconf);
}

void
resize(Client * c, int x, int y, int w, int h, Bool sizehints)
{
    double dx, dy, max, min, ratio;
    XWindowChanges wc;

    if(sizehints)
    {
        if(c->minay > 0 && c->maxay > 0 && (h - c->baseh) > 0 && (w - c->basew) > 0)
        {
            dx = (double) (w - c->basew);
            dy = (double) (h - c->baseh);
            min = (double) (c->minax) / (double) (c->minay);
            max = (double) (c->maxax) / (double) (c->maxay);
            ratio = dx / dy;
            if(max > 0 && min > 0 && ratio > 0)
            {
                if(ratio < min)
                {
                    dy = (dx * min + dy) / (min * min + 1);
                    dx = dy * min;
                    w = (int) dx + c->basew;
                    h = (int) dy + c->baseh;
                }
                else if(ratio > max)
                {
                    dy = (dx * min + dy) / (max * max + 1);
                    dx = dy * min;
                    w = (int) dx + c->basew;
                    h = (int) dy + c->baseh;
                }
            }
        }
        if(c->minw && w < c->minw)
            w = c->minw;
        if(c->minh && h < c->minh)
            h = c->minh;
        if(c->maxw && w > c->maxw)
            w = c->maxw;
        if(c->maxh && h > c->maxh)
            h = c->maxh;
        if(c->incw)
            w -= (w - c->basew) % c->incw;
        if(c->inch)
            h -= (h - c->baseh) % c->inch;
    }
    if(w <= 0 || h <= 0)
        return;
    /* offscreen appearance fixes */
    if(x > sw)
        x = sw - w - 2 * c->border;
    if(y > sh)
        y = sh - h - 2 * c->border;
    if(x + w + 2 * c->border < sx)
        x = sx;
    if(y + h + 2 * c->border < sy)
        y = sy;
    if(c->x != x || c->y != y || c->w != w || c->h != h)
    {
        c->x = wc.x = x;
        c->y = wc.y = y;
        c->w = wc.width = w;
        c->h = wc.height = h;
        wc.border_width = c->border;
        XConfigureWindow(c->display, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
        configure(c);
        XSync(c->display, False);
    }
}

void
uicb_moveresize(Display *disp __attribute__ ((unused)),
                jdwm_config *jdwmconf,
                const char *arg)
{
    int x, y, w, h, nx, ny, nw, nh, ox, oy, ow, oh;
    char xabs, yabs, wabs, habs;
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;

    if(!IS_ARRANGE(floating))
        if(!sel || !sel->isfloating || sel->isfixed || !arg)
            return;
    if(sscanf(arg, "%d%c %d%c %d%c %d%c", &x, &xabs, &y, &yabs, &w, &wabs, &h, &habs) != 8)
        return;
    nx = xabs == 'x' ? sel->x + x : x;
    ny = yabs == 'y' ? sel->y + y : y;
    nw = wabs == 'w' ? sel->w + w : w;
    nh = habs == 'h' ? sel->h + h : h;

    ox = sel->x;
    oy = sel->y;
    ow = sel->w;
    oh = sel->h;

    Bool xqp = XQueryPointer(sel->display, DefaultRootWindow(sel->display), &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    resize(sel, nx, ny, nw, nh, True);
    if (xqp && ox <= mx && (ox + ow) >= mx && oy <= my && (oy + oh) >= my)
    {
        nmx = mx-ox+sel->w-ow-1 < 0 ? 0 : mx-ox+sel->w-ow-1;
        nmy = my-oy+sel->h-oh-1 < 0 ? 0 : my-oy+sel->h-oh-1;
        XWarpPointer(sel->display, None, sel->win, 0, 0, 0, 0, nmx, nmy);
    }
}

void
saveprops(Client * c, int ntags)
{
    int i;
    char prop[128];

    for(i = 0; i < ntags && i < ssizeof(prop) - 1; i++)
        prop[i] = c->tags[i] ? '1' : '0';
    if(i < ssizeof(prop) - 1)
        prop[i++] = c->isfloating ? '1' : '0';

    prop[i] = '\0';
    XChangeProperty(c->display, c->win, jdwmprops, XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
}

void
unban(Client * c)
{
    if(!c->isbanned)
        return;
    XMapWindow(c->display, c->win);
    setclientstate(c, NormalState);
    c->isbanned = False;
}

void
unmanage(Client * c, DC *drawcontext, long state, jdwm_config *jdwmconf)
{
    XWindowChanges wc;

    wc.border_width = c->oldborder;
    /* The server grab construct avoids race conditions. */
    XGrabServer(c->display);
    XConfigureWindow(c->display, c->win, CWBorderWidth, &wc);  /* restore border */
    detach(c);
    detachstack(c);
    if(sel == c)
        focus(c->display, drawcontext, NULL, jdwmconf);
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);
    setclientstate(c, state);
    XSync(c->display, False);
    XSetErrorHandler(xerror);
    XUngrabServer(c->display);
    if(state != NormalState)
        arrange(c->display, jdwmconf);
    p_delete(&c->tags);
    p_delete(&c);
}

void
updatesizehints(Client * c)
{
    long msize;
    XSizeHints size;

    if(!XGetWMNormalHints(c->display, c->win, &size, &msize) || !size.flags)
        size.flags = PSize;
    c->flags = size.flags;
    if(c->flags & PBaseSize)
    {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    }
    else if(c->flags & PMinSize)
    {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    }
    else
        c->basew = c->baseh = 0;
    if(c->flags & PResizeInc)
    {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    }
    else
        c->incw = c->inch = 0;

    if(c->flags & PMaxSize)
    {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    }
    else
        c->maxw = c->maxh = 0;

    if(c->flags & PMinSize)
    {
        c->minw = size.min_width;
        c->minh = size.min_height;
    }
    else if(c->flags & PBaseSize)
    {
        c->minw = size.base_width;
        c->minh = size.base_height;
    }
    else
        c->minw = c->minh = 0;

    if(c->flags & PAspect)
    {
        c->minax = size.min_aspect.x;
        c->maxax = size.max_aspect.x;
        c->minay = size.min_aspect.y;
        c->maxay = size.max_aspect.y;
    }
    else
        c->minax = c->maxax = c->minay = c->maxay = 0;

    c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
                  && c->maxw == c->minw && c->maxh == c->minh);
}

void
setclienttrans(Client *c, double opacity, unsigned int current_opacity)
{
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 100)
    {
        real_opacity = ((opacity / 100.0) * 0xffffffff) + current_opacity;
        XChangeProperty(c->display, c->win,
                        XInternAtom(c->display, "_NET_WM_WINDOW_OPACITY", False),
                        XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &real_opacity, 1L);
    }
    else
        XDeleteProperty(c->display, c->win, XInternAtom(c->display, "_NET_WM_WINDOW_OPACITY", False));

    XSync(c->display, False);
}

void
uicb_settrans(Display *disp __attribute__ ((unused)),
              jdwm_config *jdwmconf __attribute__ ((unused)),
              const char *arg)
{
    unsigned int current_opacity = 0;
    double delta = 100.0;
    unsigned char *data;
    Atom actual;
    int format;
    unsigned long n, left;
    int set_prop = 0;

    if(!sel)
        return;

    if(arg && sscanf(arg, "%lf", &delta))
    {
        if(arg[0] == '+' || arg[0] == '-')
        {
            XGetWindowProperty(sel->display, sel->win, XInternAtom(sel->display, "_NET_WM_WINDOW_OPACITY", False),
                               0L, 1L, False, XA_CARDINAL, &actual, &format, &n, &left,
                               (unsigned char **) &data);
            if(data)
            {
                memcpy(&current_opacity, data, sizeof(unsigned int));
                XFree((void *) data);
                delta += ((current_opacity * 100.0) / 0xffffffff);
            }
            else
            {
                delta += 100.0;
                set_prop = 1;
            }

        }
    }

    if(delta <= 0.0)
        delta = 0.0;
    else if(delta > 100.0)
    {
        delta = 100.0;
        set_prop = 1;
    }

    if(delta == 100.0 && !set_prop)
        setclienttrans(sel, -1, 0);
    else
        setclienttrans(sel, delta, current_opacity);
}
