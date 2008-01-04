/*
 * client.h - client management header
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

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include "config.h"

Bool client_isvisible(Client *, int);
Client * get_client_bywin(Client *, Window);
Client * get_client_byname(Client *, char *);
void client_attach(Client *);
void client_detach(Client *);
void client_ban(Client *);
void focus(Client *, Bool, int);
void client_manage(Window, XWindowAttributes *, int);
void client_resize(Client *, int, int, int, int, Bool, Bool);
void client_unban(Client *);
void client_unmanage(Client *, long);
void client_updatewmhints(Client *);
void client_updatesizehints(Client *);
void client_updatetitle(Client *);
void client_saveprops(Client *); 
void client_kill(Client *);
void client_maximize(Client *c, int, int, int, int, Bool);

Uicb uicb_client_kill;
Uicb uicb_client_moveresize;
Uicb uicb_client_settrans;
Uicb uicb_client_swapnext;
Uicb uicb_client_swapprev;
Uicb uicb_client_togglemax; 
Uicb uicb_client_toggleverticalmax; 
Uicb uicb_client_togglehorizontalmax; 
Uicb uicb_client_zoom;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
