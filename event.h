/*
 * event.h - event handlers header
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include "config.h"

void grabkeys(awesome_config *);

void handle_event_buttonpress(XEvent *, awesome_config *);
void handle_event_configurerequest(XEvent *, awesome_config *);
void handle_event_configurenotify(XEvent *, awesome_config *);
void handle_event_destroynotify(XEvent *, awesome_config *);
void handle_event_enternotify(XEvent *, awesome_config *);
void handle_event_expose(XEvent *, awesome_config *);
void handle_event_keypress(XEvent *, awesome_config *);
void handle_event_leavenotify(XEvent *, awesome_config *);
void handle_event_mappingnotify(XEvent *, awesome_config *);
void handle_event_maprequest(XEvent *, awesome_config *);
void handle_event_propertynotify(XEvent *, awesome_config *);
void handle_event_unmapnotify(XEvent *, awesome_config *);
void handle_event_shape(XEvent *, awesome_config *);
void handle_event_randr_screen_change_notify(XEvent *, awesome_config *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
