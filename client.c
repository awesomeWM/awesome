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
#include "xutil.h"
#include "statusbar.h"
#include "window.h"
#include "layouts/floating.h"

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

/** Check if client supports protocol WM_DELETE_WINDOW
 * \param c the client
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
client_ban(Client * c)
{
    XUnmapWindow(c->display, c->win);
    window_setstate(c->display, c->win, IconicState);
}

/** Attach client after another one
* \param client to attach to
* \param c the client
*/
void
client_reattach_after(Client *head, Client *c)
{
    if(head->next == c)
        return;

    if(head->next)
        head->next->prev = c;

    if(c->prev)
        c->prev->next = c->next;

    c->next = head->next;
    head->next = c;
    c->prev = head;
}

/** Attach client to the beginning of the clients stack
 * \param head client list
 * \param c the client
 */
void
client_attach(Client **head, Client *c)
{
    if(*head)
        (*head)->prev = c;
    c->next = *head;
    *head = c;
}

/** Detach client from clients list
 * \param head client list
 * \param c client to detach
 */
void
client_detach(Client **head, Client *c)
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
 * \param c client
 * \param selscreen True if current screen is selected
 * \param awesomeconf awesome config
 */
void
focus(Client *c, Bool selscreen, awesome_config *awesomeconf, int screen)
{
    int i;
    Tag *tag = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags);

    /* if c is NULL or invisible, take next client in the stack */
    if((!c && selscreen) || (c && !isvisible(c, screen, awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)))
        for(c = awesomeconf->clients; c && !isvisible(c, screen, awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags); c = c->next);

    /* XXX unfocus other tags clients, this is a bit too much */
    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
        if(awesomeconf->screens[screen].tags[i].client_sel)
        {
            window_grabbuttons(awesomeconf->screens[screen].tags[i].client_sel->display,
                               awesomeconf->screens[screen].tags[i].client_sel->phys_screen,
                               awesomeconf->screens[screen].tags[i].client_sel->win,
                               False, True, awesomeconf->buttons.root,
                               awesomeconf->buttons.client, awesomeconf->numlockmask);
            XSetWindowBorder(awesomeconf->screens[screen].tags[i].client_sel->display,
                             awesomeconf->screens[screen].tags[i].client_sel->win,
                             awesomeconf->screens[screen].colors_normal[ColBorder].pixel);
            window_settrans(awesomeconf->screens[screen].tags[i].client_sel->display,
                            awesomeconf->screens[screen].tags[i].client_sel->win, awesomeconf->screens[screen].opacity_unfocused);
        }
    if(c)
    {
        XSetWindowBorder(awesomeconf->display, c->win, awesomeconf->screens[screen].colors_selected[ColBorder].pixel);
        window_grabbuttons(c->display, c->phys_screen, c->win,
                           True, True, awesomeconf->buttons.root,
                           awesomeconf->buttons.client, awesomeconf->numlockmask);
    }
    if(!selscreen)
        return;
    tag->client_sel = c;
    drawstatusbar(awesomeconf, screen);
    if(tag->client_sel)
    {
        XSetInputFocus(tag->client_sel->display, tag->client_sel->win, RevertToPointerRoot, CurrentTime);
        for(c = awesomeconf->clients; c; c = c->next)
            if(c != tag->client_sel)
                window_settrans(awesomeconf->display, tag->client_sel->win, awesomeconf->screens[screen].opacity_unfocused);
        window_settrans(awesomeconf->display, tag->client_sel->win, -1);
    }
    else
        XSetInputFocus(awesomeconf->display, RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)), RevertToPointerRoot, CurrentTime);
}


/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param ntags tags number
 */
Bool
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
 * \param w The window
 * \param wa Window attributes
 * \param awesomeconf awesome config
 */
void
client_manage(Window w, XWindowAttributes *wa, awesome_config *awesomeconf, int screen)
{
    int i;
    Client *c, *t = NULL;
    Window trans;
    Status rettrans;
    XWindowChanges wc;
    ScreenInfo *screen_info;

    c = p_new(Client, 1);

    c->win = w;
    c->x = c->rx = wa->x;
    c->y = c->ry = wa->y;
    c->w = c->rw = wa->width;
    c->h = c->rh = wa->height;
    c->oldborder = wa->border_width;

    c->display = awesomeconf->display;
    c->screen = get_screen_bycoord(c->display, c->x, c->y);
    c->phys_screen = get_phys_screen(awesomeconf->display, c->screen);

    move_client_to_screen(c, awesomeconf, screen, True);

    /* update window title */
    updatetitle(c);

    /* loadprops or apply rules if no props */
    if(!loadprops(c, awesomeconf->screens[screen].ntags))
        tag_client_with_rules(c, awesomeconf);

    screen_info = get_screen_info(awesomeconf->display, screen, NULL, NULL);

    /* if window request fullscreen mode */
    if(c->w == screen_info[screen].width && c->h == screen_info[screen].height)
    {
        c->x = screen_info[screen].x_org;
        c->y = screen_info[screen].y_org;

        c->border = wa->border_width;
    }
    else
    {
        ScreenInfo *display_info = get_display_info(c->display, c->phys_screen, &awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);

        if(c->x + c->w + 2 * c->border > display_info->x_org + display_info->width)
            c->x = c->rx = display_info->x_org + display_info->width - c->w - 2 * c->border;
        if(c->y + c->h + 2 * c->border > display_info->y_org + display_info->height)
            c->y = c->ry = display_info->y_org + display_info->height - c->h - 2 * c->border;
        if(c->x < display_info->x_org)
            c->x = c->rx = display_info->x_org;
        if(c->y < display_info->y_org)
            c->y = c->ry = display_info->y_org;

        c->border = awesomeconf->screens[screen].borderpx;

        p_delete(&display_info);
    }
    p_delete(&screen_info);

    /* set borders */
    wc.border_width = c->border;
    XConfigureWindow(c->display, w, CWBorderWidth, &wc);
    XSetWindowBorder(c->display, w, awesomeconf->screens[screen].colors_normal[ColBorder].pixel);

    /* propagates border_width, if size doesn't change */
    window_configure(c->display, c->win, c->x, c->y, c->w, c->h, c->border);

    /* update sizehint */
    updatesizehints(c);

    XSelectInput(c->display, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);

    /* handle xshape */
    if(awesomeconf->have_shape)
    {
        XShapeSelectInput(c->display, w, ShapeNotifyMask);
        window_setshape(c->display, c->phys_screen, c->win);
    }

    /* grab buttons */
    window_grabbuttons(c->display, c->phys_screen, c->win,
                       False, True, awesomeconf->buttons.root,
                       awesomeconf->buttons.client, awesomeconf->numlockmask);

    /* check for transient and set tags like its parent */
    if((rettrans = XGetTransientForHint(c->display, w, &trans) == Success)
       && (t = get_client_bywin(awesomeconf->clients, trans)))
        for(i = 0; i < awesomeconf->screens[c->screen].ntags; i++)
            c->tags[i] = t->tags[i];

    /* should be floating if transsient or fixed) */
    if(!c->isfloating)
        c->isfloating = (rettrans == Success) || c->isfixed;

    /* save new props */
    saveprops(c, awesomeconf->screens[c->screen].ntags);

    /* attach to the stack */
    client_attach(&awesomeconf->clients, c);

    /* some windows require this */
    XMoveResizeWindow(c->display, c->win, c->x, c->y, c->w, c->h);

    focus(c, True, awesomeconf, screen);

    /* rearrange to display new window */
    arrange(awesomeconf, screen);
}

void
client_resize(Client *c, int x, int y, int w, int h, awesome_config *awesomeconf,
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
    si = get_display_info(c->display, c->phys_screen, NULL, &awesomeconf->screens[c->screen].padding);
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
               || get_current_layout(awesomeconf->screens[c->screen].tags, awesomeconf->screens[c->screen].ntags)->arrange == layout_floating))
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
                move_client_to_screen(c, awesomeconf, new_screen, False);
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
client_unban(Client *c)
{
    XMapWindow(c->display, c->win);
    window_setstate(c->display, c->win, NormalState);
}

void
client_unmanage(Client *c, long state, awesome_config *awesomeconf)
{
    XWindowChanges wc;
    int tag;

    wc.border_width = c->oldborder;
    /* The server grab construct avoids race conditions. */
    XGrabServer(c->display);
    XConfigureWindow(c->display, c->win, CWBorderWidth, &wc);  /* restore border */
    client_detach(&awesomeconf->clients, c);
    if(get_current_tag(awesomeconf->screens[c->screen].tags, awesomeconf->screens[c->screen].ntags)->client_sel == c)
        focus(NULL, True, awesomeconf, c->screen);
    for(tag = 0; tag < awesomeconf->screens[c->screen].ntags; tag++)
        if(awesomeconf->screens[c->screen].tags[tag].client_sel == c)
            awesomeconf->screens[c->screen].tags[tag].client_sel = NULL;
    XUngrabButton(c->display, AnyButton, AnyModifier, c->win);
    window_setstate(c->display, c->win, state);
    XSync(c->display, False);
    XSetErrorHandler(xerror);
    XUngrabServer(c->display);
    if(state != NormalState)
        arrange(awesomeconf, c->screen);
    p_delete(&c->tags);
    p_delete(&c);
}

void
updatesizehints(Client *c)
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
tag_client_with_rules(Client *c, awesome_config *awesomeconf)
{
    Rule *r;
    Bool matched = False;
    int i;

    for(r = awesomeconf->rules; r; r = r->next)
        if(client_match_rule(c, r))
        {
            c->isfloating = r->isfloating;

            if(r->screen != RULE_NOSCREEN && r->screen != c->screen)
                move_client_to_screen(c, awesomeconf, r->screen, True);

            for(i = 0; i < awesomeconf->screens[c->screen].ntags; i++)
                if(is_tag_match_rules(&awesomeconf->screens[c->screen].tags[i], r))
                {
                    matched = True;
                    c->tags[i] = True;
                }
                else
                    c->tags[i] = False;

            if(!matched)
                tag_client_with_current_selected(c, &awesomeconf->screens[c->screen]);
            break;
        }
}

/** Set selected client transparency
 * \param awesomeconf awesome config
 * \param arg unused arg
 * \ingroup ui_callback
 */
void
uicb_client_settrans(awesome_config *awesomeconf,
                     int screen,
                     const char *arg)
{
    double delta = 100.0, current_opacity = 100.0;
    unsigned char *data;
    Atom actual;
    int format;
    unsigned long n, left;
    unsigned int current_opacity_raw = 0;
    int set_prop = 0;
    Client *sel = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->client_sel;

    if(!sel)
        return;

    XGetWindowProperty(awesomeconf->display, sel->win,
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


/** Set borrder size
 * \param awesomeconf awesome config
 * \param arg X, +X or -X
 * \ingroup ui_callback
 */
void
uicb_setborder(awesome_config *awesomeconf,
               int screen,
               const char *arg)
{
    if(!arg)
        return;

    if((awesomeconf->screens[screen].borderpx = (int) compute_new_value_from_arg(arg, (double) awesomeconf->screens[screen].borderpx)) < 0)
        awesomeconf->screens[screen].borderpx = 0;
}

void
uicb_client_swapnext(awesome_config *awesomeconf,
                     int screen,
                     const char *arg __attribute__ ((unused)))
{
    Client *next, *sel = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->client_sel;

    if(!sel)
        return;

    for(next = sel->next; next && !isvisible(next, screen, awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags); next = next->next);
    if(next)
    {
        client_swap(&awesomeconf->clients, sel, next);
        arrange(awesomeconf, screen);
        /* restore focus */
        focus(sel, True, awesomeconf, screen);
    }
}

void
uicb_client_swapprev(awesome_config *awesomeconf,
                     int screen,
                     const char *arg __attribute__ ((unused)))
{
    Client *prev, *sel = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->client_sel;

    if(!sel)
        return;

    for(prev = sel->prev; prev && !isvisible(prev, screen, awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags); prev = prev->prev);
    if(prev)
    {
        client_swap(&awesomeconf->clients, prev, sel);
        arrange(awesomeconf, screen);
        /* restore focus */
        focus(sel, True, awesomeconf, screen);
    }
}

void
uicb_client_moveresize(awesome_config *awesomeconf,
                       int screen,
                       const char *arg)
{
    int nx, ny, nw, nh, ox, oy, ow, oh;
    char x[8], y[8], w[8], h[8];
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;
    Client *sel = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->client_sel;

    if(get_current_layout(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->arrange != layout_floating)
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

    Bool xqp = XQueryPointer(awesomeconf->display, RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)), &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    client_resize(sel, nx, ny, nw, nh, awesomeconf, True, False);
    if (xqp && ox <= mx && (ox + ow) >= mx && oy <= my && (oy + oh) >= my)
    {
        nmx = mx - ox + sel->w - ow - 1 < 0 ? 0 : mx - ox + sel->w - ow - 1;
        nmy = my - oy + sel->h - oh - 1 < 0 ? 0 : my - oy + sel->h - oh - 1;
        XWarpPointer(awesomeconf->display, None, sel->win, 0, 0, 0, 0, nmx, nmy);
    }
}

/** Kill selected client
 * \param awesomeconf awesome config
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_kill(awesome_config *awesomeconf,
                 int screen,
                 const char *arg __attribute__ ((unused)))
{
    XEvent ev;
    Client *sel = get_current_tag(awesomeconf->screens[screen].tags, awesomeconf->screens[screen].ntags)->client_sel;

    if(!sel)
        return;
    if(isprotodel(sel->display, sel->win))
    {
        ev.type = ClientMessage;
        ev.xclient.window = sel->win;
        ev.xclient.message_type = XInternAtom(awesomeconf->display, "WM_PROTOCOLS", False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = XInternAtom(awesomeconf->display, "WM_DELETE_WINDOW", False);
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(awesomeconf->display, sel->win, False, NoEventMask, &ev);
    }
    else
        XKillClient(awesomeconf->display, sel->win);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
