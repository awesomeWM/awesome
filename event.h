/*
 * event.h - event handlers header
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

#ifndef AWESOME_EVENT_H
#define AWESOME_EVENT_H

#include "objects/client.h"

/* luaa.c */
void luaA_emit_refresh(void);

static inline int
awesome_refresh(void)
{
    luaA_emit_refresh();
    banning_refresh();
    stack_refresh();
    client_focus_refresh();
    return xcb_flush(globalconf.connection);
}

void event_handle(xcb_generic_event_t *event);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
