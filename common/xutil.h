/*
 * common/xutil.h - X-related useful functions header
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

#ifndef AWESOME_COMMON_XUTIL_H
#define AWESOME_COMMON_XUTIL_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>

/* XCB doesn't provide keysyms definition */
#include <X11/keysym.h>

#include "array.h"

bool xutil_text_prop_get(xcb_connection_t *, xcb_window_t, xcb_atom_t, char **, ssize_t *);

/** Set the same handler for all errors */
void xutil_error_handler_catch_all_set(xcb_event_handlers_t *,
                                       xcb_generic_error_handler_t, void *);

uint16_t xutil_key_mask_fromstr(const char *, size_t);
uint8_t xutil_button_fromint(int);

/* Get the informations about the screen.
 * \param c X connection.
 * \param screen Screen number.
 * \return Screen informations (must not be freed!).
 */
static inline xcb_screen_t *
xutil_screen_get(xcb_connection_t *c, int screen)
{
    xcb_screen_t *s;

    if(xcb_connection_has_error(c))
        fatal("X connection invalid");

    s = xcb_aux_get_screen(c, screen);

    assert(s);

    return s;
}

int xutil_root2screen(xcb_connection_t *, xcb_window_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
