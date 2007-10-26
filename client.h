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

/** Mask shorthands, used in event.c and client.c */
#define BUTTONMASK              (ButtonPressMask | ButtonReleaseMask)

Client * get_client_bywin(Client *, Window);
void grabbuttons(Client *, Bool, Bool, KeySym, unsigned int);
inline void client_attach(Client **, Client *);
inline void client_detach(Client **, Client *);
void client_reattach_after(Client *, Client *);
void ban(Client *);
void focus(Client *, Bool, awesome_config *);
void manage(Window, XWindowAttributes *, awesome_config *);
void resize(Client *, int, int, int, int, awesome_config *, Bool);
void unban(Client *);
void unmanage(Client *, long, awesome_config *);
inline void updatesizehints(Client *);
void updatetitle(Client *);
void saveprops(Client *, int);
void set_shape(Client *);

UICB_PROTO(uicb_killclient);
UICB_PROTO(uicb_moveresize);
UICB_PROTO(uicb_settrans);
UICB_PROTO(uicb_setborder);
UICB_PROTO(uicb_swapnext);
UICB_PROTO(uicb_swapprev);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
