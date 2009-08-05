/*
 * window.c - window handling functions
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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "window.h"
#include "common/atoms.h"

/** Set client state (WM_STATE) property.
 * \param win The window to set state.
 * \param state The state to set.
 */
void
window_state_set(xcb_window_t win, long state)
{
    long data[] = { state, XCB_NONE };
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                        WM_STATE, WM_STATE, 32, 2, data);
}

/** Send request to get a window state (WM_STATE).
 * \param w A client window.
 * \return The cookie associated with the request.
 */
xcb_get_property_cookie_t
window_state_get_unchecked(xcb_window_t w)
{
    return xcb_get_property_unchecked(globalconf.connection, false, w, WM_STATE,
                                      WM_STATE, 0L, 2L);
}

/** Get a window state (WM_STATE).
 * \param cookie The cookie.
 * \return The current state of the window, or 0 on error.
 */
uint32_t
window_state_get_reply(xcb_get_property_cookie_t cookie)
{
    uint32_t result = 0;
    xcb_get_property_reply_t *prop_r;

    if((prop_r = xcb_get_property_reply(globalconf.connection, cookie, NULL)))
    {
        if(xcb_get_property_value_length(prop_r))
            result = *(uint32_t *) xcb_get_property_value(prop_r);

        p_delete(&prop_r);
    }

    return result;
}

/** Configure a window with its new geometry and border size.
 * \param win The X window id to configure.
 * \param geometry The new window geometry.
 * \param border The new border size.
 */
void
window_configure(xcb_window_t win, area_t geometry, int border)
{
    xcb_configure_notify_event_t ce;

    ce.response_type = XCB_CONFIGURE_NOTIFY;
    ce.event = win;
    ce.window = win;
    ce.x = geometry.x;
    ce.y = geometry.y;
    ce.width = geometry.width;
    ce.height = geometry.height;
    ce.border_width = border;
    ce.above_sibling = XCB_NONE;
    ce.override_redirect = false;
    xcb_send_event(globalconf.connection, false, win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *) &ce);
}

/** Get the opacity of a window.
 * \param win The window.
 * \return The opacity, between 0 and 1 or -1 or no opacity set.
 */
double
window_opacity_get(xcb_window_t win)
{
    double ret;

    xcb_get_property_cookie_t prop_c =
        xcb_get_property_unchecked(globalconf.connection, false, win,
                                   _NET_WM_WINDOW_OPACITY, CARDINAL, 0L, 1L);

    xcb_get_property_reply_t *prop_r =
        xcb_get_property_reply(globalconf.connection, prop_c, NULL);

    ret = window_opacity_get_from_reply(prop_r);
    p_delete(&prop_r);
    return ret;
}

/** Get the opacity of a window.
 * \param prop_r A reply to a get property request for _NET_WM_WINDOW_OPACITY.
 * \return The opacity, between 0 and 1.
 */
double window_opacity_get_from_reply(xcb_get_property_reply_t *prop_r)
{
    if(prop_r && prop_r->value_len && prop_r->format == 32)
    {
        uint32_t value = *(uint32_t *) xcb_get_property_value(prop_r);
        return (double) value / (double) 0xffffffff;
    }

    return -1;
}

/** Set opacity of a window.
 * \param win The window.
 * \param opacity Opacity of the window, between 0 and 1.
 */
void
window_opacity_set(xcb_window_t win, double opacity)
{
    if(opacity >= 0 && opacity <= 1)
    {
        uint32_t real_opacity = opacity * 0xffffffff;
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                            _NET_WM_WINDOW_OPACITY, CARDINAL, 32, 1L, &real_opacity);
    }
    else
        xcb_delete_property(globalconf.connection, win, _NET_WM_WINDOW_OPACITY);
}

/** Send WM_TAKE_FOCUS client message to window
 * \param win destination window
 */
void
window_takefocus(xcb_window_t win)
{
    xcb_client_message_event_t ev;

    /* Initialize all of event's fields first */
    p_clear(&ev, 1);

    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = win;
    ev.format = 32;
    ev.data.data32[1] = XCB_CURRENT_TIME;
    ev.type = WM_PROTOCOLS;
    ev.data.data32[0] = WM_TAKE_FOCUS;

    xcb_send_event(globalconf.connection, false, win,
                   XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

/** Set wibox cursor.
 * \param w The wibox.
 * \param c The cursor.
 */
void
window_set_cursor(xcb_window_t w, xcb_cursor_t c)
{
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_CURSOR,
                                 (const uint32_t[]) { c });
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
