/*
 * xcb_event_handler.c - xcb event handler header
 *
 * Copyright © 2007-2008 Arnaud Fontaine <arnaud@andesi.org>
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

#ifndef XCB_EVENT_HANDLER_H
#define XCB_EVENT_HANDLER_H

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>

/* Set the same handler for all errors */
void xcb_set_error_handler_catch_all(xcb_event_handlers_t *,
                                     xcb_generic_error_handler_t,
                                     void *);

/* Number of different errors */
#define ERRORS_NBR 256

/* Number of different events */
#define EVENTS_NBR 126

/* Get the error name from its code */
extern const char *x_label_error[];

/* Get a request name from its code */
extern const char *x_label_request[];

#endif
