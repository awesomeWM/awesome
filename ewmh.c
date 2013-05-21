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

#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "ewmh.h"
#include "tag.h"
#include "focus.h"
#include "screen.h"
#include "client.h"
#include "widget.h"
#include "titlebar.h"

extern AwesomeConf globalconf;

static Atom net_supported;
static Atom net_client_list;
static Atom net_number_of_desktops;
static Atom net_current_desktop;
static Atom net_desktop_names;
static Atom net_active_window;

static Atom net_close_window;

static Atom net_wm_name;
static Atom net_wm_icon_name;
static Atom net_wm_window_type;
static Atom net_wm_window_type_normal;
static Atom net_wm_window_type_dock;
static Atom net_wm_window_type_splash;
static Atom net_wm_window_type_dialog;
static Atom net_wm_icon;
static Atom net_wm_state;
static Atom net_wm_state_sticky;
static Atom net_wm_state_skip_taskbar;
static Atom net_wm_state_fullscreen;
static Atom net_wm_state_demands_attention;

static Atom utf8_string;

typedef struct
{
    const char *name;
    Atom *atom;
} AtomItem;

static AtomItem AtomNames[] =
{
    { "_NET_SUPPORTED", &net_supported },
    { "_NET_CLIENT_LIST", &net_client_list },
    { "_NET_NUMBER_OF_DESKTOPS", &net_number_of_desktops },
    { "_NET_CURRENT_DESKTOP", &net_current_desktop },
    { "_NET_DESKTOP_NAMES", &net_desktop_names },
    { "_NET_ACTIVE_WINDOW", &net_active_window },

    { "_NET_CLOSE_WINDOW", &net_close_window },

    { "_NET_WM_NAME", &net_wm_name },
    { "_NET_WM_ICON_NAME", &net_wm_icon_name },
    { "_NET_WM_WINDOW_TYPE", &net_wm_window_type },
    { "_NET_WM_WINDOW_TYPE_NORMAL", &net_wm_window_type_normal },
    { "_NET_WM_WINDOW_TYPE_DOCK", &net_wm_window_type_dock },
    { "_NET_WM_WINDOW_TYPE_SPLASH", &net_wm_window_type_splash },
    { "_NET_WM_WINDOW_TYPE_DIALOG", &net_wm_window_type_dialog },
    { "_NET_WM_ICON", &net_wm_icon },
    { "_NET_WM_STATE", &net_wm_state },
    { "_NET_WM_STATE_STICKY", &net_wm_state_sticky },
    { "_NET_WM_STATE_SKIP_TASKBAR", &net_wm_state_skip_taskbar },
    { "_NET_WM_STATE_FULLSCREEN", &net_wm_state_fullscreen },
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
    char *names[ATOM_NUMBER];
    Atom atoms[ATOM_NUMBER];

    for(i = 0; i < ATOM_NUMBER; i++)
        names[i] = (char *) AtomNames[i].name;
    XInternAtoms(globalconf.display, names, ATOM_NUMBER, False, atoms);
    for(i = 0; i < ATOM_NUMBER; i++)
        *AtomNames[i].atom = atoms[i];
}

void
ewmh_set_supported_hints(int phys_screen)
{
    Atom atom[ATOM_NUMBER];
    int i = 0;

    atom[i++] = net_supported;
    atom[i++] = net_client_list;
    atom[i++] = net_number_of_desktops;
    atom[i++] = net_current_desktop;
    atom[i++] = net_desktop_names;
    atom[i++] = net_active_window;

    atom[i++] = net_close_window;

    atom[i++] = net_wm_name;
    atom[i++] = net_wm_icon_name;
    atom[i++] = net_wm_window_type;
    atom[i++] = net_wm_window_type_normal;
    atom[i++] = net_wm_window_type_dock;
    atom[i++] = net_wm_window_type_splash;
    atom[i++] = net_wm_window_type_dialog;
    atom[i++] = net_wm_icon;
    atom[i++] = net_wm_state;
    atom[i++] = net_wm_state_sticky;
    atom[i++] = net_wm_state_skip_taskbar;
    atom[i++] = net_wm_state_fullscreen;
    atom[i++] = net_wm_state_demands_attention;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) atom, i);
}

void
ewmh_update_net_client_list(int phys_screen)
{
    Window *wins;
    Client *c;
    int n = 0;

    for(c = globalconf.clients; c; c = c->next)
        n++;

    wins = p_new(Window, n);

    for(n = 0, c = globalconf.clients; c; c = c->next, n++)
        if(c->phys_screen == phys_screen)
            wins[n] = c->win;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_client_list, XA_WINDOW, 32, PropModeReplace, (unsigned char *) wins, n);

    p_delete(&wins);
}

void
ewmh_update_net_numbers_of_desktop(int phys_screen)
{
    CARD32 count = 0;
    Tag *tag;

    for(tag = globalconf.screens[phys_screen].tags; tag; tag = tag->next)
        count++;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_number_of_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &count, 1);
}

void
ewmh_update_net_current_desktop(int phys_screen)
{
    CARD32 count = 0;
    Tag *tag, **curtags = tags_get_current(phys_screen);

    for(tag = globalconf.screens[phys_screen].tags; tag != curtags[0]; tag = tag->next)
        count++;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_current_desktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &count, 1);

    p_delete(&curtags);
}

void
ewmh_update_net_desktop_names(int phys_screen)
{
    char buf[1024], *pos;
    ssize_t len, curr_size;
    Tag *tag;

    pos = buf;
    len = 0;
    for(tag = globalconf.screens[phys_screen].tags; tag; tag = tag->next)
    {
        curr_size = a_strlen(tag->name);
        a_strcpy(pos, sizeof(buf), tag->name);
        pos += curr_size + 1;
        len += curr_size + 1;
    }

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_desktop_names, utf8_string, 8, PropModeReplace, (unsigned char *) buf, len);
}

void
ewmh_update_net_active_window(int phys_screen)
{
    Window win;
    Client *sel = focus_get_current_client(phys_screen);

    win = sel ? sel->win : None;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_active_window, XA_WINDOW, 32,  PropModeReplace, (unsigned char *) &win, 1);
}

static void
ewmh_process_state_atom(Client *c, Atom state, int set)
{
    if(state == net_wm_state_sticky)
    {
        Tag *tag;
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            tag_client(c, tag);
    }
    else if(state == net_wm_state_skip_taskbar)
    {
        if(set == _NET_WM_STATE_REMOVE)
        {
            c->skiptb = False;
            c->skip = False;
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            c->skiptb = True;
            c->skip = True;
        }
    }
    else if(state == net_wm_state_demands_attention) {
        if (set == _NET_WM_STATE_REMOVE)
            client_seturgent(c, False);
        else if (set == _NET_WM_STATE_ADD)
            client_seturgent(c, True);
        else if (set == _NET_WM_STATE_TOGGLE)
            client_seturgent(c, !c->isurgent);
    }
    else if(state == net_wm_state_fullscreen)
    {
        area_t geometry = c->geometry;
        if(set == _NET_WM_STATE_REMOVE)
        {
            /* restore geometry */
            geometry = c->m_geometry;
            /* restore borders and titlebar */
            titlebar_position_set(&c->titlebar, c->titlebar.dposition);
            c->border = c->oldborder;
            c->ismax = False;
            client_setfloating(c, c->wasfloating);
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            geometry = screen_get_area(c->screen, NULL, &globalconf.screens[c->screen].padding);
            /* save geometry */
            c->m_geometry = c->geometry;
            c->wasfloating = c->isfloating;
            /* disable titlebar and borders */
            titlebar_position_set(&c->titlebar, Off);
            c->oldborder = c->border;
            c->border = 0;
            c->ismax = True;
            client_setfloating(c, True);
        }
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_resize(c, geometry, False);
        XRaiseWindow(globalconf.display, c->win);
    }
}

static void
ewmh_process_window_type_atom(Client *c, Atom state)
{
    if(state == net_wm_window_type_normal)
    {
        /* do nothing. this is REALLY IMPORTANT */
    }
    else if(state == net_wm_window_type_dock
            || state == net_wm_window_type_splash)
    {
        c->border = 0;
        c->skip = True;
        c->isfixed = True;
        titlebar_position_set(&c->titlebar, Off);
        client_setfloating(c, True);
    }
    else if (state == net_wm_window_type_dialog)
    {
        client_setfloating(c, True);
        client_focus(c, c->screen, True);
    }
}
void
ewmh_process_client_message(XClientMessageEvent *ev)
{
    Client *c;
    int screen;

    if(ev->message_type == net_current_desktop)
        for(screen = 0; screen < ScreenCount(globalconf.display); screen++)
            if(ev->window == RootWindow(globalconf.display, screen))
                tag_view_only_byindex(screen, ev->data.l[0]);

    if(ev->message_type == net_close_window)
    {
        if((c = client_get_bywin(globalconf.clients, ev->window)))
           client_kill(c);
    }
    else if(ev->message_type == net_wm_state)
    {
        if((c = client_get_bywin(globalconf.clients, ev->window)))
        {
            ewmh_process_state_atom(c, (Atom) ev->data.l[1], ev->data.l[0]);
            if(ev->data.l[2])
                ewmh_process_state_atom(c, (Atom) ev->data.l[2], ev->data.l[0]);
        }
    }
}

void
ewmh_check_client_hints(Client *c)
{
    int format;
    Atom real, *state;
    unsigned char *data = NULL;
    unsigned long i, n, extra;

    if(XGetWindowProperty(globalconf.display, c->win, net_wm_state, 0L, LONG_MAX, False,
                          XA_ATOM, &real, &format, &n, &extra,
                          (unsigned char **) &data) == Success && data)
    {
        state = (Atom *) data;
        for(i = 0; i < n; i++)
            ewmh_process_state_atom(c, state[i], _NET_WM_STATE_ADD);

        XFree(data);
    }

    if(XGetWindowProperty(globalconf.display, c->win, net_wm_window_type, 0L, LONG_MAX, False,
                          XA_ATOM, &real, &format, &n, &extra,
                          (unsigned char **) &data) == Success && data)
    {
        state = (Atom *) data;
        for(i = 0; i < n; i++)
            ewmh_process_window_type_atom(c, state[i]);

        XFree(data);
    }
}

NetWMIcon *
ewmh_get_window_icon(Window w)
{
    double alpha;
    NetWMIcon *icon;
    Atom type;
    int format, size, i;
    unsigned long items, rest, *data;
    unsigned char *imgdata, *wdata;

    if(XGetWindowProperty(globalconf.display, w,
                          net_wm_icon, 0L, LONG_MAX, False, XA_CARDINAL, &type, &format,
                          &items, &rest, &wdata) != Success
       || !wdata)
        return NULL;

    if(type != XA_CARDINAL || format != 32 || items < 2)
    {
        XFree(wdata);
        return NULL;
    }

    icon = p_new(NetWMIcon, 1);

    data = (unsigned long *) wdata;

    icon->width = data[0];
    icon->height = data[1];
    size = icon->width * icon->height;

    if(!size)
    {
        p_delete(&icon);
        XFree(wdata);
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

    XFree(wdata);

    return icon;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
