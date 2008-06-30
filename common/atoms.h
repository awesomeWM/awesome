/*
 * atoms.h - atoms header
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

#ifndef AWESOME_COMMON_ATOMS_H
#define AWESOME_COMMON_ATOMS_H

#include <xcb/xcb.h>

extern xcb_atom_t _NET_SUPPORTED;
extern xcb_atom_t _NET_CLIENT_LIST;
extern xcb_atom_t _NET_CLIENT_LIST_STACKING;
extern xcb_atom_t _NET_NUMBER_OF_DESKTOPS;
extern xcb_atom_t _NET_CURRENT_DESKTOP;
extern xcb_atom_t _NET_DESKTOP_NAMES;
extern xcb_atom_t _NET_ACTIVE_WINDOW;
extern xcb_atom_t _NET_WORKAREA;
extern xcb_atom_t _NET_SUPPORTING_WM_CHECK;
extern xcb_atom_t _NET_CLOSE_WINDOW;
extern xcb_atom_t _NET_WM_NAME;
extern xcb_atom_t _NET_WM_VISIBLE_NAME;
extern xcb_atom_t _NET_WM_DESKTOP;
extern xcb_atom_t _NET_WM_ICON_NAME;
extern xcb_atom_t _NET_WM_VISIBLE_ICON_NAME;
extern xcb_atom_t _NET_WM_WINDOW_TYPE;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_DESKTOP;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_SPLASH;
extern xcb_atom_t _NET_WM_WINDOW_TYPE_DIALOG;
extern xcb_atom_t _NET_WM_ICON;
extern xcb_atom_t _NET_WM_PID;
extern xcb_atom_t _NET_WM_STATE;
extern xcb_atom_t _NET_WM_STATE_STICKY;
extern xcb_atom_t _NET_WM_STATE_SKIP_TASKBAR;
extern xcb_atom_t _NET_WM_STATE_FULLSCREEN;
extern xcb_atom_t _NET_WM_STATE_ABOVE;
extern xcb_atom_t _NET_WM_STATE_BELOW;
extern xcb_atom_t _NET_WM_STATE_MODAL;
extern xcb_atom_t _NET_WM_STATE_HIDDEN;
extern xcb_atom_t _NET_WM_STATE_DEMANDS_ATTENTION;
extern xcb_atom_t UTF8_STRING;
extern xcb_atom_t _AWESOME_PROPERTIES;
extern xcb_atom_t WM_PROTOCOLS;
extern xcb_atom_t WM_DELETE_WINDOW;
extern xcb_atom_t _XEMBED;
extern xcb_atom_t _XEMBED_INFO;
extern xcb_atom_t _NET_SYSTEM_TRAY_OPCODE;
extern xcb_atom_t _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR;
extern xcb_atom_t MANAGER;
extern xcb_atom_t _XROOTPMAP_ID;
extern xcb_atom_t WM_STATE;
extern xcb_atom_t _NET_WM_WINDOW_OPACITY;
extern xcb_atom_t _NET_SYSTEM_TRAY_ORIENTATION;

void atoms_init(xcb_connection_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
