/*
 * event.h - event handlers header
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

#ifndef AWESOME_EVENT_H
#define AWESOME_EVENT_H

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shape.h>

#define CLEANMASK(mask)      (mask & ~(globalconf.numlockmask | XCB_MOD_MASK_LOCK))

int event_handle_buttonpress(void *, xcb_connection_t *, xcb_button_press_event_t *);
int event_handle_configurerequest(void *, xcb_connection_t *, xcb_configure_request_event_t *);
int event_handle_configurenotify(void *, xcb_connection_t *, xcb_configure_notify_event_t *);
int event_handle_destroynotify(void *, xcb_connection_t *, xcb_destroy_notify_event_t *);
int event_handle_enternotify(void *, xcb_connection_t *, xcb_enter_notify_event_t *);
int event_handle_expose(void *, xcb_connection_t *, xcb_expose_event_t *);
int event_handle_keypress(void *, xcb_connection_t *, xcb_key_press_event_t *);
int event_handle_mappingnotify(void *, xcb_connection_t *, xcb_mapping_notify_event_t *);
int event_handle_maprequest(void *, xcb_connection_t *, xcb_map_request_event_t *);
int event_handle_propertynotify(void *, xcb_connection_t *, xcb_property_notify_event_t *);
int event_handle_unmapnotify(void *, xcb_connection_t *, xcb_unmap_notify_event_t *);
int event_handle_shape(void *, xcb_connection_t *, xcb_shape_notify_event_t *);
int event_handle_randr_screen_change_notify(void *, xcb_connection_t *, xcb_randr_screen_change_notify_event_t *);
int event_handle_clientmessage(void *, xcb_connection_t *, xcb_client_message_event_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
