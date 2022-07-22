/*
 * A simple client icon that "does nothing".
 *
 * Copyright © 2021 Uli Schlachter <psychon@znc.in>
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>

#include "common/atoms.h"
#include "common/util.h"

// Include all the C code we might need directly into this file - I am lazy
#include "common/atoms.c"
#include "common/util.c"

#define SYSTEM_TRAY_REQUEST_DOCK 0

static xcb_atom_t systray_atom(xcb_connection_t *conn, int screen) {
    char *atom_name;
    xcb_intern_atom_reply_t *reply;
    xcb_atom_t atom;

    atom_name = xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", screen);
    if(!atom_name)
        fatal("Error getting systray atom name\n");

    reply = xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, strlen(atom_name), atom_name), NULL);
    free(atom_name);
    if (!reply)
        fatal("Error interning systray icon\n");

    atom = reply->atom;
    free(reply);
    return atom;
}

static xcb_window_t find_systray(xcb_connection_t *conn, xcb_atom_t net_system_tray) {
    xcb_get_selection_owner_reply_t* reply = xcb_get_selection_owner_reply(conn, xcb_get_selection_owner(conn, net_system_tray), NULL);
    if (!reply)
        fatal("Failed to request selection owner\n");
    xcb_window_t owner = reply->owner;
    if (owner == XCB_NONE)
        fatal("No systray running\n");
    free(reply);
    return owner;
}

static uint32_t get_color(xcb_connection_t *conn, xcb_colormap_t cm, uint16_t red, uint16_t green, uint16_t blue) {
    xcb_alloc_color_reply_t *reply = xcb_alloc_color_reply(conn, xcb_alloc_color(conn, cm, red, green, blue), NULL);
    if (!reply)
        fatal("Error allocating color");
    uint32_t pixel = reply->pixel;
    free(reply);
    return pixel;
}

static uint8_t find_visual_depth(const xcb_screen_t *s, xcb_visualid_t visual)
{
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(s);

    if(depth_iter.data)
        for(; depth_iter.rem; xcb_depth_next (&depth_iter))
            for(xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
                visual_iter.rem; xcb_visualtype_next (&visual_iter))
                if(visual == visual_iter.data->visual_id)
                    return depth_iter.data->depth;
    return 0;
}

int main() {
    int default_screen;
    xcb_connection_t* conn = xcb_connect(NULL, &default_screen);
    if (xcb_connection_has_error(conn)) {
        fatal("Could not connect to X11 server: %d\n", xcb_connection_has_error(conn));
    }
    atoms_init(conn);
    xcb_screen_t* screen = xcb_aux_get_screen(conn, default_screen);
    xcb_window_t systray_owner = find_systray(conn, systray_atom(conn, default_screen));

    // Try to use tray visual hint, if requested.
    char *use_hint_var = getenv("USE_TRAY_VISUAL_HINT");
    bool use_tray_visual_hint = use_hint_var && *use_hint_var;
    xcb_visualid_t tray_visual_id;
    uint8_t tray_depth;
    if (use_tray_visual_hint) {
        xcb_get_property_reply_t *tray_visual_r =
            xcb_get_property_reply(
                conn, xcb_get_property(conn, false, systray_owner,
                                       _NET_SYSTEM_TRAY_VISUAL,
                                       XCB_ATOM_VISUALID, 0, 1),
                NULL);
        tray_visual_id = screen->root_visual;
        if(tray_visual_r != NULL && xcb_get_property_value_length(tray_visual_r)) {
            tray_visual_id = *(uint32_t *)xcb_get_property_value(tray_visual_r);
            p_delete(&tray_visual_r);
        }
        tray_depth = find_visual_depth(screen, tray_visual_id);
        if (tray_depth == 0) {
            fatal("Error getting visual hint\n");
        }
    } else {
        tray_visual_id= screen->root_visual;
        tray_depth = screen->root_depth;
    }

    xcb_colormap_t cm = xcb_generate_id(conn);
    xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE,
                        cm, screen->root,
                        tray_visual_id);

    // Create a window for the systray icon
    xcb_window_t window = xcb_generate_id(conn);
    xcb_create_window(conn, tray_depth, window, screen->root, 0, 0, 10, 10, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      tray_visual_id,
                      (use_tray_visual_hint ? XCB_CW_BACK_PIXEL : XCB_CW_BACK_PIXMAP)
                          | XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP,
                      (uint32_t[]) {
                          use_tray_visual_hint
                              ? get_color(conn, cm, 0xffff, 0x9999, 0x0000)
                              : XCB_BACK_PIXMAP_PARENT_RELATIVE,
                          screen->black_pixel, cm
                      });
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, _XEMBED_INFO, _XEMBED_INFO, 32, 2, (uint32_t[]) { 0, 1 });

    // Make our window a systray icon
    xcb_client_message_event_t ev;

    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = systray_owner;
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = SYSTEM_TRAY_REQUEST_DOCK;
    ev.data.data32[2] = window;
    ev.data.data32[3] = 0;
    ev.data.data32[4] = 0;
    ev.type = _NET_SYSTEM_TRAY_OPCODE;
    xcb_send_event(conn, false, systray_owner, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);

    xcb_flush(conn);
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(conn)) != NULL) {
        free(event);
    }

    xcb_disconnect(conn);
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
