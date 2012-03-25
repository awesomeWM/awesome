/*
 * window.h - window handling functions header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_WINDOW_H
#define AWESOME_WINDOW_H

#include "globalconf.h"
#include "draw.h"

void window_state_set(xcb_window_t, long);
xcb_get_property_cookie_t window_state_get_unchecked(xcb_window_t);
uint32_t window_state_get_reply(xcb_get_property_cookie_t);
void window_configure(xcb_window_t, area_t, int);
void window_buttons_grab(xcb_window_t, button_array_t *);
double window_opacity_get(xcb_window_t);
double window_opacity_get_from_reply(xcb_get_property_reply_t *);
void window_opacity_set(xcb_window_t, double);
void window_grabbuttons(xcb_window_t, xcb_window_t, button_array_t *);
void window_grabkeys(xcb_window_t, key_array_t *);
void window_takefocus(xcb_window_t);
void window_set_cursor(xcb_window_t, xcb_cursor_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
