/*
 * systray.c - systray handling
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include "screen.h"
#include "systray.h"
#include "xwindow.h"
#include "objects/widget.h"
#include "objects/wibox.h"
#include "common/array.h"
#include "common/atoms.h"
#include "common/xutil.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0 /* Begin icon docking */

/** Initialize systray information in X.
 */
void
systray_init(void)
{
    xcb_screen_t *xscreen = globalconf.screen;

    globalconf.systray.window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth,
                      globalconf.systray.window,
                      xscreen->root,
                      -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, globalconf.visual->visual_id, 0, NULL);
}


/** Refresh all systrays registrations per physical screen
 */
void
systray_refresh(void)
{
    bool has_systray = false;
    foreach(w, globalconf.wiboxes)
        if((*w)->has_systray)
            /* Can't use "break" with foreach() :( */
            has_systray = true;

    if(has_systray)
        systray_register();
    else
        systray_cleanup();
}


/** Register systray in X.
 */
void
systray_register(void)
{
    xcb_client_message_event_t ev;
    xcb_screen_t *xscreen = globalconf.screen;
    char *atom_name;
    xcb_intern_atom_cookie_t atom_systray_q;
    xcb_intern_atom_reply_t *atom_systray_r;
    xcb_atom_t atom_systray;

    /* Set registered even if it fails to don't try again unless forced */
    if(globalconf.systray.registered)
        return;
    globalconf.systray.registered = true;

    /* Send requests */
    if(!(atom_name = xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", globalconf.default_screen)))
    {
        warn("error getting systray atom");
        return;
    }

    atom_systray_q = xcb_intern_atom_unchecked(globalconf.connection, false,
                                               a_strlen(atom_name), atom_name);

    p_delete(&atom_name);

    /* Fill event */
    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = xscreen->root;
    ev.format = 32;
    ev.type = MANAGER;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[2] = globalconf.systray.window;
    ev.data.data32[3] = ev.data.data32[4] = 0;

    if(!(atom_systray_r = xcb_intern_atom_reply(globalconf.connection, atom_systray_q, NULL)))
    {
        warn("error getting systray atom");
        return;
    }

    ev.data.data32[1] = atom_systray = atom_systray_r->atom;

    p_delete(&atom_systray_r);

    xcb_set_selection_owner(globalconf.connection,
                            globalconf.systray.window,
                            atom_systray,
                            XCB_CURRENT_TIME);

    xcb_send_event(globalconf.connection, false, xscreen->root, 0xFFFFFF, (char *) &ev);
}

/** Remove systray information in X.
 */
void
systray_cleanup(void)
{
    xcb_intern_atom_reply_t *atom_systray_r;
    char *atom_name;

    if(!globalconf.systray.registered)
        return;
    globalconf.systray.registered = false;

    if(!(atom_name = xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", globalconf.default_screen))
       || !(atom_systray_r = xcb_intern_atom_reply(globalconf.connection,
                                                   xcb_intern_atom_unchecked(globalconf.connection,
                                                                             false,
                                                                             a_strlen(atom_name),
                                                                             atom_name),
                                                   NULL)))
    {
        warn("error getting systray atom");
        p_delete(&atom_name);
        return;
    }

    p_delete(&atom_name);

    xcb_set_selection_owner(globalconf.connection,
                            XCB_NONE,
                            atom_systray_r->atom,
                            XCB_CURRENT_TIME);

    p_delete(&atom_systray_r);
}

/** Handle a systray request.
 * \param embed_win The window to embed.
 * \param info The embedding info
 * \return 0 on no error.
 */
int
systray_request_handle(xcb_window_t embed_win, xembed_info_t *info)
{
    xembed_window_t em;
    xcb_get_property_cookie_t em_cookie;
    const uint32_t select_input_val[] =
    {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE
            | XCB_EVENT_MASK_ENTER_WINDOW
    };

    /* check if not already trayed */
    if(xembed_getbywin(&globalconf.embedded, embed_win))
        return -1;

    p_clear(&em_cookie, 1);

    if(!info)
        em_cookie = xembed_info_get_unchecked(globalconf.connection, embed_win);

    xcb_change_window_attributes(globalconf.connection, embed_win, XCB_CW_EVENT_MASK,
                                 select_input_val);
    xwindow_set_state(embed_win, XCB_WM_STATE_WITHDRAWN);

    /* we grab the window, but also make sure it's automatically reparented back
     * to the root window if we should die.
     */
    xcb_change_save_set(globalconf.connection, XCB_SET_MODE_INSERT, embed_win);
    xcb_reparent_window(globalconf.connection, embed_win,
                        globalconf.systray.window,
                        0, 0);

    em.win = embed_win;

    if(info)
        em.info = *info;
    else
        xembed_info_get_reply(globalconf.connection, em_cookie, &em.info);

    xembed_embedded_notify(globalconf.connection, em.win,
                           globalconf.systray.window,
                           MIN(XEMBED_VERSION, em.info.version));

    xembed_window_array_append(&globalconf.embedded, em);

    widget_invalidate_bytype(widget_systray);

    return 0;
}

/** Handle systray message.
 * \param ev The event.
 * \return 0 on no error.
 */
int
systray_process_client_message(xcb_client_message_event_t *ev)
{
    int ret = 0;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;

    switch(ev->data.data32[1])
    {
      case SYSTEM_TRAY_REQUEST_DOCK:
        geom_c = xcb_get_geometry_unchecked(globalconf.connection, ev->window);

        if(!(geom_r = xcb_get_geometry_reply(globalconf.connection, geom_c, NULL)))
            return -1;

        if(globalconf.screen->root == geom_r->root)
            ret = systray_request_handle(ev->data.data32[2], NULL);

        p_delete(&geom_r);
        break;
    }

    return ret;
}

/** Check if a window is a KDE tray.
 * \param w The window to check.
 * \return True if it is, false otherwise.
 */
bool
systray_iskdedockapp(xcb_window_t w)
{
    xcb_get_property_cookie_t kde_check_q;
    xcb_get_property_reply_t *kde_check;
    bool ret;

    /* Check if that is a KDE tray because it does not respect fdo standards,
     * thanks KDE. */
    kde_check_q = xcb_get_property_unchecked(globalconf.connection, false, w,
                                             _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
                                             XCB_ATOM_WINDOW, 0, 1);

    kde_check = xcb_get_property_reply(globalconf.connection, kde_check_q, NULL);

    /* it's a KDE systray ?*/
    ret = (kde_check && kde_check->value_len);

    p_delete(&kde_check);

    return ret;
}

/** Handle xembed client message.
 * \param ev The event.
 * \return 0 on no error.
 */
int
xembed_process_client_message(xcb_client_message_event_t *ev)
{
    switch(ev->data.data32[1])
    {
      case XEMBED_REQUEST_FOCUS:
        xembed_focus_in(globalconf.connection, ev->window, XEMBED_FOCUS_CURRENT);
        break;
    }
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
