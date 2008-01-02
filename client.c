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
#include <X11/extensions/shape.h>
#include <X11/extensions/Xinerama.h>

#include "client.h"
#include "awesome.h"
#include "tag.h"
#include "rules.h"
#include "util.h"
#include "xutil.h"
#include "statusbar.h"
#include "window.h"
#include "focus.h"
#include "ewmh.h"
#include "screen.h"
#include "layouts/floating.h"


extern AwesomeConf globalconf;

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param screen Screen ID
 */
static Bool
client_loadprops(Client * c, int screen)
{
    int i, ntags = 0;
    Tag *tag;
    char *prop;
    Bool result = False;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 2);

    if(xgettextprop(globalconf.display, c->win, AWESOMEPROPS_ATOM(globalconf.display), prop, ntags + 2))
    {
        for(i = 0, tag = globalconf.screens[screen].tags; tag && i < ntags && prop[i]; i++, tag = tag->next)
            if(prop[i] == '1')
            {
                tag_client(c, tag);
                result = True;
            }
            else
                untag_client(c, tag);

        if(i <= ntags && prop[i])
            c->isfloating = prop[i] == '1';
    }

    p_delete(&prop);

    return result;
}

/** Check if client supports protocol WM_DELETE_WINDOW
 * \param disp the display
 * \param win the Window
 * \return True if client has WM_DELETE_WINDOW
 */
static Bool
isprotodel(Display *disp, Window win)
{
    int i, n;
    Atom *protocols;
    Bool ret = False;

    if(XGetWMProtocols(disp, win, &protocols, &n))
    {
        for(i = 0; !ret && i < n; i++)
            if(protocols[i] == XInternAtom(disp, "WM_DELETE_WINDOW", False))
                ret = True;
        XFree(protocols);
    }
    return ret;
}

/** Swap two client in the linked list clients
 * \param head pointer ito the client list head
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

/** Get a Client by its window
 * \param list Client list to look info
 * \param w Client window to find
 * \return client
 */
Client *
get_client_bywin(Client *list, Window w)
{
    Client *c;

    for(c = list; c && c->win != w; c = c->next);
    return c;
}


/** Get a client by its name
 * \param list Client list
 * \param name name to search
 * \return first matching client
 */
Client *
get_client_byname(Client *list, char *name)
{
    Client *c;

    for(c = list; c; c = c->next)
        if(strstr(c->name, name))
            return c;

    return NULL;
}

/** Update client name attribute with its title
 * \param c the client
 */
void
client_updatetitle(Client *c)
{
    if(!xgettextprop(globalconf.display, c->win, XInternAtom(globalconf.display, "_NET_WM_NAME", False), c->name, sizeof(c->name)))
        xgettextprop(globalconf.display, c->win, XInternAtom(globalconf.display, "WM_NAME", False), c->name, sizeof(c->name));
}

/** Ban client and unmap it
 * \param c the client
 */
void
client_ban(Client * c)
{
    XUnmapWindow(globalconf.display, c->win);
    window_setstate(c->win, IconicState);
}

/** Attach client to the beginning of the clients stack
 * \param c the client
 */
void
client_attach(Client *c)
{
    if(globalconf.clients)
        (globalconf.clients)->prev = c;
    c->next = globalconf.clients;
    globalconf.clients = c;
}

/** Detach client from clients list
 * \param c client to detach
 */
void
client_detach(Client *c)
{
    if(c->prev)
        c->prev->next = c->next;
    if(c->next)
        c->next->prev = c->prev;
    if(c == globalconf.clients)
        globalconf.clients = c->next;
    c->next = c->prev = NULL;
}

/** Give focus to client, or to first client if c is NULL
 * \param c client
 * \param selscreen True if current screen is selected
 * \param screen Screen ID
 */
void
focus(Client *c, Bool selscreen, int screen)
{
    /* unfocus current selected client */
    if(globalconf.focus->client)
    {
        window_grabbuttons(globalconf.focus->client->phys_screen,
                           globalconf.focus->client->win, False, True);
        XSetWindowBorder(globalconf.display, globalconf.focus->client->win,
                         globalconf.screens[screen].colors_normal[ColBorder].pixel);
        window_settrans(globalconf.focus->client->win,
                        globalconf.screens[screen].opacity_unfocused);
    }


    /* if c is NULL or invisible, take next client in the focus history */
    if((!c && selscreen) || (c && !client_isvisible(c, screen)))
    {
        c = focus_get_current_client(screen);
        /* if c is still NULL take next client in the stack */
        if(!c)
            for(c = globalconf.clients; c && (c->skip || !client_isvisible(c, screen)); c = c->next);
    }

    if(c)
    {
        XSetWindowBorder(globalconf.display, c->win, globalconf.screens[screen].colors_selected[ColBorder].pixel);
        window_grabbuttons(c->phys_screen, c->win, True, True);
    }

    if(!selscreen)
        return;

    /* save sel in focus history */
    focus_add_client(c);

    statusbar_draw_all(screen);

    if(globalconf.focus->client)
    {
        XSetInputFocus(globalconf.display,
                       globalconf.focus->client->win, RevertToPointerRoot, CurrentTime);
        for(c = globalconf.clients; c; c = c->next)
            if(c != globalconf.focus->client)
                window_settrans(globalconf.focus->client->win,
                                globalconf.screens[screen].opacity_unfocused);
        window_settrans(globalconf.focus->client->win, -1);
    }
    else
        XSetInputFocus(globalconf.display,
                       RootWindow(globalconf.display, get_phys_screen(screen)),
                       RevertToPointerRoot, CurrentTime);

    ewmh_update_net_active_window(get_phys_screen(screen));
}

/** Manage a new client
 * \param w The window
 * \param wa Window attributes
 * \param screen Screen ID
 */
void
client_manage(Window w, XWindowAttributes *wa, int screen)
{
    Client *c, *t = NULL;
    Window trans;
    Status rettrans;
    XWindowChanges wc;
    Area area, darea;
    Tag *tag;

    c = p_new(Client, 1);

    c->win = w;
    c->x = c->rx = wa->x;
    c->y = c->ry = wa->y;
    c->w = c->rw = wa->width;
    c->h = c->rh = wa->height;
    c->oldborder = wa->border_width;

    globalconf.display = globalconf.display;
    c->screen = get_screen_bycoord(c->x, c->y);
    c->phys_screen = get_phys_screen(c->screen);

    move_client_to_screen(c, screen, True);

    /* update window title */
    client_updatetitle(c);

    if(c->w == area.width && c->h == area.height)
        c->border = wa->border_width;
    else
        c->border = globalconf.screens[screen].borderpx;

    ewmh_check_client_hints(c);
    /* loadprops or apply rules if no props */
    if(!client_loadprops(c, screen))
        tag_client_with_rules(c);

    area = get_screen_area(screen, NULL, NULL);

    /* if window request fullscreen mode */
    if(c->w == area.width && c->h == area.height)
    {
        c->x = area.x;
        c->y = area.y;
    }
    else
    {
        darea = get_display_area(c->phys_screen,
                                 globalconf.screens[screen].statusbar,
                                 &globalconf.screens[screen].padding);

        if(c->x + c->w + 2 * c->border > darea.x + darea.width)
            c->x = c->rx = darea.x + darea.width - c->w - 2 * c->border;
        if(c->y + c->h + 2 * c->border > darea.y + darea.height)
            c->y = c->ry = darea.y + darea.height - c->h - 2 * c->border;
        if(c->x < darea.x)
            c->x = c->rx = darea.x;
        if(c->y < darea.y)
            c->y = c->ry = darea.y;
    }

    /* set borders */
    wc.border_width = c->border;
    XConfigureWindow(globalconf.display, w, CWBorderWidth, &wc);
    XSetWindowBorder(globalconf.display, w, globalconf.screens[screen].colors_normal[ColBorder].pixel);

    /* propagates border_width, if size doesn't change */
    window_configure(c->win, c->x, c->y, c->w, c->h, c->border);

    /* update hints */
    client_updatesizehints(c);
    client_updatewmhints(c);

    XSelectInput(globalconf.display, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);

    /* handle xshape */
    if(globalconf.have_shape)
    {
        XShapeSelectInput(globalconf.display, w, ShapeNotifyMask);
        window_setshape(c->phys_screen, c->win);
    }

    /* grab buttons */
    window_grabbuttons(c->phys_screen, c->win, False, True);

    /* check for transient and set tags like its parent */
    if((rettrans = XGetTransientForHint(globalconf.display, w, &trans) == Success)
       && (t = get_client_bywin(globalconf.clients, trans)))
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            if(is_client_tagged(t, tag))
                tag_client(c, tag);

    /* should be floating if transsient or fixed */
    if(!c->isfloating)
        c->isfloating = (rettrans == Success) || c->isfixed;

    /* save new props */
    client_saveprops(c);

    /* attach to the stack */
    client_attach(c);

    /* some windows require this */
    XMoveResizeWindow(globalconf.display, c->win, c->x, c->y, c->w, c->h);

    focus(c, True, screen);

    ewmh_update_net_client_list(c->phys_screen);

    /* rearrange to display new window */
    arrange(screen);
}

/** Resize client window
 * \param c client to resize
 * \param x x coord
 * \param y y coord
 * \param w width
 * \param h height
 * \param sizehints respect size hints
 * \param volatile_coords register coords in rx/ry/rw/rh
 */
void
client_resize(Client *c, int x, int y, int w, int h,
              Bool sizehints, Bool volatile_coords)
{
    double dx, dy, max, min, ratio;
    XWindowChanges wc;
    Area area;
    Tag **curtags;

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
    area = get_display_area(c->phys_screen,
                            NULL,
                            &globalconf.screens[c->screen].padding);
    if(x > area.width)
        x = area.width - w - 2 * c->border;
    if(y > area.height)
        y = area.height - h - 2 * c->border;
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
        curtags = get_current_tags(c->screen);
        if(!volatile_coords
           && (c->isfloating
               || curtags[0]->layout->arrange == layout_floating))
        {
            c->rx = c->x;
            c->ry = c->y;
            c->rw = c->w;
            c->rh = c->h;
        }
        p_delete(&curtags);
        wc.border_width = c->border;
        XConfigureWindow(globalconf.display, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
        window_configure(c->win, c->x, c->y, c->w, c->h, c->border);
        XSync(globalconf.display, False);
        if((c->x >= 0 || c->y >= 0) && XineramaIsActive(globalconf.display))
        {
            int new_screen = get_screen_bycoord(c->x, c->y);
            if(c->screen != new_screen)
                move_client_to_screen(c, new_screen, False);
        }
    }
}

/** Save client properties as an X property
 * \param c client
 */
void
client_saveprops(Client *c)
{
    int i = 0, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 2);

    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next, i++)
        prop[i] = is_client_tagged(c, tag) ? '1' : '0';

    if(i <= ntags)
        prop[i] = c->isfloating ? '1' : '0';

    prop[++i] = '\0';

    XChangeProperty(globalconf.display, c->win, AWESOMEPROPS_ATOM(globalconf.display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);

    p_delete(&prop);
}

void
client_unban(Client *c)
{
    XMapWindow(globalconf.display, c->win);
    window_setstate(c->win, NormalState);
}

void
client_unmanage(Client *c, long state)
{
    XWindowChanges wc;
    Tag *tag;

    wc.border_width = c->oldborder;
    /* The server grab construct avoids race conditions. */
    XGrabServer(globalconf.display);
    XConfigureWindow(globalconf.display, c->win, CWBorderWidth, &wc);  /* restore border */
    client_detach(c);
    if(globalconf.focus->client == c)
        focus(NULL, True, c->screen);
    focus_delete_client(c);
    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        untag_client(c, tag);
    XUngrabButton(globalconf.display, AnyButton, AnyModifier, c->win);
    window_setstate(c->win, state);
    XSync(globalconf.display, False);
    XSetErrorHandler(xerror);
    XUngrabServer(globalconf.display);
    if(state != NormalState)
        arrange(c->screen);
    p_delete(&c);
}

void
client_updatewmhints(Client *c)
{
    XWMHints *wmh;

    if((wmh = XGetWMHints(globalconf.display, c->win)))
    {
        c->isurgent = (wmh->flags & XUrgencyHint);
        if((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
            c->skip = True;
        XFree(wmh);
    }
}

void
client_updatesizehints(Client *c)
{
    long msize;
    XSizeHints size;

    if(!XGetWMNormalHints(globalconf.display, c->win, &size, &msize) || !size.flags)
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

    if(c->maxw && c->minw && c->maxh && c->minh
       && c->maxw == c->minw && c->maxh == c->minh)
        c->isfixed = True;
}

/** Returns True if a client is tagged
 * with one of the tags
 * \return True or False
 */
Bool
client_isvisible(Client *c, int screen)
{
    Tag *tag;

    if(c->screen != screen)
        return False;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected && is_client_tagged(c, tag))
            return True;
    return False;
}

/** Set selected client transparency
 * \param screen Screen ID
 * \param arg unused arg
 * \ingroup ui_callback
 */
void
uicb_client_settrans(int screen __attribute__ ((unused)), char *arg)
{
    double delta = 100.0, current_opacity = 100.0;
    unsigned char *data;
    Atom actual;
    int format;
    unsigned long n, left;
    unsigned int current_opacity_raw = 0;
    int set_prop = 0;
    Client *sel = globalconf.focus->client;

    if(!sel)
        return;

    XGetWindowProperty(globalconf.display, sel->win,
                       XInternAtom(globalconf.display, "_NET_WM_WINDOW_OPACITY", False),
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
        window_settrans(sel->win, -1);
    else
        window_settrans(sel->win, delta);
}


/** Set border size
 * \param screen Screen ID
 * \param arg X, +X or -X
 * \ingroup ui_callback
 */
void
uicb_setborder(int screen, char *arg)
{
    if(!arg)
        return;

    if((globalconf.screens[screen].borderpx = (int) compute_new_value_from_arg(arg, (double) globalconf.screens[screen].borderpx)) < 0)
        globalconf.screens[screen].borderpx = 0;
}

/** Swap current with next client
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapnext(int screen, char *arg __attribute__ ((unused)))
{
    Client *next, *sel = globalconf.focus->client;

    if(!sel)
        return;

    for(next = sel->next; next && !client_isvisible(next, screen); next = next->next);
    if(next)
    {
        client_swap(&globalconf.clients, sel, next);
        arrange(screen);
        /* restore focus */
        focus(sel, True, screen);
    }
}

/** Swap current with previous client
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapprev(int screen, char *arg __attribute__ ((unused)))
{
    Client *prev, *sel = globalconf.focus->client;

    if(!sel)
        return;

    for(prev = sel->prev; prev && !client_isvisible(prev, screen); prev = prev->prev);
    if(prev)
    {
        client_swap(&globalconf.clients, prev, sel);
        arrange(screen);
        /* restore focus */
        focus(sel, True, screen);
    }
}

/** Move and resize client
 * \param screen Screen ID
 * \param arg x y w h
 * \ingroup ui_callback
 */
void
uicb_client_moveresize(int screen, char *arg)
{
    int nx, ny, nw, nh, ox, oy, ow, oh;
    char x[8], y[8], w[8], h[8];
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;
    Client *sel = globalconf.focus->client;
    Tag **curtags = get_current_tags(screen);

    if(curtags[0]->layout->arrange != layout_floating)
        if(!sel || !sel->isfloating || sel->isfixed || !arg)
        {
            p_delete(&curtags);
            return;
        }
    p_delete(&curtags);
    if(sscanf(arg, "%s %s %s %s", x, y, w, h) != 4)
        return;
    nx = (int) compute_new_value_from_arg(x, sel->x);
    ny = (int) compute_new_value_from_arg(y, sel->y);
    nw = (int) compute_new_value_from_arg(w, sel->w);
    nh = (int) compute_new_value_from_arg(h, sel->h);

    ox = sel->x;
    oy = sel->y;
    ow = sel->w;
    oh = sel->h;

    Bool xqp = XQueryPointer(globalconf.display,
                             RootWindow(globalconf.display,
                                        get_phys_screen(screen)),
                             &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    client_resize(sel, nx, ny, nw, nh, True, False);
    if (xqp && ox <= mx && (ox + ow) >= mx && oy <= my && (oy + oh) >= my)
    {
        nmx = mx - ox + sel->w - ow - 1 < 0 ? 0 : mx - ox + sel->w - ow - 1;
        nmy = my - oy + sel->h - oh - 1 < 0 ? 0 : my - oy + sel->h - oh - 1;
        XWarpPointer(globalconf.display,
                     None, sel->win,
                     0, 0, 0, 0, nmx, nmy);
    }
}


void
client_kill(Client *c)
{
    XEvent ev;

    if(isprotodel(globalconf.display, c->win))
    {
        ev.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = XInternAtom(globalconf.display, "WM_PROTOCOLS", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = XInternAtom(globalconf.display, "WM_DELETE_WINDOW", False);
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(globalconf.display, c->win, False, NoEventMask, &ev);
    }
    else
        XKillClient(globalconf.display, c->win);
}

/** Kill selected client
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_kill(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;

    if(sel)
        client_kill(sel);
}

void
client_maximize(Client *c, int x, int y, int w, int h)
{
    if((c->ismax = !c->ismax))
    {
        c->wasfloating = c->isfloating;
        c->isfloating = True;
        client_resize(c, x, y, w, h, False, True);
    }
    else if(c->wasfloating)
        client_resize(c, c->rx, c->ry, c->rw, c->rh, True, False);
    else
        c->isfloating = False;

    arrange(c->screen);
}

/** Toggle maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglemax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);
    if(sel)
        client_maximize(sel, area.x, area.y,
                        area.width - 2 * globalconf.screens[screen].borderpx,
                        area.height - 2 * globalconf.screens[screen].borderpx);
}

/** Toggle vertical maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_toggleverticalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
        client_maximize(sel, sel->x, area.y,
                        sel->w,
                        area.height - 2 * globalconf.screens[screen].borderpx);
}


/** Toggle horizontal maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglehorizontalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
        client_maximize(sel, area.x, sel->y,
                        area.height - 2 * globalconf.screens[screen].borderpx,
                        sel->h);
}

/** Zoom client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_zoom(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;

    if(globalconf.clients == sel)
         for(sel = sel->next; sel && !client_isvisible(sel, screen); sel = sel->next);

    if(!sel)
        return;

    client_detach(sel);
    client_attach(sel);

    focus(sel, True, screen);
    arrange(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
