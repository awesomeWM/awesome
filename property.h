/*
 * property.h - property handlers header
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

#ifndef AWESOME_PROPERTY_H
#define AWESOME_PROPERTY_H

#include "structs.h"

void property_update_wm_transient_for(client_t *, xcb_get_property_reply_t *);
void property_update_wm_normal_hints(client_t *, xcb_get_property_reply_t *);
void property_update_wm_hints(client_t *, xcb_get_property_reply_t *);
void property_update_wm_name(client_t *);
void property_update_wm_icon_name(client_t *);
void a_xcb_set_property_handlers(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
