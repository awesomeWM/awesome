/*
 * ewmh.c - EWMH support functions
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

#include <sys/types.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "ewmh.h"
#include "tag.h"
#include "screen.h"
#include "client.h"
#include "widget.h"
#include "cnode.h"
#include "titlebar.h"
#include "common/atoms.h"

extern awesome_t globalconf;

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

void
ewmh_init(int phys_screen)
{
    xcb_window_t father;
    xcb_screen_t *xscreen = xutil_screen_get(globalconf.connection, phys_screen);
    xcb_atom_t atom[] =
    {
        _NET_SUPPORTED,
        _NET_SUPPORTING_WM_CHECK,
        _NET_CLIENT_LIST,
        _NET_CLIENT_LIST_STACKING,
        _NET_NUMBER_OF_DESKTOPS,
        _NET_CURRENT_DESKTOP,
        _NET_DESKTOP_NAMES,
        _NET_ACTIVE_WINDOW,
        _NET_WORKAREA,
        _NET_CLOSE_WINDOW,
        _NET_WM_NAME,
        _NET_WM_ICON_NAME,
        _NET_WM_VISIBLE_ICON_NAME,
        _NET_WM_DESKTOP,
        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_NORMAL,
        _NET_WM_WINDOW_TYPE_DESKTOP,
        _NET_WM_WINDOW_TYPE_DOCK,
        _NET_WM_WINDOW_TYPE_SPLASH,
        _NET_WM_WINDOW_TYPE_DIALOG,
        _NET_WM_ICON,
        _NET_WM_PID,
        _NET_WM_STATE,
        _NET_WM_STATE_STICKY,
        _NET_WM_STATE_SKIP_TASKBAR,
        _NET_WM_STATE_FULLSCREEN,
        _NET_WM_STATE_ABOVE,
        _NET_WM_STATE_BELOW,
        _NET_WM_STATE_MODAL,
        _NET_WM_STATE_HIDDEN,
        _NET_WM_STATE_DEMANDS_ATTENTION
    };
    int i;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, _NET_SUPPORTED, ATOM, 32,
                        countof(atom), atom);

    /* create our own window */
    father = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, xscreen->root_depth,
                      father, xscreen->root, -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, xscreen->root_visual, 0, NULL);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
                        1, &father);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
                        1, &father);

    /* set the window manager name */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_NAME, UTF8_STRING, 8, 7, "awesome");

    /* set the window manager PID */
    i = getpid();
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_PID, CARDINAL, 32, 1, &i);
}

void
ewmh_update_net_client_list(int phys_screen)
{
    xcb_window_t *wins;
    client_t *c;
    int n = 0;

    for(c = globalconf.clients; c; c = c->next)
        n++;

    wins = p_new(xcb_window_t, n);

    for(n = 0, c = globalconf.clients; c; c = c->next, n++)
        if(c->phys_screen == phys_screen)
            wins[n] = c->win;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_CLIENT_LIST, WINDOW, 32, n, wins);

    p_delete(&wins);
}

/** Set the client list in stacking order, bottom to top.
 * \param phys_screen The physical screen id.
 */
void
ewmh_update_net_client_list_stacking(int phys_screen)
{
    xcb_window_t *wins;
    client_node_t *c;
    int n = 0;

    for(c = globalconf.stack; c; c = c->next)
        n++;

    wins = p_new(xcb_window_t, n);

    for(n = 0, c = *client_node_list_last(&globalconf.stack); c; c = c->prev, n++)
        if(c->client->phys_screen == phys_screen)
            wins[n] = c->client->win;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_CLIENT_LIST_STACKING, WINDOW, 32, n, wins);

    p_delete(&wins);
}

void
ewmh_update_net_numbers_of_desktop(int phys_screen)
{
    uint32_t count = globalconf.screens[phys_screen].tags.len;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_NUMBER_OF_DESKTOPS, CARDINAL, 32, 1, &count);
}

void
ewmh_update_net_current_desktop(int phys_screen)
{
    tag_array_t *tags = &globalconf.screens[phys_screen].tags;
    uint32_t count = 0;
    tag_t **curtags = tags_get_current(phys_screen);

    while(count < (uint32_t) tags->len && tags->tab[count] != curtags[0])
        count++;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xutil_screen_get(globalconf.connection, phys_screen)->root,
                        _NET_CURRENT_DESKTOP, CARDINAL, 32, 1, &count);

    p_delete(&curtags);
}

void
ewmh_update_net_desktop_names(int phys_screen)
{
    tag_array_t *tags = &globalconf.screens[phys_screen].tags;
    buffer_t buf;

    buffer_inita(&buf, BUFSIZ);

    for(int i = 0; i < tags->len; i++)
    {
        buffer_adds(&buf, tags->tab[i]->name);
        buffer_addc(&buf, '\0');
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_DESKTOP_NAMES, UTF8_STRING, 8, buf.len, buf.s);
    buffer_wipe(&buf);
}

/** Update the work area space for each physical screen and each desktop.
 * \param phys_screen The physical screen id.
 */
void
ewmh_update_workarea(int phys_screen)
{
    tag_array_t *tags = &globalconf.screens[phys_screen].tags;
    uint32_t *area = p_alloca(uint32_t, tags->len * 4);
    area_t geom = screen_area_get(&globalconf.screens[phys_screen].geometry,
                                  globalconf.screens[phys_screen].statusbar,
                                  &globalconf.screens[phys_screen].padding);


    for(int i = 0; i < tags->len; i++)
    {
        area[4 * i + 0] = geom.x;
        area[4 * i + 1] = geom.y;
        area[4 * i + 2] = geom.width;
        area[4 * i + 3] = geom.height;
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xutil_screen_get(globalconf.connection, phys_screen)->root,
                        _NET_WORKAREA, CARDINAL, 32, tags->len * 4, area);
}

void
ewmh_update_net_active_window(int phys_screen)
{
    xcb_window_t win;

    if(globalconf.screen_focus->client_focus
       && globalconf.screen_focus->client_focus->phys_screen == phys_screen)
        win = globalconf.screen_focus->client_focus->win;
    else
        win = XCB_NONE;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_ACTIVE_WINDOW, WINDOW, 32, 1, &win);
}

static void
ewmh_process_state_atom(client_t *c, xcb_atom_t state, int set)
{
    if(state == _NET_WM_STATE_STICKY)
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;
        for(int i = 0; i < tags->len; i++)
            tag_client(c, tags->tab[i]);
    }
    else if(state == _NET_WM_STATE_SKIP_TASKBAR)
    {
        if(set == _NET_WM_STATE_REMOVE)
        {
            c->skiptb = false;
            c->skip = false;
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            c->skiptb = true;
            c->skip = true;
        }
    }
    else if(state == _NET_WM_STATE_FULLSCREEN)
    {
        area_t geometry = c->geometry;
        if(set == _NET_WM_STATE_REMOVE)
        {
            /* restore geometry */
            geometry = c->m_geometry;
            /* restore borders and titlebar */
            if(c->titlebar && c->titlebar->sw && (c->titlebar->position = c->titlebar->oldposition))
                xcb_map_window(globalconf.connection, c->titlebar->sw->window);
            c->noborder = false;
            c->ismax = false;
            client_setlayer(c, c->oldlayer);
            client_setborder(c, c->oldborder);
            client_setfloating(c, c->wasfloating);
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            geometry = screen_area_get(&globalconf.screens[c->screen].geometry,
                                       NULL,
                                       &globalconf.screens[c->screen].padding);
            /* save geometry */
            c->m_geometry = c->geometry;
            c->wasfloating = c->isfloating;
            /* disable titlebar and borders */
            if(c->titlebar && c->titlebar->sw && (c->titlebar->oldposition = c->titlebar->position))
            {
                xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
                c->titlebar->position = Off;
            }
            c->ismax = true;
            c->oldborder = c->border;
            client_setborder(c, 0);
            c->noborder = true;
            c->oldlayer = c->layer;
            client_setlayer(c, LAYER_FULLSCREEN);
            client_setfloating(c, true);
        }
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_resize(c, geometry, false);
    }
    else if(state == _NET_WM_STATE_ABOVE)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_setlayer(c, c->oldlayer);
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            client_setlayer(c, LAYER_ABOVE);
        }
    }
    else if(state == _NET_WM_STATE_BELOW)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_setlayer(c, c->oldlayer);
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            client_setlayer(c, LAYER_BELOW);
        }
    }
    else if(state == _NET_WM_STATE_MODAL)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_setlayer(c, c->oldlayer);
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            client_setlayer(c, LAYER_MODAL);
        }
    }
    else if(state == _NET_WM_STATE_HIDDEN)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->ishidden = false;
        else if(set == _NET_WM_STATE_ADD)
            c->ishidden = true;
        globalconf.screens[c->screen].need_arrange = true;
    }
    else if(state == _NET_WM_STATE_DEMANDS_ATTENTION)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->isurgent = false;
        else if(set == _NET_WM_STATE_ADD)
        {
            c->isurgent = true;
            /* execute hook */
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.urgent, 1, 0);
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        }
    }
}

static void
ewmh_process_window_type_atom(client_t *c, xcb_atom_t state)
{
    if(state == _NET_WM_WINDOW_TYPE_NORMAL)
    {
        /* do nothing. this is REALLY IMPORTANT */
    }
    else if(state == _NET_WM_WINDOW_TYPE_DOCK
            || state == _NET_WM_WINDOW_TYPE_SPLASH)
    {
        c->skip = true;
        c->isfixed = true;
        if(c->titlebar && c->titlebar->position && c->titlebar->sw)
        {
            xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
            c->titlebar->position = Off;
        }
        client_setborder(c, 0);
        c->noborder = true;
        client_setlayer(c, LAYER_ABOVE);
        client_setfloating(c, true);
    }
    else if(state == _NET_WM_WINDOW_TYPE_DIALOG)
    {
        client_setlayer(c, LAYER_MODAL);
        client_setfloating(c, true);
    }
    else if(state == _NET_WM_WINDOW_TYPE_DESKTOP)
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;
        c->noborder = true;
        c->isfixed = true;
        c->skip = true;
        client_setlayer(c, LAYER_DESKTOP);
        for(int i = 0; i < tags->len; i++)
            tag_client(c, tags->tab[i]);
    }
}

int
ewmh_process_client_message(xcb_client_message_event_t *ev)
{
    client_t *c;
    int screen;

    if(ev->type == _NET_CURRENT_DESKTOP)
        for(screen = 0;
            screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
            screen++)
        {
            if(ev->window == xutil_screen_get(globalconf.connection, screen)->root)
                tag_view_only_byindex(screen, ev->data.data32[0]);
        }
    else if(ev->type == _NET_CLOSE_WINDOW)
    {
        if((c = client_getbywin(ev->window)))
           client_kill(c);
    }
    else if(ev->type == _NET_WM_DESKTOP)
    {
        if((c = client_getbywin(ev->window)))
        {
            tag_array_t *tags = &globalconf.screens[c->screen].tags;

            if(ev->data.data32[0] == 0xffffffff)
                for(int i = 0; i < tags->len; i++)
                    tag_client(c, tags->tab[i]);
            else
                for(int i = 0; i < tags->len; i++)
                    if((int)ev->data.data32[0] == i)
                        tag_client(c, tags->tab[i]);
                    else
                        untag_client(c, tags->tab[i]);
        }
    }
    else if(ev->type == _NET_WM_STATE)
    {
        if((c = client_getbywin(ev->window)))
        {
            ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[1], ev->data.data32[0]);
            if(ev->data.data32[2])
                ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[2],
                                        ev->data.data32[0]);
        }
    }

    return 0;
}

void
ewmh_check_client_hints(client_t *c)
{
    xcb_atom_t *state;
    void *data = NULL;
    int desktop;
    xcb_get_property_cookie_t c0, c1, c2;
    xcb_get_property_reply_t *reply;

    /* Send the GetProperty requests which will be processed later */
    c0 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    _NET_WM_DESKTOP, XCB_GET_PROPERTY_TYPE_ANY, 0, 1);

    c1 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    _NET_WM_STATE, ATOM, 0, UINT32_MAX);

    c2 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    _NET_WM_WINDOW_TYPE, ATOM, 0, UINT32_MAX);

    reply = xcb_get_property_reply(globalconf.connection, c0, NULL);
    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;

        desktop = *(uint32_t *) data;
        if(desktop == -1)
            for(int i = 0; i < tags->len; i++)
                tag_client(c, tags->tab[i]);
        else
            for(int i = 0; i < tags->len; i++)
                if(desktop == i)
                    tag_client(c, tags->tab[i]);
                else
                    untag_client(c, tags->tab[i]);
    }

    p_delete(&reply);

    reply = xcb_get_property_reply(globalconf.connection, c1, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(int i = 0; i < xcb_get_property_value_length(reply); i++)
            ewmh_process_state_atom(c, state[i], _NET_WM_STATE_ADD);
    }

    p_delete(&reply);

    reply = xcb_get_property_reply(globalconf.connection, c2, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(int i = 0; i < xcb_get_property_value_length(reply); i++)
            ewmh_process_window_type_atom(c, state[i]);
    }

    p_delete(&reply);
}

/** Send request to get NET_WM_ICON (EWMH)
 * \param w The window.
 * \return The cookie associated with the request.
 */
xcb_get_property_cookie_t
ewmh_window_icon_get_unchecked(xcb_window_t w)
{
  return xcb_get_property_unchecked(globalconf.connection, false, w,
                                    _NET_WM_ICON, CARDINAL, 0, UINT32_MAX);
}

/** Get NET_WM_ICON.
 * \param cookie The cookie.
 * \return A netwm_icon_t structure which must be deleted after usage.
 */
netwm_icon_t *
ewmh_window_icon_get_reply(xcb_get_property_cookie_t cookie)
{
    double alpha;
    netwm_icon_t *icon;
    int size, i;
    uint32_t *data;
    unsigned char *imgdata;
    xcb_get_property_reply_t *r;

    r = xcb_get_property_reply(globalconf.connection, cookie, NULL);
    if(!r || r->type != CARDINAL || r->format != 32 || r->length < 2 ||
       !(data = (uint32_t *) xcb_get_property_value(r)))
    {
        p_delete(&r);
        return NULL;
    }

    icon = p_new(netwm_icon_t, 1);

    icon->width = data[0];
    icon->height = data[1];
    size = icon->width * icon->height;

    if(!size)
    {
        p_delete(&icon);
        p_delete(&r);
        return NULL;
    }

    icon->image = p_new(unsigned char, size * 4);
    for(imgdata = icon->image, i = 2; i < size + 2; i++, imgdata += 4)
    {
        imgdata[3] = (data[i] >> 24) & 0xff;           /* A */
        alpha = imgdata[3] / 255.0;
        imgdata[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        imgdata[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        imgdata[0] = (data[i]         & 0xff) * alpha; /* B */
    }

    p_delete(&r);

    return icon;
}

/**
 * Restart awesome.
 */
void
ewmh_restart(void)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);
    xcb_disconnect(globalconf.connection);

    a_exec(globalconf.argv);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
