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
#include "common/atoms.h"

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

    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = towin;
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = message;
    ev.data.data32[2] = d1;
    ev.data.data32[3] = d2;
    ev.data.data32[4] = d3;
    ev.type = _XEMBED;
    xcb_send_event(connection, false, towin, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

/** Deliver a request to get XEMBED info for a window.
 * \param connection The X connection.
 * \param win The window.
 * \return A cookie.
 */
xcb_get_property_cookie_t
xembed_info_get_unchecked(xcb_connection_t *connection, xcb_window_t win)
{
    return xcb_get_property_unchecked(connection, false, win, _XEMBED_INFO,
                                      XCB_GET_PROPERTY_TYPE_ANY, 0L, 2);
}

static bool
xembed_info_from_reply(xembed_info_t *info, xcb_get_property_reply_t *prop_r)
{
    uint32_t *data;

    if(!prop_r || !prop_r->value_len)
        return false;

    if(!(data = (uint32_t *) xcb_get_property_value(prop_r)))
        return false;

    info->version = data[0];
    info->flags = data[1] & XEMBED_INFO_FLAGS_ALL;

    return true;
}

/** Get the XEMBED info for a window.
 * \param connection The X connection.
 * \param cookie The cookie of the request.
 * \param info The xembed_info_t structure to fill.
 */
bool
xembed_info_get_reply(xcb_connection_t *connection,
                      xcb_get_property_cookie_t cookie,
                      xembed_info_t *info)
{
    xcb_get_property_reply_t *prop_r = xcb_get_property_reply(connection, cookie, NULL);
    bool ret = xembed_info_from_reply(info, prop_r);
    p_delete(&prop_r);
    return ret;
}

/** Get a XEMBED window from a xembed_window_t list.
 * \param list The xembed window list.
 * \param win The window to look for.
 * \return The xembed window if found, NULL otherwise.
 */
xembed_window_t *
xembed_getbywin(xembed_window_array_t *list, xcb_window_t win)
{
    for(int i = 0; i < list->len; i++)
        if(list->tab[i].win == win)
            return &list->tab[i];
    return NULL;
}

/** Update embedded window properties.
 * \param connection The X connection.
 * \param emwin The embedded window.
 */
void
xembed_property_update(xcb_connection_t *connection, xembed_window_t *emwin,
                       xcb_get_property_reply_t *reply)
{
    int flags_changed;
    xembed_info_t info = { 0, 0 };

    xembed_info_from_reply(&info, reply);

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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
