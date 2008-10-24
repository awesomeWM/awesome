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
#include "mouse.h"
#include "common/atoms.h"

extern awesome_t globalconf;

/** Mask shorthands */
#define BUTTONMASK     (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE)

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
 * \return The current state of the window, or -1 on error.
 */
long
window_state_get_reply(xcb_get_property_cookie_t cookie)
{
    long result = -1;
    unsigned char *p = NULL;
    xcb_get_property_reply_t *prop_r;

    if(!(prop_r = xcb_get_property_reply(globalconf.connection, cookie, NULL)))
        return -1;

    p = xcb_get_property_value(prop_r);
    if(xcb_get_property_value_length(prop_r) != 0)
        result = *p;

    p_delete(&prop_r);

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

/** Grab or ungrab buttons on a window.
 * \param win The window.
 * \param root The root window.
 * \param buttons The buttons to grab.
 */
void
window_buttons_grab(xcb_window_t win, xcb_window_t root, button_array_t *buttons)
{
    for(int i = 0; i < buttons->len; i++)
    {
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        buttons->tab[i]->button, buttons->tab[i]->mod);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        buttons->tab[i]->button, buttons->tab[i]->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        buttons->tab[i]->button, buttons->tab[i]->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        buttons->tab[i]->button, buttons->tab[i]->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, root, XCB_BUTTON_MASK_ANY);
}

/** Grab all buttons on the root window.
 * \param root The root window.
 */
void
window_root_buttons_grab(xcb_window_t root)
{
    button_array_t *barr = &globalconf.buttons;

    for(int i = 0; i < barr->len; i++)
    {
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        barr->tab[i]->button, barr->tab[i]->mod);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        barr->tab[i]->button, barr->tab[i]->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        barr->tab[i]->button, barr->tab[i]->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        barr->tab[i]->button, barr->tab[i]->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }
}

/** Get the opacity of a window.
 * \param The window.
 * \return The opacity, between 0 and 1 or -1 or no opacity set.
 */
double
window_opacity_get(xcb_window_t win)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false, win,
                                        _NET_WM_WINDOW_OPACITY, CARDINAL, 0L, 1L);

    prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL);

    if(!prop_r || !prop_r->value_len || prop_r->format != 32)
        goto bailout;

    if(prop_r->value_len)
    {
        unsigned int *data = xcb_get_property_value(prop_r);
        return (double) *data / (double) 0xffffffff;
    }

  bailout:
    p_delete(&prop_r);
    return -1;
}

/** Set opacity of a window.
 * \param win The window.
 * \param opacity Opacity of the window, between 0 and 1.
 */
void
window_opacity_set(xcb_window_t win, double opacity)
{
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 1)
    {
        real_opacity = opacity * 0xffffffff;
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                            _NET_WM_WINDOW_OPACITY, CARDINAL, 32, 1L, &real_opacity);
    }
    else
        xcb_delete_property(globalconf.connection, win, _NET_WM_WINDOW_OPACITY);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
