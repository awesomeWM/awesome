/*
 * client.h - client management header
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

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include "common.h"

Client * get_client_bywin(Client *, Window);
void client_attach(Client **, Client *);
void client_detach(Client **, Client *);
void client_ban(Client *);
void focus(Client *, Bool, awesome_config *, int);
void client_manage(Window, XWindowAttributes *, awesome_config *, int);
void client_resize(Client *, int, int, int, int, awesome_config *, Bool, Bool);
void client_unban(Client *);
void client_unmanage(Client *, long, awesome_config *);
void client_updatesizehints(Client *);
void client_updatetitle(Client *);
void client_saveprops(Client *, VirtScreen *);
void tag_client_with_rules(Client *, awesome_config *);

UICB_PROTO(uicb_client_kill);
UICB_PROTO(uicb_client_moveresize);
UICB_PROTO(uicb_client_settrans);
UICB_PROTO(uicb_setborder);
UICB_PROTO(uicb_client_swapnext);
UICB_PROTO(uicb_client_swapprev);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
