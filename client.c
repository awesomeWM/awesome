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

#include "screen.h"
#include "awesome.h"
#include "layout.h"
#include "tag.h"
#include "rules.h"
#include "util.h"
#include "xutil.h"
#include "statusbar.h"
#include "window.h"
#include "focus.h"
#include "layouts/floating.h"


extern awesome_config globalconf;

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param ntags tags number
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

    if(xgettextprop(c->display, c->win, AWESOMEPROPS_ATOM(c->display), prop, ntags + 2))
    {
        for(i = 0, tag = globalconf.screens[screen].tags; tag && i < ntags && prop[i]; i++, tag = tag->next)
            if(prop[i] == '1')
            {
                tag_client(c, tag, screen);
                result = True;
            }
            else
                untag_client(c, tag, screen);

        if(i <= ntags && prop[i])
            c->isfloating = prop[i] == '1';
    }

    p_delete(&prop);

    return result;
}

/** Check if client supports protocol WM_DELETE_WINDOW
 * \param disp the display
 * \win the Window
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

void
client_updatetitle(Client *c)
{
    if(!xgettextprop(c->display, c->win, XInternAtom(c->display, "_NET_WM_NAME", False), c->name, sizeof(c->name)))
        xgettextprop(c->display, c->win, XInternAtom(c->display, "WM_NAME", False), c->name, sizeof(c->name));
}

/** Ban client and unmap it
 * \param c the client
 */
void
client_ban(Client * c)
{
    XUnmapWindow(c->display, c->win);
    window_setstate(c->display, c->win, IconicState);
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
 */
void
focus(Client *c, Bool selscreen, int screen)
{
    /* unfocus current selected client */
    if(globalconf.focus->client)
    {
        window_grabbuttons(globalconf.focus->client->display, globalconf.focus->client->phys_screen,
                           globalconf.focus->client->win, False, True);
        XSetWindowBorder(globalconf.focus->client->display, globalconf.focus->client->win,
                         globalconf.screens[screen].colors_normal[ColBorder].pixel);
        window_settrans(globalconf.focus->client->display, globalconf.focus->client->win,
                        globalconf.screens[screen].opacity_unfocused);
    }

    /* if c is NULL or invisible, take next client in the stack */
    if((!c && selscreen) || (c && !client_isvisible(c, screen)))
        for(c = globalconf.clients; c && !client_isvisible(c, screen); c = c->next);

    if(c)
    {
        XSetWindowBorder(globalconf.display, c->win, globalconf.screens[screen].colors_selected[ColBorder].pixel);
        window_grabbuttons(c->display, c->phys_screen, c->win,
                           True, True);
    }

    if(!selscreen)
        return;

    /* save old sel in focus history */
    focus_add_client(c);

    statusbar_draw(screen);

    if(globalconf.focus->client)
    {
        XSetInputFocus(globalconf.focus->client->display,
                       globalconf.focus->client->win, RevertToPointerRoot, CurrentTime);
        for(c = globalconf.clients; c; c = c->next)
            if(c != globalconf.focus->client)
                window_settrans(globalconf.display, globalconf.focus->client->win,
                                globalconf.screens[screen].opacity_unfocused);
        window_settrans(globalconf.display, globalconf.focus->client->win, -1);
    }
    else
        XSetInputFocus(globalconf.display,
                       RootWindow(globalconf.display, get_phys_screen(globalconf.display, screen)),
                       RevertToPointerRoot, CurrentTime);
}

/** Manage a new client
 * \param w The window
 * \param wa Window attributes
 */
void
client_manage(Window w, XWindowAttributes *wa, int screen)
{
    Client *c, *t = NULL;
    Window trans;
    Status rettrans;
    XWindowChanges wc;
    ScreenInfo *screen_info;
    Tag *tag;

    c = p_new(Client, 1);

    c->win = w;
    c->x = c->rx = wa->x;
    c->y = c->ry = wa->y;
    c->w = c->rw = wa->width;
    c->h = c->rh = wa->height;
    c->oldborder = wa->border_width;

    c->display = globalconf.display;
    c->screen = get_screen_bycoord(c->display, c->x, c->y);
    c->phys_screen = get_phys_screen(globalconf.display, c->screen);

    move_client_to_screen(c, screen, True);

    /* update window title */
    client_updatetitle(c);

    /* loadprops or apply rules if no props */
    if(!client_loadprops(c, screen))
        tag_client_with_rules(c);

    screen_info = get_screen_info(globalconf.display, screen, NULL, NULL);

    /* if window request fullscreen mode */
    if(c->w == screen_info[screen].width && c->h == screen_info[screen].height)
    {
        c->x = screen_info[screen].x_org;
        c->y = screen_info[screen].y_org;

        c->border = wa->border_width;
    }
    else
    {
        ScreenInfo *display_info = get_display_info(c->display, c->phys_screen, &globalconf.screens[screen].statusbar, &globalconf.screens[screen].padding);

        if(c->x + c->w + 2 * c->border > display_info->x_org + display_info->width)
            c->x = c->rx = display_info->x_org + display_info->width - c->w - 2 * c->border;
        if(c->y + c->h + 2 * c->border > display_info->y_org + display_info->height)
            c->y = c->ry = display_info->y_org + display_info->height - c->h - 2 * c->border;
        if(c->x < display_info->x_org)
            c->x = c->rx = display_info->x_org;
        if(c->y < display_info->y_org)
            c->y = c->ry = display_info->y_org;

        c->border = globalconf.screens[screen].borderpx;

        p_delete(&display_info);
    }
    p_delete(&screen_info);

    /* set borders */
    wc.border_width = c->border;
    XConfigureWindow(c->display, w, CWBorderWidth, &wc);
    XSetWindowBorder(c->display, w, globalconf.screens[screen].colors_normal[ColBorder].pixel);

    /* propagates border_width, if size doesn't change */
    window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);

    /* update sizehint */
    client_updatesizehints(c);

    XSelectInput(c->display, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);

    /* handle xshape */
    if(globalconf.have_shape)
    {
        XShapeSelectInput(c->display, w, ShapeNotifyMask);
        window_setshape(c->display, c->phys_screen, c->win);
    }

    /* grab buttons */
    window_grabbuttons(c->display, c->phys_screen, c->win, False, True);

    /* check for transient and set tags like its parent */
    if((rettrans = XGetTransientForHint(c->display, w, &trans) == Success)
       && (t = get_client_bywin(globalconf.clients, trans)))
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            if(is_client_tagged(t, tag, c->screen))
                tag_client(c, tag, c->screen);

    /* should be floating if transsient or fixed) */
    if(!c->isfloating)
        c->isfloating = (rettrans == Success) || c->isfixed;

    /* save new props */
    client_saveprops(c, c->screen);

    /* attach to the stack */
    client_attach(c);

    /* some windows require this */
    XMoveResizeWindow(c->display, c->win, c->x, c->y, c->w, c->h);

    focus(c, True, screen);

    /* rearrange to display new window */
    arrange(screen);
}

void
client_resize(Client *c, int x, int y, int w, int h,
              Bool sizehints, Bool volatile_coords)
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
    si = get_display_info(c->display, c->phys_screen, NULL, &globalconf.screens[c->screen].padding);
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
        if(!volatile_coords
           && (c->isfloating
               || get_current_layout(c->screen)->arrange == layout_floating))
        {
            c->rx = c->x;
            c->ry = c->y;
            c->rw = c->w;
            c->rh = c->h;
        }
        wc.border_width = c->border;
        XConfigureWindow(c->display, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
        window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);
        XSync(c->display, False);
        if((c->x >= 0 || c->y >= 0) && XineramaIsActive(c->display))
        {
            int new_screen = get_screen_bycoord(c->display, c->x, c->y);
            if(c->screen != new_screen)
                move_client_to_screen(c, new_screen, False);
        }
    }
}

void
client_saveprops(Client * c, int screen)
{
    int i = 0, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 2);

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next, i++)
        prop[i] = is_client_tagged(c, tag, screen) ? '1' : '0';

    if(i <= ntags)
        prop[i] = c->isfloating ? '1' : '0';

    prop[++i] = '\0';

    XChangeProperty(c->display, c->win, AWESOMEPROPS_ATOM(c->display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);

    p_delete(&prop);
}

void
client_unban(Client *c)
{
    XMapWindow(c->display, c->win);
    window_setstate(c->display, c->win, NormalState);
}

void
client_unmanage(Client *c, long state)
{
    XWindowChanges wc;
    Tag *tag;

    wc.border_width = c->oldborder;
    /* The server grab construct avoids race conditions. */
    XGrabServer(c->display);
    XConfigureWindow(c->display, c->win, CWBorderWidth, &wc);  /* restore border */
    client_detach(c);
    if(globalconf.focus->client == c)
        focus(NULL, True, c->screen);
    focus_delete_client(c);
    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        untag_client(c, tag, c->screen);
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);
    window_setstate(c->display, c->win, state);
    XSync(c->display, False);
    XSetErrorHandler(xerror);
    XUngrabServer(c->display);
    if(state != NormalState)
        arrange(c->screen);
    p_delete(&c);
}

void
client_updatesizehints(Client *c)
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
        if(tag->selected && is_client_tagged(c, tag, screen))
            return True;
    return False;
}

/** Set selected client transparency
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
                       XInternAtom(sel->display, "_NET_WM_WINDOW_OPACITY", False),
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
        window_settrans(sel->display, sel->win, -1);
    else
        window_settrans(sel->display, sel->win, delta);
}


/** Set border size
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

void
uicb_client_moveresize(int screen, char *arg)
{
    int nx, ny, nw, nh, ox, oy, ow, oh;
    char x[8], y[8], w[8], h[8];
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;
    Client *sel = globalconf.focus->client;

    if(get_current_layout(screen)->arrange != layout_floating)
        if(!sel || !sel->isfloating || sel->isfixed || !arg)
            return;
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

    Bool xqp = XQueryPointer(globalconf.display, RootWindow(globalconf.display, get_phys_screen(globalconf.display, screen)), &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    client_resize(sel, nx, ny, nw, nh, True, False);
    if (xqp && ox <= mx && (ox + ow) >= mx && oy <= my && (oy + oh) >= my)
    {
        nmx = mx - ox + sel->w - ow - 1 < 0 ? 0 : mx - ox + sel->w - ow - 1;
        nmy = my - oy + sel->h - oh - 1 < 0 ? 0 : my - oy + sel->h - oh - 1;
        XWarpPointer(globalconf.display, None, sel->win, 0, 0, 0, 0, nmx, nmy);
    }
}

/** Kill selected client
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_kill(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    XEvent ev;
    Client *sel = globalconf.focus->client;

    if(!sel)
        return;
    if(isprotodel(sel->display, sel->win))
    {
        ev.type = ClientMessage;
        ev.xclient.window = sel->win;
        ev.xclient.message_type = XInternAtom(globalconf.display, "WM_PROTOCOLS", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = XInternAtom(globalconf.display, "WM_DELETE_WINDOW", False);
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(globalconf.display, sel->win, False, NoEventMask, &ev);
    }
    else
        XKillClient(globalconf.display, sel->win);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
