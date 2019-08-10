/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#include <stdbool.h>

#include <mousegrabber.h>
#include <root.h>

#include "globalconf.h"
#include "x11/root.h"
#include "x11/mousegrabber.h"
#include "x11/globals.h"

extern struct mousegrabber_impl mousegrabber_impl;
extern struct root_impl root_impl;

void init_x11(void)
{
    const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    xcb_void_cookie_t cookie;

    /* This causes an error if some other window manager is running */
    cookie = xcb_change_window_attributes_checked(globalconf.connection,
            globalconf.screen->root,
            XCB_CW_EVENT_MASK, &select_input_val);
    if (xcb_request_check(globalconf.connection, cookie))
        fatal("another window manager is already running (can't select SubstructureRedirect)");

    mousegrabber_impl = (struct mousegrabber_impl){
        .grab_mouse = x11_grab_mouse,
        .release_mouse = x11_release_mouse,
    };
    root_impl = (struct root_impl){
        .grab_keys = x11_grab_keys,
    };
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
