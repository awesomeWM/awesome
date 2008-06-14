/*
 * common/xembed.c - XEMBED functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2004 Matthew Reppert
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

#include "common/xembed.h"
#include "common/xutil.h"
#include "common/util.h"

/** Have the embedder end XEMBED protocol communication with a child.
 * \param connection The X connection.
 * \param child The window to unembed.
 * \param root The root window to reparent to.
 */
static inline void
xembed_window_unembed(xcb_connection_t *connection, xcb_window_t child, xcb_window_t root)
{
    xcb_unmap_window(connection, child);
    xcb_reparent_window(connection, child, root, 0, 0);
}

/** Indicate to an embedded window that it has lost focus.
 * \param c The X connection.
 * \param client The client to send message to.
 */
static inline void
xembed_focus_out(xcb_connection_t *c, xcb_window_t client)
{
    xembed_message_send(c, client, XEMBED_FOCUS_OUT, 0, 0, 0);
}

/** Send an XEMBED message to a window.
 * \param connection Connection to the X server.
 * \param towin Destination window
 * \param message The message.
 * \param d1 Element 3 of message.
 * \param d2 Element 4 of message.
 * \param d3 Element 5 of message.
 */
void
xembed_message_send(xcb_connection_t *connection, xcb_window_t towin,
                    long message, long d1, long d2, long d3)
{
    xcb_client_message_event_t ev;
    xutil_intern_atom_request_t atom_q;
    xcb_atom_t atom;

    /** \todo use atom cache */
    atom_q = xutil_intern_atom(connection, NULL, "_XEMBED");
    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = towin;
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = message;
    ev.data.data32[2] = d1;
    ev.data.data32[3] = d2;
    ev.data.data32[4] = d3;
    atom = xutil_intern_atom_reply(connection, NULL, atom_q);
    ev.type = atom;
    xcb_send_event(connection, false, towin, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

/** Get the XEMBED info for a window.
 * \param connection The X connection.
 * \param win The window.
 * \param info The xembed_info_t structure to fill.
 */
bool
xembed_info_get(xcb_connection_t *connection, xcb_window_t win, xembed_info_t *info)
{
    xutil_intern_atom_request_t atom_q;
    xcb_atom_t atom;
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    uint32_t *data;
    bool ret = false;

    /** \todo use atom cache */
    atom_q = xutil_intern_atom(connection, NULL, "_XEMBED_INFO");
    atom = xutil_intern_atom_reply(connection, NULL, atom_q);

    prop_c = xcb_get_property(connection, false, win, atom, XCB_GET_PROPERTY_TYPE_ANY, 0L, 2);
    prop_r = xcb_get_property_reply(connection, prop_c, NULL);

    if(!prop_r || !prop_r->value_len)
        goto bailout;

    if(!(data = (uint32_t *) xcb_get_property_value(prop_r)))
        goto bailout;

    info->version = data[0];
    info->flags = data[1] & XEMBED_INFO_FLAGS_ALL;
    ret = true;

bailout:
    p_delete(&prop_r);
    return ret;
}

/** Get a XEMBED window from a xembed_window_t list.
 * \param list The xembed window list.
 * \param win The window to look for.
 */
xembed_window_t *
xembed_getbywin(xembed_window_t *list, xcb_window_t win)
{
    xembed_window_t *n;
    for(n = list; n; n = n->next)
        if(win == n->win)
            return n;
    return NULL;
}

/** Update embedded window properties.
 * \param connection The X connection.
 * \param emwin The embedded window.
 */
void
xembed_property_update(xcb_connection_t *connection, xembed_window_t *emwin)
{
    int flags_changed;
    xembed_info_t info = { 0, 0 };

    xembed_info_get(connection, emwin->win, &info);
    /* test if it changed */
    if(!(flags_changed = info.flags ^ emwin->info.flags))
        return;

    emwin->info.flags = info.flags;
    if(flags_changed & XEMBED_MAPPED)
    {
        if(info.flags & XEMBED_MAPPED)
        {
            xcb_map_window(connection, emwin->win);
            xembed_window_activate(connection, emwin->win);
        }
        else
        {
            xcb_unmap_window(connection, emwin->win);
            xembed_window_deactivate(connection, emwin->win);
            xembed_focus_out(connection, emwin->win);
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
