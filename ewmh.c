/*
 * ewmh.c - EWMH support functions
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

#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "ewmh.h"
#include "util.h"
#include "tag.h"
#include "focus.h"

extern AwesomeConf globalconf;

static Atom net_supported;
static Atom net_client_list;
static Atom net_number_of_desktops;
static Atom net_current_desktop;
static Atom net_desktop_names;
static Atom net_active_window;

static Atom net_close_window;

static Atom net_wm_name;
static Atom net_wm_icon;

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
    { "_NET_WM_ICON", &net_wm_icon },

    { "UTF8_STRING", &utf8_string },
};

#define ATOM_NUMBER (sizeof(AtomNames)/sizeof(AtomItem)) 

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
    atom[i++] = net_wm_icon;

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
        if(c->phys_screen == phys_screen)
            n++;

    wins = p_new(Window, n + 1);

    for(n = 0, c = globalconf.clients; c; c = c->next, n++)
        if(c->phys_screen == phys_screen)
            wins[n] = c->win;

    XChangeProperty(globalconf.display, RootWindow(globalconf.display, phys_screen),
                    net_client_list, XA_WINDOW, 32, PropModeReplace, (unsigned char *) wins, n);

    p_delete(&wins);
    XFlush(globalconf.display);
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
    Tag *tag, **curtags = get_current_tags(phys_screen);

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

void
ewmh_process_client_message(XClientMessageEvent *ev)
{
    Client *c;

    if(ev->message_type == net_close_window)
        if((c = get_client_bywin(globalconf.clients, ev->window)))
           client_kill(c);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
