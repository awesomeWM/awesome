/*
 * property.h - property handlers header
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_PROPERTY_H
#define AWESOME_PROPERTY_H

#include "globalconf.h"

#define PROPERTY(funcname) \
    xcb_get_property_cookie_t property_get_##funcname(client_t *c); \
    void property_update_##funcname(client_t *c, xcb_get_property_cookie_t cookie)

PROPERTY(wm_name);
PROPERTY(net_wm_name);
PROPERTY(wm_icon_name);
PROPERTY(net_wm_icon_name);
PROPERTY(wm_client_machine);
PROPERTY(wm_window_role);
PROPERTY(wm_transient_for);
PROPERTY(wm_client_leader);
PROPERTY(wm_normal_hints);
PROPERTY(wm_hints);
PROPERTY(wm_class);
PROPERTY(wm_protocols);
PROPERTY(net_wm_pid);
PROPERTY(net_wm_icon);

#undef PROPERTY

void property_handle_propertynotify(xcb_property_notify_event_t *ev);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
