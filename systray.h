/*
 * systray.h - systray handlers header
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_SYSTRAY_H
#define AWESOME_SYSTRAY_H

#include <xcb/xcb.h>
#include <lua.h>
#include "common/xembed.h"

void systray_init(void);
void systray_cleanup(void);
int systray_request_handle(xcb_window_t, xembed_info_t *);
bool systray_iskdedockapp(xcb_window_t);
int systray_process_client_message(xcb_client_message_event_t *);
int xembed_process_client_message(xcb_client_message_event_t *);
int luaA_systray(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
