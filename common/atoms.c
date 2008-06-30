/*
 * atoms.c - X atoms functions
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

#include "common/atoms.h"
#include "common/util.h"

xcb_atom_t _NET_SUPPORTED;
xcb_atom_t _NET_CLIENT_LIST;
xcb_atom_t _NET_CLIENT_LIST_STACKING;
xcb_atom_t _NET_NUMBER_OF_DESKTOPS;
xcb_atom_t _NET_CURRENT_DESKTOP;
xcb_atom_t _NET_DESKTOP_NAMES;
xcb_atom_t _NET_ACTIVE_WINDOW;
xcb_atom_t _NET_WORKAREA;
xcb_atom_t _NET_SUPPORTING_WM_CHECK;
xcb_atom_t _NET_CLOSE_WINDOW;
xcb_atom_t _NET_WM_NAME;
xcb_atom_t _NET_WM_VISIBLE_NAME;
xcb_atom_t _NET_WM_DESKTOP;
xcb_atom_t _NET_WM_ICON_NAME;
xcb_atom_t _NET_WM_VISIBLE_ICON_NAME;
xcb_atom_t _NET_WM_WINDOW_TYPE;
xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
xcb_atom_t _NET_WM_WINDOW_TYPE_DESKTOP;
xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
xcb_atom_t _NET_WM_WINDOW_TYPE_SPLASH;
xcb_atom_t _NET_WM_WINDOW_TYPE_DIALOG;
xcb_atom_t _NET_WM_ICON;
xcb_atom_t _NET_WM_PID;
xcb_atom_t _NET_WM_STATE;
xcb_atom_t _NET_WM_STATE_STICKY;
xcb_atom_t _NET_WM_STATE_SKIP_TASKBAR;
xcb_atom_t _NET_WM_STATE_FULLSCREEN;
xcb_atom_t _NET_WM_STATE_ABOVE;
xcb_atom_t _NET_WM_STATE_BELOW;
xcb_atom_t _NET_WM_STATE_MODAL;
xcb_atom_t _NET_WM_STATE_HIDDEN;
xcb_atom_t _NET_WM_STATE_DEMANDS_ATTENTION;
xcb_atom_t UTF8_STRING;
xcb_atom_t _AWESOME_PROPERTIES;
xcb_atom_t WM_PROTOCOLS;
xcb_atom_t WM_DELETE_WINDOW;
xcb_atom_t _XEMBED;
xcb_atom_t _XEMBED_INFO;
xcb_atom_t _NET_SYSTEM_TRAY_OPCODE;
xcb_atom_t _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;
xcb_atom_t MANAGER;
xcb_atom_t _XROOTPMAP_ID;
xcb_atom_t WM_STATE;
xcb_atom_t _NET_WM_WINDOW_OPACITY;
xcb_atom_t _NET_SYSTEM_TRAY_ORIENTATION;

typedef struct
{
    const char *name;
    xcb_atom_t *atom;
} atom_item_t;

static atom_item_t ATOM_LIST[] =
{
    { "_NET_SUPPORTED", &_NET_SUPPORTED },
    { "_NET_CLIENT_LIST", &_NET_CLIENT_LIST },
    { "_NET_CLIENT_LIST_STACKING", &_NET_CLIENT_LIST_STACKING },
    { "_NET_NUMBER_OF_DESKTOPS", &_NET_NUMBER_OF_DESKTOPS },
    { "_NET_CURRENT_DESKTOP", &_NET_CURRENT_DESKTOP },
    { "_NET_DESKTOP_NAMES", &_NET_DESKTOP_NAMES },
    { "_NET_ACTIVE_WINDOW", &_NET_ACTIVE_WINDOW },
    { "_NET_WORKAREA", &_NET_WORKAREA },
    { "_NET_SUPPORTING_WM_CHECK", &_NET_SUPPORTING_WM_CHECK },
    { "_NET_CLOSE_WINDOW", &_NET_CLOSE_WINDOW },
    { "_NET_WM_NAME", &_NET_WM_NAME },
    { "_NET_WM_VISIBLE_NAME", &_NET_WM_VISIBLE_NAME },
    { "_NET_WM_DESKTOP", &_NET_WM_DESKTOP },
    { "_NET_WM_ICON_NAME", &_NET_WM_ICON_NAME },
    { "_NET_WM_VISIBLE_ICON_NAME", &_NET_WM_VISIBLE_ICON_NAME },
    { "_NET_WM_WINDOW_TYPE", &_NET_WM_WINDOW_TYPE },
    { "_NET_WM_WINDOW_TYPE_NORMAL", &_NET_WM_WINDOW_TYPE_NORMAL },
    { "_NET_WM_WINDOW_TYPE_DESKTOP", &_NET_WM_WINDOW_TYPE_DESKTOP },
    { "_NET_WM_WINDOW_TYPE_DOCK", &_NET_WM_WINDOW_TYPE_DOCK },
    { "_NET_WM_WINDOW_TYPE_SPLASH", &_NET_WM_WINDOW_TYPE_SPLASH },
    { "_NET_WM_WINDOW_TYPE_DIALOG", &_NET_WM_WINDOW_TYPE_DIALOG },
    { "_NET_WM_ICON", &_NET_WM_ICON },
    { "_NET_WM_PID", &_NET_WM_PID },
    { "_NET_WM_STATE", &_NET_WM_STATE },
    { "_NET_WM_STATE_STICKY", &_NET_WM_STATE_STICKY },
    { "_NET_WM_STATE_SKIP_TASKBAR", &_NET_WM_STATE_SKIP_TASKBAR },
    { "_NET_WM_STATE_FULLSCREEN", &_NET_WM_STATE_FULLSCREEN },
    { "_NET_WM_STATE_ABOVE", &_NET_WM_STATE_ABOVE },
    { "_NET_WM_STATE_BELOW", &_NET_WM_STATE_BELOW },
    { "_NET_WM_STATE_MODAL", &_NET_WM_STATE_MODAL },
    { "_NET_WM_STATE_HIDDEN", &_NET_WM_STATE_HIDDEN },
    { "_NET_WM_STATE_DEMANDS_ATTENTION", &_NET_WM_STATE_DEMANDS_ATTENTION },
    { "UTF8_STRING", &UTF8_STRING },
    { "_AWESOME_PROPERTIES", &_AWESOME_PROPERTIES },
    { "WM_PROTOCOLS", &WM_PROTOCOLS },
    { "WM_DELETE_WINDOW", &WM_DELETE_WINDOW },
    { "_XEMBED", &_XEMBED },
    { "_XEMBED_INFO", &_XEMBED_INFO },
    { "_NET_SYSTEM_TRAY_OPCODE", &_NET_SYSTEM_TRAY_OPCODE },
    { "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", &_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR },
    { "MANAGER", &MANAGER },
    { "_XROOTPMAP_ID", &_XROOTPMAP_ID },
    { "WM_STATE", &WM_STATE },
    { "_NET_WM_WINDOW_OPACITY", &_NET_WM_WINDOW_OPACITY },
    { "_NET_SYSTEM_TRAY_ORIENTATION", &_NET_SYSTEM_TRAY_ORIENTATION },
};

void
atoms_init(xcb_connection_t *conn)
{
    unsigned int i;
    xcb_intern_atom_cookie_t cs[countof(ATOM_LIST)];
    xcb_intern_atom_reply_t *r;

    /* Create the atom and get the reply in a XCB way (e.g. send all
     * the requests at the same time and then get the replies) */
    for(i = 0; i < countof(ATOM_LIST); i++)
	cs[i] = xcb_intern_atom_unchecked(conn,
					  false,
					  a_strlen(ATOM_LIST[i].name),
					  ATOM_LIST[i].name);

    for(i = 0; i < countof(ATOM_LIST); i++)
    {
	if(!(r = xcb_intern_atom_reply(conn, cs[i], NULL)))
	    /* An error occured, get reply for next atom */
	    continue;

	*ATOM_LIST[i].atom = r->atom;
        p_delete(&r);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
