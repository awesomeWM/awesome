/*
 * client.c - client management
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

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include "screen.h"
#include "tab.h"
#include "awesome.h"
#include "layout.h"
#include "tag.h"
#include "util.h"
#include "statusbar.h"
#include "layouts/floating.h"

Client *
get_client_bywin(Client **list, Window w)
{
    Client *c;

    for(c = *list; c && c->win != w; c = c->next);
    return c;
}

/** Grab or ungrab buttons when a client is focused
 * \param c client
 * \param focused True if client is focused
 * \param raised True if the client is above other clients
 * \param modkey Mod key mask
 * \param numlockmask Numlock mask
 */
void
grabbuttons(Client * c, Bool focused, Bool raised, KeySym modkey, unsigned int numlockmask)
{
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);

    if(focused)
    {
        if(!raised)
        {
            XGrabButton(c->display, Button1, NoSymbol, c->win, False,
                        BUTTONMASK, GrabModeSync, GrabModeAsync, None, None);
        }

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

        XGrabButton(c->display, Button4, modkey, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, modkey | LockMask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, modkey | numlockmask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, modkey | numlockmask | LockMask,
                    c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button5, modkey, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, modkey | LockMask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, modkey | numlockmask, c->win, False,
                    BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, modkey | numlockmask | LockMask,
                    c->win, False, BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
        
        XUngrabButton(c->display, AnyButton, AnyModifier, RootWindow(c->display, c->phys_screen));
    }
    else
    {
        XGrabButton(c->display, AnyButton, AnyModifier, c->win, False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button4, NoSymbol, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, LockMask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, numlockmask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button4, numlockmask | LockMask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

        XGrabButton(c->display, Button5, NoSymbol, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, LockMask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, numlockmask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);
        XGrabButton(c->display, Button5, numlockmask | LockMask, RootWindow(c->display, c->phys_screen), False, BUTTONMASK,
                    GrabModeAsync, GrabModeSync, None, None);

    }
}

/** Check if client supports protocol WM_DELETE_WINDOW
 * \param c the client
 * \return True if client has WM_DELETE_WINDOW
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
            if(protocols[i] == XInternAtom(c->display, "WM_DELETE_WINDOW", False))
                ret = True;
        XFree(protocols);
    }
    return ret;
}

/** Set client WM_STATE property
 * \param c the client
 * \param state no idea
 */
static void
setclientstate(Client * c, long state)
{
    long data[] = { state, None };

    XChangeProperty(c->display, c->win, XInternAtom(c->display, "WM_STATE", False),
                    XInternAtom(c->display, "WM_STATE", False),  32,
                    PropModeReplace, (unsigned char *) data, 2);
}

/** Set client transparency using composite
 * \param c client
 * \param opacity opacity percentage
 */
static void
setclienttrans(Client *c, double opacity)
{
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 100)
    {
        real_opacity = ((opacity / 100.0) * 0xffffffff);
        XChangeProperty(c->display, c->win,
                        XInternAtom(c->display, "_NET_WM_WINDOW_OPACITY", False),
                        XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &real_opacity, 1L);
    }
    else
        XDeleteProperty(c->display, c->win, XInternAtom(c->display, "_NET_WM_WINDOW_OPACITY", False));

    XSync(c->display, False);
}

/** Swap two client in the linked list clients
 * \param c1 first client
 * \param c2 second client
 */
static void
client_swap(Client **head, Client *c1, Client *c2)
{
    Client *tmp;

    tmp = c1->next;
    c1->next = c2->next;
    c2->next = (tmp == c2 ? c1 : tmp);

    tmp = c2->prev;
    c2->prev = c1->prev;
    c1->prev = (tmp == c1 ? c2 : tmp );

    if(c1->next)
        c1->next->prev = c1;

    if(c1->prev)
        c1->prev->next = c1;

    if(c2->next)
        c2->next->prev = c2;

    if(c2->prev)
        c2->prev->next = c2;

    if(*head == c1)
        *head = c2;
}

/** Attach client to the beginning of the clients stack
 * \param c the client
 */
void
attach(Client **head, Client *c)
{
    if(*head)
        (*head)->prev = c;
    c->next = *head;
    *head = c;
}

void
updatetitle(Client *c)
{
    if(!xgettextprop(c->display, c->win, XInternAtom(c->display, "_NET_WM_NAME", False), c->name, sizeof(c->name)))
        xgettextprop(c->display, c->win, XInternAtom(c->display, "WM_NAME", False), c->name, sizeof(c->name));
}

/** Ban client and unmapped it
 * \param c the client
 */
void
ban(Client * c)
{
    XUnmapWindow(c->display, c->win);
    setclientstate(c, IconicState);
    c->isbanned = True;
    c->unmapped = True;
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

/** Detach client from clients list
 * \param c client to detach
 */
void
detach(Client **head, Client *c)
{
    if(c->prev)
        c->prev->next = c->next;
    if(c->next)
        c->next->prev = c->prev;
    if(c == *head)
        *head = c->next;
    c->next = c->prev = NULL;
}

/** Give focus to client, or to first client if c is NULL
 * \param disp Display ref
 * \param c client
 * \param selscreen True if current screen is selected
 * \param awesomeconf awesome config
 */
void
focus(Client * c, Bool selscreen, awesome_config *awesomeconf)
{
    /* if c is NULL or invisible, take next client in the stack */
    if((!c && selscreen) || (c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags)))
        for(c = *awesomeconf->clients; c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); c = c->next);

    /* if a client was selected but it's not the current client, unfocus it */
    if(*awesomeconf->client_sel && *awesomeconf->client_sel != c)
    {
        grabbuttons(*awesomeconf->client_sel, False, True, awesomeconf->modkey, awesomeconf->numlockmask);
        XSetWindowBorder(awesomeconf->display, (*awesomeconf->client_sel)->win, awesomeconf->colors_normal[ColBorder].pixel);
        setclienttrans(*awesomeconf->client_sel, awesomeconf->opacity_unfocused);
    }
    if(c)
    {
        if(c->tab.next || c->tab.prev)
            XSetWindowBorder(awesomeconf->display, c->win, awesomeconf->colors_tab[ColBorder].pixel);
        else
            XSetWindowBorder(awesomeconf->display, c->win, awesomeconf->colors_selected[ColBorder].pixel);
    }
    if(*awesomeconf->client_sel == c)
        return;
    if(c)
        grabbuttons(c, True, True, awesomeconf->modkey, awesomeconf->numlockmask);
    if(!selscreen)
        return;
    *awesomeconf->client_sel = c;
    drawstatusbar(awesomeconf);
    if(*awesomeconf->client_sel)
    {
        XSetInputFocus(awesomeconf->display, (*awesomeconf->client_sel)->win, RevertToPointerRoot, CurrentTime);
        for(c = *awesomeconf->clients; c; c = c->next)
            if(c != *awesomeconf->client_sel)
                setclienttrans(c, awesomeconf->opacity_unfocused);
        setclienttrans(*awesomeconf->client_sel, -1);
    }
    else
        XSetInputFocus(awesomeconf->display, RootWindow(awesomeconf->display, awesomeconf->phys_screen), RevertToPointerRoot, CurrentTime);
}


/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param ntags tags number
 */
static Bool
loadprops(Client * c, int ntags)
{
    int i;
    char *prop;
    Bool result = False;

    prop = p_new(char, ntags + 2);

    if(xgettextprop(c->display, c->win, AWESOMEPROPS_ATOM(c->display), prop, ntags + 2))
    {
        for(i = 0; i < ntags && prop[i]; i++)
            if((c->tags[i] = prop[i] == '1'))
                result = True;
        if(i <= ntags && prop[i])
            c->isfloating = prop[i] == '1';
    }

    p_delete(&prop);

    return result;
}

/** Manage a new client
 * \param disp Display ref
 * \param w The window
 * \param wa Window attributes
 * \param awesomeconf awesome config
 */
void
manage(Display *disp, Window w, XWindowAttributes *wa, awesome_config *awesomeconf)
{
    int i;
    Client *c, *t = NULL;
    Window trans;
    Status rettrans;
    XWindowChanges wc;
    ScreenInfo *si = get_display_info(disp, awesomeconf->phys_screen, &awesomeconf->statusbar);
    ScreenInfo *screen_info;

    c = p_new(Client, 1);
    c->win = w;
    c->ftview = True;
    c->x = c->rw = wa->x;
    c->y = c->ry = wa->y;
    c->w = c->rw = wa->width;
    c->h = c->rh = wa->height;
    c->oldborder = wa->border_width;
    c->display = disp;
    c->phys_screen = get_phys_screen(c->display, c->screen);
    c->tab.isvisible = True;
    screen_info = get_screen_info(c->display, c->screen, NULL);
    if(c->w == screen_info[c->screen].width && c->h == screen_info[c->screen].height)
    {
        c->x = 0;
        c->y = 0;
        c->border = wa->border_width;
    }
    else
    {
        if(c->x + c->w + 2 * c->border > si->x_org + si->width)
            c->x = c->rx = si->x_org + si->width - c->w - 2 * c->border;
        if(c->y + c->h + 2 * c->border > si->y_org + si->height)
            c->y = c->ry = si->y_org + si->height - c->h - 2 * c->border;
        if(c->x < si->x_org)
            c->x = c->rx = si->x_org;
        if(c->y < si->y_org)
            c->y = c->ry = si->y_org;
        c->border = awesomeconf->borderpx;
    }
    p_delete(&si);
    wc.border_width = c->border;
    XConfigureWindow(disp, w, CWBorderWidth, &wc);
    XSetWindowBorder(disp, w, awesomeconf->colors_normal[ColBorder].pixel);
    configure(c);               /* propagates border_width, if size doesn't change */
    updatesizehints(c);
    XSelectInput(disp, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
    if(awesomeconf->have_shape)
    {
        XShapeSelectInput(disp, w, ShapeNotifyMask);
        set_shape(c);
    }
    grabbuttons(c, False, True, awesomeconf->modkey, awesomeconf->numlockmask);
    updatetitle(c);
    move_client_to_screen(c, awesomeconf, False);
    if((rettrans = XGetTransientForHint(disp, w, &trans) == Success))
        for(t = *awesomeconf->clients; t && t->win != trans; t = t->next);
    if(t)
        for(i = 0; i < awesomeconf->ntags; i++)
            c->tags[i] = t->tags[i];
    if(!loadprops(c, awesomeconf->ntags))
        applyrules(c, awesomeconf);
    if(!c->isfloating)
        c->isfloating = (rettrans == Success) || c->isfixed;
    saveprops(c, awesomeconf->ntags);
    attach(awesomeconf->clients, c);
    XMoveResizeWindow(disp, c->win, c->x, c->y, c->w, c->h);     /* some windows require this */
    c->isbanned = True;
    arrange(awesomeconf);
}

void
resize(Client *c, int x, int y, int w, int h, awesome_config *awesomeconf, Bool sizehints)
{
    double dx, dy, max, min, ratio;
    XWindowChanges wc;
    ScreenInfo *si;

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
    si = get_display_info(c->display, c->phys_screen, NULL);
    if(x > si->width)
        x = si->width - w - 2 * c->border;
    if(y > si->height)
        y = si->height - h - 2 * c->border;
    p_delete(&si);
    if(x + w + 2 * c->border < 0)
        x = 0;
    if(y + h + 2 * c->border < 0)
        y = 0;
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
        if(XineramaIsActive(c->display))
        {
            int new_screen = get_screen_bycoord(c->display, c->x, c->y);
            if(c->screen != new_screen)
                move_client_to_screen(c, &awesomeconf[new_screen - awesomeconf->screen], False);
        }
    }
}

void
saveprops(Client * c, int ntags)
{
    int i;
    char *prop;

    prop = p_new(char, ntags + 2);

    for(i = 0; i < ntags; i++)
        prop[i] = c->tags[i] ? '1' : '0';

    if(i <= ntags)
        prop[i] = c->isfloating ? '1' : '0';

    prop[++i] = '\0';

    XChangeProperty(c->display, c->win, AWESOMEPROPS_ATOM(c->display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);

    p_delete(&prop);
}

void
unban(Client * c)
{
    XMapWindow(c->display, c->win);
    setclientstate(c, NormalState);
    c->isbanned = False;
    c->unmapped = False;
}

void
unmanage(Client * c, long state, awesome_config *awesomeconf)
{
    XWindowChanges wc;

    client_untab(c);
    c->unmapped = True;
    wc.border_width = c->oldborder;
    /* The server grab construct avoids race conditions. */
    XGrabServer(c->display);
    XConfigureWindow(c->display, c->win, CWBorderWidth, &wc);  /* restore border */
    detach(awesomeconf->clients, c);
    if(*awesomeconf->client_sel == c)
        focus(NULL, True, awesomeconf);
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);
    setclientstate(c, state);
    XSync(c->display, False);
    XSetErrorHandler(xerror);
    XUngrabServer(c->display);
    if(state != NormalState)
        arrange(awesomeconf);
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
set_shape(Client *c)
{
    int bounding_shaped;
    int i, b;  unsigned int u;  /* dummies */
    /* Logic to decide if we have a shaped window cribbed from fvwm-2.5.10. */
    if (XShapeQueryExtents(c->display, c->win, &bounding_shaped, &i, &i,
                           &u, &u, &b, &i, &i, &u, &u) && bounding_shaped)
        XShapeCombineShape(c->display, RootWindow(c->display, c->phys_screen), ShapeBounding, 0, 0, c->win, ShapeBounding, ShapeSet);
}

/** Set selected client transparency
 * \param awesomeconf awesome config
 * \param arg unused arg
 * \ingroup ui_callback
 */
void
uicb_settrans(awesome_config *awesomeconf __attribute__ ((unused)),
              const char *arg)
{
    double delta = 100.0, current_opacity = 100.0;
    unsigned char *data;
    Atom actual;
    int format;
    unsigned long n, left;
    unsigned int current_opacity_raw = 0;
    int set_prop = 0;

    if(!*awesomeconf->client_sel)
        return;

    XGetWindowProperty(awesomeconf->display, (*awesomeconf->client_sel)->win,
                       XInternAtom((*awesomeconf->client_sel)->display, "_NET_WM_WINDOW_OPACITY", False),
                       0L, 1L, False, XA_CARDINAL, &actual, &format, &n, &left,
                       (unsigned char **) &data);
    if(data)
    {
        memcpy(&current_opacity_raw, data, sizeof(unsigned int));
        XFree(data);
        current_opacity = (current_opacity_raw * 100.0) / 0xffffffff;
    }
    else
        set_prop = 1;

    delta = compute_new_value_from_arg(arg, current_opacity);

    if(delta <= 0.0)
        delta = 0.0;
    else if(delta > 100.0)
    {
        delta = 100.0;
        set_prop = 1;
    }

    if(delta == 100.0 && !set_prop)
        setclienttrans(*awesomeconf->client_sel, -1);
    else
        setclienttrans(*awesomeconf->client_sel, delta);
}


/** Set borrder size
 * \param awesomeconf awesome config
 * \param arg X, +X or -X
 * \ingroup ui_callback
 */
void
uicb_setborder(awesome_config *awesomeconf,
               const char *arg)
{
    if(!arg)
        return;

    if((awesomeconf->borderpx = (int) compute_new_value_from_arg(arg, (double) awesomeconf->borderpx)) < 0)
        awesomeconf->borderpx = 0;
}

void
uicb_swapnext(awesome_config *awesomeconf,
              const char *arg __attribute__ ((unused)))
{
    Client *next, *sel = *awesomeconf->client_sel;

    if(!sel)
        return;

    for(next = sel->next; next && !isvisible(next, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); next = next->next);
    if(next)
    {
        client_swap(awesomeconf->clients, *awesomeconf->client_sel, next);
        arrange(awesomeconf);
        /* restore focus */
        focus(sel, True, awesomeconf);
    }
}

void
uicb_swapprev(awesome_config *awesomeconf,
              const char *arg __attribute__ ((unused)))
{
    Client *prev, *sel = *awesomeconf->client_sel;

    if(!sel)
        return;

    for(prev = sel->prev; prev && !isvisible(prev, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); prev = prev->prev);
    if(prev)
    {
        client_swap(awesomeconf->clients, prev, *awesomeconf->client_sel);
        arrange(awesomeconf);
        /* restore focus */
        focus(sel, True, awesomeconf);
    }
}

void
uicb_moveresize(awesome_config *awesomeconf,
                const char *arg)
{
    int nx, ny, nw, nh, ox, oy, ow, oh;
    char x[8], y[8], w[8], h[8];
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;

    if(get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange == layout_floating)
        if(!*awesomeconf->client_sel || !(*awesomeconf->client_sel)->isfloating || (*awesomeconf->client_sel)->isfixed || !arg)
            return;
    if(sscanf(arg, "%s %s %s %s", x, y, w, h) != 4)
        return;
    nx = (int) compute_new_value_from_arg(x, (*awesomeconf->client_sel)->x);
    ny = (int) compute_new_value_from_arg(y, (*awesomeconf->client_sel)->y);
    nw = (int) compute_new_value_from_arg(w, (*awesomeconf->client_sel)->w);
    nh = (int) compute_new_value_from_arg(h, (*awesomeconf->client_sel)->h);

    ox = (*awesomeconf->client_sel)->x;
    oy = (*awesomeconf->client_sel)->y;
    ow = (*awesomeconf->client_sel)->w;
    oh = (*awesomeconf->client_sel)->h;

    Bool xqp = XQueryPointer(awesomeconf->display, RootWindow(awesomeconf->display, awesomeconf->phys_screen), &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    resize(*awesomeconf->client_sel, nx, ny, nw, nh, awesomeconf, True);
    if (xqp && ox <= mx && (ox + ow) >= mx && oy <= my && (oy + oh) >= my)
    {
        nmx = mx - ox + (*awesomeconf->client_sel)->w - ow - 1 < 0 ? 0 : mx - ox + (*awesomeconf->client_sel)->w - ow - 1;
        nmy = my - oy + (*awesomeconf->client_sel)->h - oh - 1 < 0 ? 0 : my - oy + (*awesomeconf->client_sel)->h - oh - 1;
        XWarpPointer(awesomeconf->display, None, (*awesomeconf->client_sel)->win, 0, 0, 0, 0, nmx, nmy);
    }
}

/** Kill selected client
 * \param awesomeconf awesome config
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_killclient(awesome_config *awesomeconf,
                const char *arg __attribute__ ((unused)))
{
    XEvent ev;

    if(!*awesomeconf->client_sel)
        return;
    if(isprotodel(*awesomeconf->client_sel))
    {
        ev.type = ClientMessage;
        ev.xclient.window = (*awesomeconf->client_sel)->win;
        ev.xclient.message_type = XInternAtom(awesomeconf->display, "WM_PROTOCOLS", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = XInternAtom(awesomeconf->display, "WM_DELETE_WINDOW", False);
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(awesomeconf->display, (*awesomeconf->client_sel)->win, False, NoEventMask, &ev);
    }
    else
        XKillClient(awesomeconf->display, (*awesomeconf->client_sel)->win);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
