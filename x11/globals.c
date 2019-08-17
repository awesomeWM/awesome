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

#include "objects/drawin.h"
#include "objects/drawable.h"
#include <mousegrabber.h>
#include <root.h>

#include "globalconf.h"
#include "x11/drawin.h"
#include "x11/drawable.h"
#include "x11/root.h"
#include "x11/mousegrabber.h"
#include "x11/globals.h"

extern struct drawin_impl drawin_impl;
extern struct drawable_impl drawable_impl;
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

    drawin_impl = (struct drawin_impl){
        .get_xcb_window = x11_get_xcb_window,
        .get_drawin_by_window = x11_get_drawin_by_window,
        .drawin_cleanup = x11_drawin_cleanup,
        .drawin_allocate = x11_drawin_allocate,
        .drawin_moveresize = x11_drawin_moveresize,
        .drawin_refresh = x11_drawin_refresh,
        .drawin_map = x11_drawin_map,
        .drawin_unmap = x11_drawin_unmap,
        .drawin_get_opacity = x11_drawin_get_opacity,
        .drawin_set_cursor = x11_drawin_set_cursor,
        .drawin_systray_kickout = x11_drawin_systray_kickout,
        .drawin_get_shape_bounding = x11_drawin_get_shape_bounding,
        .drawin_get_shape_clip = x11_drawin_get_shape_clip,
        .drawin_get_shape_input = x11_drawin_get_shape_input,
        .drawin_set_shape_bounding = x11_drawin_set_shape_bounding,
        .drawin_set_shape_clip = x11_drawin_set_shape_clip,
        .drawin_set_shape_input = x11_drawin_set_shape_input,
    };
    drawable_impl = (struct drawable_impl){
        .get_pixmap = x11_get_pixmap,
        .drawable_allocate = x11_drawable_allocate,
        .drawable_unset_surface = x11_drawable_unset_surface,
        .drawable_allocate_buffer = x11_drawable_create_pixmap,
        .drawable_cleanup = x11_drawable_cleanup,
    };
    mousegrabber_impl = (struct mousegrabber_impl){
        .grab_mouse = x11_grab_mouse,
        .release_mouse = x11_release_mouse,
    };
    root_impl = (struct root_impl){
        .grab_keys = x11_grab_keys,
    };
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
