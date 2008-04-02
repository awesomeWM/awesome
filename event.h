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

#include <X11/Xlib.h>

#define CLEANMASK(mask)      (mask & ~(globalconf.numlockmask | LockMask))

void event_handle_buttonpress(XEvent *);
void event_handle_configurerequest(XEvent *);
void event_handle_configurenotify(XEvent *);
void event_handle_destroynotify(XEvent *);
void event_handle_enternotify(XEvent *);
void event_handle_expose(XEvent *);
void event_handle_keypress(XEvent *);
void event_handle_leavenotify(XEvent *);
void event_handle_mappingnotify(XEvent *);
void event_handle_maprequest(XEvent *);
void event_handle_propertynotify(XEvent *);
void event_handle_unmapnotify(XEvent *);
void event_handle_shape(XEvent *);
void event_handle_randr_screen_change_notify(XEvent *);
void event_handle_clientmessage(XEvent *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
