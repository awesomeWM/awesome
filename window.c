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

extern awesome_t globalconf;

/** Mask shorthands */
#define BUTTONMASK     (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE)

/** Set client state (WM_STATE) property.
 * \param win The window to set state.
 * \param state The state to set.
 */
void
window_setstate(xcb_window_t win, long state)
{
    long data[] = { state, XCB_NONE };
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                        WM_STATE, WM_STATE, 32, 2, data);
}

/** Get a window state (WM_STATE).
 * \param w A client window.
 * \return The current state of the window, or -1 on error.
 */
long
window_getstate(xcb_window_t w)
{
    long result = -1;
    unsigned char *p = NULL;
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false, w,
                                        WM_STATE, WM_STATE,
                                        0L, 2L);

    if(!(prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)))
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
window_grabbuttons(xcb_window_t win, xcb_window_t root, button_t *buttons)
{
    button_t *b;

    for(b = buttons; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, root, ANY_MODIFIER);
}

/** Grab all buttons on the root window.
 * \param root The root window.
 */
void
window_root_grabbuttons(xcb_window_t root)
{
    button_t *b;

    for(b = globalconf.buttons.root; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }
}

/** Set transparency of a window.
 * \param win The window.
 * \param opacity Opacity of the window, between 0 and 1.
 */
void
window_settrans(xcb_window_t win, double opacity)
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
