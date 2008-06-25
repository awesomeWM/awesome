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
#include <xcb/xcb_aux.h>

#include "ewmh.h"
#include "tag.h"
#include "focus.h"
#include "screen.h"
#include "client.h"
#include "widget.h"
#include "cnode.h"
#include "titlebar.h"

extern awesome_t globalconf;

static xcb_atom_t net_supported;
static xcb_atom_t net_client_list;
static xcb_atom_t net_client_list_stacking;
static xcb_atom_t net_number_of_desktops;
static xcb_atom_t net_current_desktop;
static xcb_atom_t net_desktop_names;
static xcb_atom_t net_active_window;
static xcb_atom_t net_workarea;
static xcb_atom_t net_supporting_wm_check;
static xcb_atom_t net_close_window;
static xcb_atom_t net_wm_name;
static xcb_atom_t net_wm_visible_name;
static xcb_atom_t net_wm_desktop;
static xcb_atom_t net_wm_icon_name;
static xcb_atom_t net_wm_visible_icon_name;
static xcb_atom_t net_wm_window_type;
static xcb_atom_t net_wm_window_type_normal;
static xcb_atom_t net_wm_window_type_desktop;
static xcb_atom_t net_wm_window_type_dock;
static xcb_atom_t net_wm_window_type_splash;
static xcb_atom_t net_wm_window_type_dialog;
static xcb_atom_t net_wm_icon;
static xcb_atom_t net_wm_pid;
static xcb_atom_t net_wm_state;
static xcb_atom_t net_wm_state_sticky;
static xcb_atom_t net_wm_state_skip_taskbar;
static xcb_atom_t net_wm_state_fullscreen;
static xcb_atom_t net_wm_state_above;
static xcb_atom_t net_wm_state_below;
static xcb_atom_t net_wm_state_modal;
static xcb_atom_t net_wm_state_hidden;
static xcb_atom_t net_wm_state_demands_attention;

static xcb_atom_t utf8_string;

typedef struct
{
    const char *name;
    xcb_atom_t *atom;
} AtomItem;

static AtomItem AtomNames[] =
{
    { "_NET_SUPPORTED", &net_supported },
    { "_NET_CLIENT_LIST", &net_client_list },
    { "_NET_CLIENT_LIST_STACKING", &net_client_list_stacking },
    { "_NET_NUMBER_OF_DESKTOPS", &net_number_of_desktops },
    { "_NET_CURRENT_DESKTOP", &net_current_desktop },
    { "_NET_DESKTOP_NAMES", &net_desktop_names },
    { "_NET_ACTIVE_WINDOW", &net_active_window },
    { "_NET_WORKAREA", &net_workarea },
    { "_NET_SUPPORTING_WM_CHECK", &net_supporting_wm_check },

    { "_NET_CLOSE_WINDOW", &net_close_window },

    { "_NET_WM_NAME", &net_wm_name },
    { "_NET_WM_VISIBLE_NAME", &net_wm_visible_name },
    { "_NET_WM_DESKTOP", &net_wm_desktop },
    { "_NET_WM_ICON_NAME", &net_wm_icon_name },
    { "_NET_WM_VISIBLE_ICON_NAME", &net_wm_visible_icon_name },
    { "_NET_WM_WINDOW_TYPE", &net_wm_window_type },
    { "_NET_WM_WINDOW_TYPE_NORMAL", &net_wm_window_type_normal },
    { "_NET_WM_WINDOW_TYPE_DESKTOP", &net_wm_window_type_desktop },
    { "_NET_WM_WINDOW_TYPE_DOCK", &net_wm_window_type_dock },
    { "_NET_WM_WINDOW_TYPE_SPLASH", &net_wm_window_type_splash },
    { "_NET_WM_WINDOW_TYPE_DIALOG", &net_wm_window_type_dialog },
    { "_NET_WM_ICON", &net_wm_icon },
    { "_NET_WM_PID", &net_wm_pid },
    { "_NET_WM_STATE", &net_wm_state },
    { "_NET_WM_STATE_STICKY", &net_wm_state_sticky },
    { "_NET_WM_STATE_SKIP_TASKBAR", &net_wm_state_skip_taskbar },
    { "_NET_WM_STATE_FULLSCREEN", &net_wm_state_fullscreen },
    { "_NET_WM_STATE_ABOVE", &net_wm_state_above },
    { "_NET_WM_STATE_BELOW", &net_wm_state_below },
    { "_NET_WM_STATE_MODAL", &net_wm_state_modal },
    { "_NET_WM_STATE_HIDDEN", &net_wm_state_hidden },
    { "_NET_WM_STATE_DEMANDS_ATTENTION", &net_wm_state_demands_attention },

    { "UTF8_STRING", &utf8_string },
};

#define ATOM_NUMBER (sizeof(AtomNames)/sizeof(AtomItem))

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

void
ewmh_init_atoms(void)
{
    unsigned int i;
    xcb_intern_atom_cookie_t cs[ATOM_NUMBER];
    xcb_intern_atom_reply_t *r;

    /*
     * Create the atom and get the reply in a XCB way (e.g. send all
     * the requests at the same time and then get the replies)
     */
    for(i = 0; i < ATOM_NUMBER; i++)
	cs[i] = xcb_intern_atom_unchecked(globalconf.connection,
					  false,
					  strlen(AtomNames[i].name),
					  AtomNames[i].name);

    for(i = 0; i < ATOM_NUMBER; i++)
    {
	if(!(r = xcb_intern_atom_reply(globalconf.connection, cs[i], NULL)))
	    /* An error occured, get reply for next atom */
	    continue;

	*AtomNames[i].atom = r->atom;
        p_delete(&r);
    }
}

void
ewmh_set_supported_hints(int phys_screen)
{
    xcb_atom_t atom[ATOM_NUMBER];
    xcb_window_t father;
    xcb_screen_t *xscreen = xutil_screen_get(globalconf.connection, phys_screen);
    int i = 0;

    atom[i++] = net_supported;
    atom[i++] = net_supporting_wm_check;
    atom[i++] = net_client_list;
    atom[i++] = net_client_list_stacking;
    atom[i++] = net_number_of_desktops;
    atom[i++] = net_current_desktop;
    atom[i++] = net_desktop_names;
    atom[i++] = net_active_window;
    atom[i++] = net_workarea;

    atom[i++] = net_close_window;

    atom[i++] = net_wm_name;
    atom[i++] = net_wm_icon_name;
    atom[i++] = net_wm_visible_icon_name;
    atom[i++] = net_wm_desktop;
    atom[i++] = net_wm_window_type;
    atom[i++] = net_wm_window_type_normal;
    atom[i++] = net_wm_window_type_desktop;
    atom[i++] = net_wm_window_type_dock;
    atom[i++] = net_wm_window_type_splash;
    atom[i++] = net_wm_window_type_dialog;
    atom[i++] = net_wm_icon;
    atom[i++] = net_wm_pid;
    atom[i++] = net_wm_state;
    atom[i++] = net_wm_state_sticky;
    atom[i++] = net_wm_state_skip_taskbar;
    atom[i++] = net_wm_state_fullscreen;
    atom[i++] = net_wm_state_above;
    atom[i++] = net_wm_state_below;
    atom[i++] = net_wm_state_modal;
    atom[i++] = net_wm_state_hidden;
    atom[i++] = net_wm_state_demands_attention;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, net_supported, ATOM, 32, i, atom);

    /* create our own window */
    father = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, xscreen->root_depth,
                      father, xscreen->root, -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, xscreen->root_visual, 0, NULL);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, net_supporting_wm_check, WINDOW, 32,
                        1, &father);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, net_supporting_wm_check, WINDOW, 32,
                        1, &father);

    /* set the window manager name */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, net_wm_name, utf8_string, 8, 7, "awesome");

    /* set the window manager PID */
    i = getpid();
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, net_wm_pid, CARDINAL, 32, 1, &i);
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
			net_client_list, WINDOW, 32, n, wins);

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
			net_client_list_stacking, WINDOW, 32, n, wins);

    p_delete(&wins);
}

void
ewmh_update_net_numbers_of_desktop(int phys_screen)
{
    uint32_t count = globalconf.screens[phys_screen].tags.len;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			net_number_of_desktops, CARDINAL, 32, 1, &count);
}

void
ewmh_update_net_current_desktop(int phys_screen)
{
    tag_array_t *tags = &globalconf.screens[phys_screen].tags;
    uint32_t count = 0;
    tag_t **curtags = tags_get_current(phys_screen);

    for(count = 0; tags->tab[count] != curtags[0]; count++);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xutil_screen_get(globalconf.connection, phys_screen)->root,
                        net_current_desktop, CARDINAL, 32, 1, &count);

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
			net_desktop_names, utf8_string, 8, buf.len, buf.s);
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
    area_t geom = screen_area_get(phys_screen,
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
                        net_workarea, CARDINAL, 32, tags->len * 4, area);
}

void
ewmh_update_net_active_window(int phys_screen)
{
    xcb_window_t win;
    client_t *sel = focus_get_current_client(phys_screen);

    win = sel ? sel->win : XCB_NONE;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			net_active_window, WINDOW, 32, 1, &win);
}

static void
ewmh_process_state_atom(client_t *c, xcb_atom_t state, int set)
{
    static uint32_t const raise_window_val = XCB_STACK_MODE_ABOVE;

    if(state == net_wm_state_sticky)
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;
        for(int i = 0; i < tags->len; i++)
            tag_client(c, tags->tab[i]);
    }
    else if(state == net_wm_state_skip_taskbar)
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
    else if(state == net_wm_state_fullscreen)
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
            client_setborder(c, c->oldborder);
            client_setfloating(c, c->wasfloating, c->oldlayer);
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            geometry = screen_area_get(c->screen, NULL, &globalconf.screens[c->screen].padding);
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
            client_setfloating(c, true, LAYER_FULLSCREEN);
        }
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_resize(c, geometry, false);
    }
    else if(state == net_wm_state_above)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->layer = c->oldlayer;
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            c->layer = LAYER_ABOVE;
        }
    }
    else if(state == net_wm_state_below)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->layer = c->oldlayer;
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            c->layer = LAYER_BELOW;
        }
    }
    else if(state == net_wm_state_modal)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->layer = c->oldlayer;
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            c->layer = LAYER_MODAL;
        }
    }
    else if(state == net_wm_state_hidden)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->ishidden = false;
        else if(set == _NET_WM_STATE_ADD)
            c->ishidden = true;
        globalconf.screens[c->screen].need_arrange = true;
    }
    else if(state == net_wm_state_demands_attention)
    {
        if(set == _NET_WM_STATE_REMOVE)
            c->isurgent = false;
        else if(set == _NET_WM_STATE_ADD)
        {
            c->isurgent = true;
            /* execute hook */
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.urgent, 1);
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        }
    }
}

static void
ewmh_process_window_type_atom(client_t *c, xcb_atom_t state)
{
    if(state == net_wm_window_type_normal)
    {
        /* do nothing. this is REALLY IMPORTANT */
    }
    else if(state == net_wm_window_type_dock
            || state == net_wm_window_type_splash)
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
        client_setfloating(c, true, LAYER_ABOVE);
    }
    else if(state == net_wm_window_type_dialog)
        client_setfloating(c, true, LAYER_MODAL);
    else if(state == net_wm_window_type_desktop)
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;
        c->noborder = true;
        c->isfixed = true;
        c->skip = true;
        c->layer = LAYER_DESKTOP;
        for(int i = 0; i < tags->len; i++)
            tag_client(c, tags->tab[i]);
    }
}

int
ewmh_process_client_message(xcb_client_message_event_t *ev)
{
    client_t *c;
    int screen;

    if(ev->type == net_current_desktop)
        for(screen = 0;
            screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
            screen++)
        {
            if(ev->window == xutil_screen_get(globalconf.connection, screen)->root)
                tag_view_only_byindex(screen, ev->data.data32[0]);
        }
    else if(ev->type == net_close_window)
    {
        if((c = client_getbywin(ev->window)))
           client_kill(c);
    }
    else if(ev->type == net_wm_desktop)
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
    else if(ev->type == net_wm_state)
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
                                    net_wm_desktop, XCB_GET_PROPERTY_TYPE_ANY, 0, 1);

    c1 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    net_wm_state, ATOM, 0, UINT32_MAX);

    c2 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    net_wm_window_type, ATOM, 0, UINT32_MAX);

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

netwm_icon_t *
ewmh_get_window_icon(xcb_window_t w)
{
    double alpha;
    netwm_icon_t *icon;
    int size, i;
    uint32_t *data;
    unsigned char *imgdata;
    xcb_get_property_reply_t *r;

    r = xcb_get_property_reply(globalconf.connection,
                               xcb_get_property_unchecked(globalconf.connection, false, w,
                                                          net_wm_icon, CARDINAL, 0, UINT32_MAX),
                               NULL);
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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
