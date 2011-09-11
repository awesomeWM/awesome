/*
 * selection.c - Selection handling
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2009 Gregor Best <farhaven@googlemail.com>
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

#include <xcb/xcb_atom.h>

#include "selection.h"
#include "event.h"
#include "common/atoms.h"
#include "common/xutil.h"

static xcb_window_t selection_window = XCB_NONE;

/** Get the current X selection buffer.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn A string with the current X selection buffer.
 */
int
luaA_selection_get(lua_State *L)
{
    if(selection_window == XCB_NONE)
    {
        xcb_screen_t *screen = globalconf.screen;
        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
        uint32_t values[] = { screen->black_pixel, 1, XCB_EVENT_MASK_PROPERTY_CHANGE };

        selection_window = xcb_generate_id(globalconf.connection);

        xcb_create_window(globalconf.connection, screen->root_depth, selection_window, screen->root,
                          0, 0, 1, 1, 0, XCB_COPY_FROM_PARENT, screen->root_visual,
                          mask, values);
    }

    xcb_convert_selection(globalconf.connection, selection_window,
                          XCB_ATOM_PRIMARY, UTF8_STRING, XSEL_DATA, globalconf.timestamp);
    xcb_flush(globalconf.connection);

    xcb_generic_event_t *event;

    while(true)
    {
        event = xcb_wait_for_event(globalconf.connection);

        if(!event)
            return 0;

        if(XCB_EVENT_RESPONSE_TYPE(event) != XCB_SELECTION_NOTIFY)
        {
            /* \todo Eventually, this may be rewritten with adding a static
             * buffer, then a event handler for XCB_SELECTION_NOTIFY, then call
             * xcb_event_poll_for_event_loop() and awesome_refresh(),
             * then check if some static buffer has been filled with data.
             * If yes, that'd be the xsel data, otherwise, re-loop.
             * Anyway that's still brakes the socket or D-Bus, so maybe using
             * ev_loop() would be even better.
             */
            event_handle(event);
            p_delete(&event);
            awesome_refresh();
            continue;
        }

        xcb_selection_notify_event_t *event_notify =
            (xcb_selection_notify_event_t *) event;

        if(event_notify->selection == XCB_ATOM_PRIMARY
           && event_notify->property != XCB_NONE)
        {
            xcb_icccm_get_text_property_reply_t prop;
            xcb_get_property_cookie_t cookie =
                xcb_icccm_get_text_property(globalconf.connection,
					    event_notify->requestor,
					    event_notify->property);

            if(xcb_icccm_get_text_property_reply(globalconf.connection,
						 cookie, &prop, NULL))
	      {
                lua_pushlstring(L, prop.name, prop.name_len);

                xcb_icccm_get_text_property_reply_wipe(&prop);

                xcb_delete_property(globalconf.connection,
                                    event_notify->requestor,
                                    event_notify->property);

                p_delete(&event);

                return 1;
            }
            else
                break;
        }
    }

    p_delete(&event);
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
