/*
 * client.c - client management
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include "client.h"
#include "tag.h"
#include "rules.h"
#include "window.h"
#include "focus.h"
#include "ewmh.h"
#include "widget.h"
#include "screen.h"
#include "layouts/floating.h"
#include "common/xutil.h"
#include "common/xscreen.h"

extern AwesomeConf globalconf;

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param screen Screen ID
 * \return true if client had property
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

    if(xgettextprop(globalconf.display, c->win,
                    XInternAtom(globalconf.display, "_AWESOME_PROPERTIES", False),
                    prop, ntags + 2))
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
            client_setfloating(c, prop[i] == '1');
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
client_isprotodel(Display *disp, Window win)
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

/** Get a Client by its window
 * \param list Client list to look info
 * \param w Client window to find
 * \return client
 */
Client *
client_get_bywin(Client *list, Window w)
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
client_get_byname(Client *list, char *name)
{
    Client *c;

    for(c = list; c; c = c->next)
        if(strstr(c->name, name))
            return c;

    return NULL;
}

static void
client_updatetitlebar(Client *c)
{
    DrawCtx *ctx;
    int phys_screen;
    style_t style;
    area_t geometry;

    if(!c->titlebar)
        return;

    phys_screen = get_phys_screen(c->screen);

    ctx = draw_context_new(globalconf.display, phys_screen,
                           c->titlebar->geometry.width,
                           c->titlebar->geometry.height,
                           c->titlebar->drawable);

    style = globalconf.focus->client == c ?
        globalconf.screens[c->screen].styles.focus :
        globalconf.screens[c->screen].styles.normal;

    geometry = c->titlebar->geometry;
    geometry.x = geometry.y = 0;

    draw_text(ctx, geometry, AlignCenter, 0,
              c->name, style);

    simplewindow_refresh_drawable(c->titlebar, phys_screen);

    draw_context_delete(ctx);
}

/** Update client name attribute with its title
 * \param c the client
 */
void
client_updatetitle(Client *c)
{
    if(!xgettextprop(globalconf.display, c->win,
                     XInternAtom(globalconf.display, "_NET_WM_NAME", False), c->name, sizeof(c->name)))
        xgettextprop(globalconf.display, c->win,
                     XInternAtom(globalconf.display, "WM_NAME", False), c->name, sizeof(c->name));

    client_updatetitlebar(c);

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

static void
client_unfocus(Client *c)
{
    if(globalconf.screens[c->screen].opacity_unfocused != -1)
        window_settrans(c->win, globalconf.screens[c->screen].opacity_unfocused);
    focus_add_client(NULL);
    XSetWindowBorder(globalconf.display, c->win,
                     globalconf.screens[c->screen].styles.normal.border.pixel);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    client_updatetitlebar(c);
}

/** Ban client and unmap it
 * \param c the client
 */
void
client_ban(Client *c)
{
    if(globalconf.focus->client == c)
        client_unfocus(c);
    XUnmapWindow(globalconf.display, c->win);
    window_setstate(c->win, IconicState);
    if(c->titlebar)
        XUnmapWindow(globalconf.display, c->titlebar->window);
}

/** Give focus to client, or to first client if c is NULL
 * \param c client
 * \param screen Screen ID
 */
void
client_focus(Client *c, int screen, Bool raise)
{
    int phys_screen = get_phys_screen(screen);

    /* if c is NULL or invisible, take next client in the focus history */
    if(!c || (c && !client_isvisible(c, screen)))
    {
        c = focus_get_current_client(screen);
        /* if c is still NULL take next client in the stack */
        if(!c)
            for(c = globalconf.clients; c && (c->skip || !client_isvisible(c, screen)); c = c->next);
    }

    /* unfocus current selected client */
    if(globalconf.focus->client)
        client_unfocus(globalconf.focus->client);

    if(c)
    {
        /* unban the client before focusing or it will fail */
        client_unban(c);
        /* save sel in focus history */
        focus_add_client(c);
        if(globalconf.screens[c->screen].opacity_unfocused != -1)
            window_settrans(c->win, -1);
        XSetWindowBorder(globalconf.display, c->win,
                         globalconf.screens[screen].styles.focus.border.pixel);
        client_updatetitlebar(c);
        XSetInputFocus(globalconf.display, c->win, RevertToPointerRoot, CurrentTime);
        if(raise)
        {
            XWindowChanges wc;
            Layout *curlay = layout_get_current(screen);
            if(c->isfloating || curlay->arrange == layout_floating)
            {
                XRaiseWindow(globalconf.display, c->win);
                if(c->titlebar)
                    XRaiseWindow(globalconf.display, c->titlebar->window);
            }
            else
            {
                Client *client;
                wc.stack_mode = Below;
                wc.sibling = None;
                for(client = globalconf.clients; client; client = client->next)
                    if(client != c && client_isvisible(client, c->screen) && client->isfloating)
                    {
                        if(client->titlebar)
                        {
                            XConfigureWindow(globalconf.display, client->titlebar->window,
                                             CWSibling | CWStackMode, &wc);
                            wc.sibling = client->titlebar->window;
                        }
                        XConfigureWindow(globalconf.display, client->win, CWSibling | CWStackMode, &wc);
                        wc.sibling = client->win;
                    }
                if(c->titlebar)
                {
                     XConfigureWindow(globalconf.display, c->titlebar->window,
                                      CWSibling | CWStackMode, &wc);
                     wc.sibling = c->titlebar->window;
                }
                XConfigureWindow(globalconf.display, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
                for(client = globalconf.clients; client; client = client->next)
                    if(client != c && IS_TILED(client, c->screen))
                    {
                        if(client->titlebar)
                        {
                            XConfigureWindow(globalconf.display, client->titlebar->window,
                                             CWSibling | CWStackMode, &wc);
                            wc.sibling = client->titlebar->window;
                        }
                        XConfigureWindow(globalconf.display, client->win, CWSibling | CWStackMode, &wc);
                        wc.sibling = client->win;
                    }
            }
        }
        /* since we're dropping EnterWindow events and sometimes the window
         * will appear under the mouse, grabbuttons */
        window_grabbuttons(phys_screen, c->win);
    }
    else
        XSetInputFocus(globalconf.display,
                       RootWindow(globalconf.display, phys_screen),
                       RevertToPointerRoot, CurrentTime);

    widget_invalidate_cache(screen, WIDGET_CACHE_CLIENTS);
    ewmh_update_net_active_window(phys_screen);
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
    Bool rettrans, retloadprops;
    XWindowChanges wc;
    Tag *tag;
    Rule *rule;
    area_t screen_geom;
    int phys_screen = get_phys_screen(screen), titlebar_height;
    long flags;

    c = p_new(Client, 1);

    c->screen = screen_get_bycoord(globalconf.screens_info, screen, wa->x, wa->y);

    screen_geom = screen_get_area(c->screen,
                                  globalconf.screens[screen].statusbar,
                                  &globalconf.screens[screen].padding);
    /* Initial values */
    c->win = w;
    c->geometry.x = c->f_geometry.x = c->m_geometry.x = MAX(wa->x, screen_geom.x);
    c->geometry.y = c->f_geometry.y = c->m_geometry.y = MAX(wa->y, screen_geom.y);
    c->geometry.width = c->f_geometry.width = c->m_geometry.width = wa->width;
    c->geometry.height = c->f_geometry.height = c->m_geometry.height = wa->height;
    c->oldborder = wa->border_width;
    c->newcomer = True;

    c->border = globalconf.screens[screen].borderpx;

    /* Set windows borders */
    wc.border_width = c->border;
    XConfigureWindow(globalconf.display, w, CWBorderWidth, &wc);
    XSetWindowBorder(globalconf.display, w,
                     globalconf.screens[screen].styles.normal.border.pixel);
    /* propagates border_width, if size doesn't change */
    window_configure(c->win, c->geometry, c->border);

    switch(globalconf.screens[c->screen].titlebar)
    {
      case Top:
        titlebar_height = 1.5 * MAX(globalconf.screens[c->screen].styles.normal.font->height,
                                    MAX(globalconf.screens[c->screen].styles.focus.font->height,
                                        globalconf.screens[c->screen].styles.urgent.font->height)),
        c->titlebar = simplewindow_new(globalconf.display,
                                       phys_screen,
                                       c->geometry.x,
                                       c->geometry.y - titlebar_height,
                                       c->geometry.width,
                                       titlebar_height,
                                       0);
        break;
      default:
        break;
    }
    /* update window title */
    client_updatetitle(c);

    /* update hints */
    flags = client_updatesizehints(c);
    client_updatewmhints(c);

    /* Try to load props if any */
    retloadprops = client_loadprops(c, screen);

    /* Then check clients hints */
    ewmh_check_client_hints(c);

    /* Then apply rules if no props */
    if(!retloadprops)
    {
        /* Get the client's rule */
        if((rule = rule_matching_client(c)))
        {
            if(rule->screen != RULE_NOSCREEN)
                move_client_to_screen(c, rule->screen, True);
            else
                move_client_to_screen(c, screen, True);
            tag_client_with_rule(c, rule);

            switch(rule->isfloating)
            {
              case Auto:
                break;
              case Yes:
                client_setfloating(c, True);
                break;
              case No:
                client_setfloating(c, False);
                break;
            }

            if(rule->opacity >= 0.0f)
                window_settrans(c->win, rule->opacity);
        }
        else
            move_client_to_screen(c, screen, True);
    }

    /* check for transient and set tags like its parent,
     * XGetTransientForHint returns 1 on success
     */
    if((rettrans = XGetTransientForHint(globalconf.display, w, &trans))
       && (t = client_get_bywin(globalconf.clients, trans)))
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            if(is_client_tagged(t, tag))
                tag_client(c, tag);

    /* should be floating if transsient or fixed */
    if(!c->isfloating)
        client_setfloating(c, rettrans || c->isfixed);

    if(!(flags & (USPosition | PPosition)))
        c->f_geometry =
            globalconf.screens[c->screen].floating_placement(c->f_geometry, c->border, c->screen);

    XSelectInput(globalconf.display, w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);

    /* handle xshape */
    if(globalconf.have_shape)
    {
        XShapeSelectInput(globalconf.display, w, ShapeNotifyMask);
        window_setshape(phys_screen, c->win);
    }

    /* attach to the stack */
    if((rule = rule_matching_client(c)))
        switch(rule->ismaster)
        {
          case Yes:
            client_list_push(&globalconf.clients, c);
            break;
          case No:
            client_list_append(&globalconf.clients, c);
            break;
          case Auto:
            rule = NULL;
            break;
        }

    if(!rule)
    {
        if(globalconf.screens[c->screen].new_become_master)
            client_list_push(&globalconf.clients, c);
        else
            client_list_append(&globalconf.clients, c);
    }

    /* some windows require this */
    XMoveResizeWindow(globalconf.display, c->win, c->geometry.x, c->geometry.y,
                      c->geometry.width, c->geometry.height);

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    ewmh_update_net_client_list(phys_screen);
}

static area_t
client_geometry_sizehint(Client *c, area_t geometry)
{
    double dx, dy, max, min, ratio;

    if(c->minay > 0 && c->maxay > 0 && (geometry.height - c->baseh) > 0
       && (geometry.width - c->basew) > 0)
    {
        dx = (double) (geometry.width - c->basew);
        dy = (double) (geometry.height - c->baseh);
        min = (double) (c->minax) / (double) (c->minay);
        max = (double) (c->maxax) / (double) (c->maxay);
        ratio = dx / dy;
        if(max > 0 && min > 0 && ratio > 0)
        {
            if(ratio < min)
            {
                dy = (dx * min + dy) / (min * min + 1);
                dx = dy * min;
                geometry.width = (int) dx + c->basew;
                geometry.height = (int) dy + c->baseh;
            }
            else if(ratio > max)
            {
                dy = (dx * min + dy) / (max * max + 1);
                dx = dy * min;
                geometry.width = (int) dx + c->basew;
                geometry.height = (int) dy + c->baseh;
            }
        }
    }
    if(c->minw && geometry.width < c->minw)
        geometry.width = c->minw;
    if(c->minh && geometry.height < c->minh)
        geometry.height = c->minh;
    if(c->maxw && geometry.width > c->maxw)
        geometry.width = c->maxw;
    if(c->maxh && geometry.height > c->maxh)
        geometry.height = c->maxh;
    if(c->incw)
        geometry.width -= (geometry.width - c->basew) % c->incw;
    if(c->inch)
        geometry.height -= (geometry.height - c->baseh) % c->inch;

    return geometry;
}

/** Resize client window
 * \param c client to resize
 * \param geometry new window geometry
 * \param sizehints respect size hints
 * \param return True if resize has been done
 */
Bool
client_resize(Client *c, area_t geometry, Bool sizehints)
{
    int new_screen;
    area_t area;
    XWindowChanges wc;

    if(c->titlebar)
    {
        geometry.y += c->titlebar->geometry.height;
        geometry.height -= c->titlebar->geometry.height;
    }

    if(sizehints)
        geometry = client_geometry_sizehint(c, geometry);

    if(geometry.width <= 0 || geometry.height <= 0)
        return False;

    /* offscreen appearance fixes */
    area = get_display_area(get_phys_screen(c->screen),
                            NULL,
                            &globalconf.screens[c->screen].padding);
    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width - 2 * c->border;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height - 2 * c->border;
    if(geometry.x + geometry.width + 2 * c->border < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height + 2 * c->border < 0)
        geometry.y = 0;

    if(c->geometry.x != geometry.x || c->geometry.y != geometry.y
       || c->geometry.width != geometry.width || c->geometry.height != geometry.height)
    {
        new_screen = screen_get_bycoord(globalconf.screens_info, c->screen, geometry.x, geometry.y);

        c->geometry.x = wc.x = geometry.x;
        c->geometry.width = wc.width = geometry.width;
        c->geometry.y = wc.y = geometry.y;
        c->geometry.height = wc.height = geometry.height;

        if(c->titlebar)
        {
            simplewindow_move_resize(c->titlebar,
                                     geometry.x,
                                     geometry.y - c->titlebar->geometry.height,
                                     geometry.width,
                                     c->titlebar->geometry.height);
            client_updatetitlebar(c);

            c->geometry.y -= c->titlebar->geometry.height;
            c->geometry.height += c->titlebar->geometry.height;
        }

        /* save the floating geometry if the window is floating but not
         * maximized */
        if((c->isfloating ||
           layout_get_current(new_screen)->arrange == layout_floating) && !c->ismax)
            c->f_geometry = geometry;

        printf("moving client %s to %d\n", c->name, c->geometry.y);

        XConfigureWindow(globalconf.display, c->win,
                         CWX | CWY | CWWidth | CWHeight, &wc);
        window_configure(c->win, geometry, c->border);

        if(c->screen != new_screen)
            move_client_to_screen(c, new_screen, False);

        return True;
    }
    return False;
}

void
client_setfloating(Client *c, Bool floating)
{
    if(c->isfloating != floating)
    {
        if((c->isfloating = floating))
        {
            client_resize(c, c->f_geometry, False);
            XRaiseWindow(globalconf.display, c->win);
        }
        else
        {
            XLowerWindow(globalconf.display, c->win);
            if(c->ismax)
            {
                c->ismax = False;
                client_resize(c, c->m_geometry, False);
            }
        }
        if(client_isvisible(c, c->screen))
            globalconf.screens[c->screen].need_arrange = True;
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_saveprops(c);
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

    XChangeProperty(globalconf.display, c->win,
                    XInternAtom(globalconf.display, "_AWESOME_PROPERTIES", False),
                    XA_STRING, 8, PropModeReplace, (unsigned char *) prop, i);

    p_delete(&prop);
}

void
client_unban(Client *c)
{
    XMapWindow(globalconf.display, c->win);
    window_setstate(c->win, NormalState);
    if(c->titlebar)
        XMapWindow(globalconf.display, c->titlebar->window);
}

void
client_unmanage(Client *c)
{
    XWindowChanges wc;
    Tag *tag;

    wc.border_width = c->oldborder;

    /* The server grab construct avoids race conditions. */
    XGrabServer(globalconf.display);

    XConfigureWindow(globalconf.display, c->win, CWBorderWidth, &wc);  /* restore border */

    /* remove client everywhere */
    client_list_detach(&globalconf.clients, c);
    focus_delete_client(c);
    if(globalconf.scratch.client == c)
        globalconf.scratch.client = NULL;
    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        untag_client(c, tag);

    if(globalconf.focus->client == c)
        client_focus(NULL, c->screen, True);

    XUngrabButton(globalconf.display, AnyButton, AnyModifier, c->win);
    window_setstate(c->win, WithdrawnState);

    XSync(globalconf.display, False);
    XUngrabServer(globalconf.display);

    if(c->titlebar)
        simplewindow_delete(c->titlebar);

    p_delete(&c);
}

void
client_updatewmhints(Client *c)
{
    XWMHints *wmh;

    if((wmh = XGetWMHints(globalconf.display, c->win)))
    {
        if((c->isurgent = (wmh->flags & XUrgencyHint)))
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        if((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
        {
            c->border = 0;
            c->skip = True;
        }
        XFree(wmh);
    }
}

long
client_updatesizehints(Client *c)
{
    long msize;
    XSizeHints size;

    if(!XGetWMNormalHints(globalconf.display, c->win, &size, &msize))
        return 0L;

    if(size.flags & PBaseSize)
    {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    }
    else if(size.flags & PMinSize)
    {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    }
    else
        c->basew = c->baseh = 0;
    if(size.flags & PResizeInc)
    {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    }
    else
        c->incw = c->inch = 0;

    if(size.flags & PMaxSize)
    {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    }
    else
        c->maxw = c->maxh = 0;

    if(size.flags & PMinSize)
    {
        c->minw = size.min_width;
        c->minh = size.min_height;
    }
    else if(size.flags & PBaseSize)
    {
        c->minw = size.base_width;
        c->minh = size.base_height;
    }
    else
        c->minw = c->minh = 0;

    if(size.flags & PAspect)
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

    return size.flags;
}

/** Returns True if a client is tagged
 * with one of the tags
 * \return True or False
 */
Bool
client_isvisible(Client *c, int screen)
{
    Tag *tag;

    if(!c || c->screen != screen)
        return False;

    if(globalconf.scratch.client == c)
        return globalconf.scratch.isvisible;

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

static Client *
client_find_visible(Client *sel, Bool reverse)
{
    Client *next;
    Client *(*client_iter)(Client **, Client *) = client_list_next_cycle;

    if(!sel) return NULL;

    if(reverse)
        client_iter = client_list_prev_cycle;

    /* look for previous or next starting at sel */

    for(next = client_iter(&globalconf.clients, sel);
        next && (next->skip || !client_isvisible(next, sel->screen));
        next = client_iter(&globalconf.clients, next));

    return next;
}

/** Swap current with previous client
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapprev(int screen __attribute__ ((unused)),
                     char *arg __attribute__ ((unused)))
{
    Client *prev;

    if((prev = client_find_visible(globalconf.focus->client, True)))
    {
        client_list_swap(&globalconf.clients, prev, globalconf.focus->client);
        globalconf.screens[prev->screen].need_arrange = True;
    }
}

/** Swap current with next client
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapnext(int screen __attribute__ ((unused)),
                     char *arg __attribute__ ((unused)))
{
    Client *next;

    if((next = client_find_visible(globalconf.focus->client, False)))
    {
        client_list_swap(&globalconf.clients, globalconf.focus->client, next);
        globalconf.screens[next->screen].need_arrange = True;
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
    int ox, oy, ow, oh; /* old geometry */
    char x[8], y[8], w[8], h[8];
    int mx, my, dx, dy, nmx, nmy;
    unsigned int dui;
    Window dummy;
    area_t area;
    Client *sel = globalconf.focus->client;
    Layout *curlay = layout_get_current(screen);

    if(!sel || sel->isfixed || !arg ||
       (curlay->arrange != layout_floating && !sel->isfloating))
        return;

    if(sscanf(arg, "%s %s %s %s", x, y, w, h) != 4)
        return;

    area.x = (int) compute_new_value_from_arg(x, sel->geometry.x);
    area.y = (int) compute_new_value_from_arg(y, sel->geometry.y);
    area.width = (int) compute_new_value_from_arg(w, sel->geometry.width);
    area.height = (int) compute_new_value_from_arg(h, sel->geometry.height);

    ox = sel->geometry.x;
    oy = sel->geometry.y;
    ow = sel->geometry.width;
    oh = sel->geometry.height;

    Bool xqp = XQueryPointer(globalconf.display,
                             RootWindow(globalconf.display,
                                        get_phys_screen(screen)),
                             &dummy, &dummy, &mx, &my, &dx, &dy, &dui);
    client_resize(sel, area, globalconf.screens[sel->screen].resize_hints);
    if (xqp && ox <= mx && (ox + 2 * sel->border + ow) >= mx &&
        oy <= my && (oy + 2 * sel->border + oh) >= my)
    {
        nmx = mx - (ox + sel->border) + sel->geometry.width - ow;
        nmy = my - (oy + sel->border) + sel->geometry.height - oh;

        if(nmx < -sel->border) /* can happen on a resize */
            nmx = -sel->border;
        if(nmy < -sel->border)
            nmy = -sel->border;

        XWarpPointer(globalconf.display,
                     None, sel->win,
                     0, 0, 0, 0, nmx, nmy);
    }
}

void
client_kill(Client *c)
{
    XEvent ev;

    if(client_isprotodel(globalconf.display, c->win))
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

static void
client_maximize(Client *c, area_t geometry)
{

    if((c->ismax = !c->ismax))
    {
        c->wasfloating = c->isfloating;
        c->m_geometry = c->geometry;
        if(layout_get_current(c->screen)->arrange != layout_floating)
            client_setfloating(c, True);
        client_focus(c, c->screen, True);
        client_resize(c, geometry, False);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    }
    else if(c->wasfloating)
    {
        client_setfloating(c, True);
        client_resize(c, c->m_geometry, False);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    }
    else if(layout_get_current(c->screen)->arrange == layout_floating)
    {
        client_resize(c, c->m_geometry, False);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    }
    else
    {
        client_setfloating(c, False);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    }
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
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.width -= 2 * sel->border;
        area.height -= 2 * sel->border;
        client_maximize(sel, area);
    }
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
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.x = sel->geometry.x;
        area.width = sel->geometry.width;
        area.height -= 2 * sel->border;
        client_maximize(sel, area);
    }
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
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.y = sel->geometry.y;
        area.height = sel->geometry.height;
        area.width -= 2 * sel->border;
        client_maximize(sel, area);
    }
}

/** Zoom client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_zoom(int screen, char *arg __attribute__ ((unused)))
{
    Client *c, *sel = globalconf.focus->client;

    if(!sel)
        return;

    for(c = globalconf.clients; !client_isvisible(c, screen); c = c->next);
    if(c == sel)
         for(sel = sel->next; sel && !client_isvisible(sel, screen); sel = sel->next);

    if(sel)
    {
        client_list_detach(&globalconf.clients, sel);
        client_list_push(&globalconf.clients, sel);
        globalconf.screens[screen].need_arrange = True;
    }
}

/** Send focus to next client in stack
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusnext(int screen, char *arg __attribute__ ((unused)))
{
    Client *next;

    if((next = client_find_visible(globalconf.focus->client, False)))
        client_focus(next, screen, True);
}

/** Send focus to previous client in stack
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusprev(int screen, char *arg __attribute__ ((unused)))
{
    Client *prev;

    if((prev = client_find_visible(globalconf.focus->client, True)))
        client_focus(prev, screen, True);
}

/** Toggle floating state of a client
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_togglefloating(int screen __attribute__ ((unused)),
                           char *arg __attribute__ ((unused)))
{
    if(globalconf.focus->client)
        client_setfloating(globalconf.focus->client, !globalconf.focus->client->isfloating);
}

/** Toggle scratch client attribute
 * \param screen screen number
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_setscratch(int screen,
                       char *arg __attribute__ ((unused)))
{
    if(!globalconf.focus->client)
        return;

    if(globalconf.scratch.client == globalconf.focus->client)
        globalconf.scratch.client = NULL;
    else
        globalconf.scratch.client = globalconf.focus->client;

    widget_invalidate_cache(screen, WIDGET_CACHE_CLIENTS | WIDGET_CACHE_TAGS);
    globalconf.screens[screen].need_arrange = True;
}

/** Toggle scratch client visibility
 * \param screen screen number
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_togglescratch(int screen,
                          char *arg __attribute__ ((unused)))
{
    if(globalconf.scratch.client)
    {
        globalconf.scratch.isvisible = !globalconf.scratch.isvisible;
        if(globalconf.scratch.isvisible)
            client_focus(globalconf.scratch.client, screen, True);
        globalconf.screens[globalconf.scratch.client->screen].need_arrange = True;
        widget_invalidate_cache(globalconf.scratch.client->screen, WIDGET_CACHE_CLIENTS);
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
