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

#include "structs.h"

Bool client_isvisible(Client *, int);
Client * client_get_bywin(Client *, Window);
Client * client_get_byname(Client *, char *);
Bool client_focus(Client *, int, Bool);
void client_stack(Client *);
void client_ban(Client *);
void client_unban(Client *);
void client_manage(Window, XWindowAttributes *, int);
Bool client_resize(Client *, area_t, Bool);
void client_unmanage(Client *);
void client_updatewmhints(Client *);
long client_updatesizehints(Client *);
void client_updatetitle(Client *);
void client_saveprops(Client *);
void client_kill(Client *);
void client_setfloating(Client *, Bool);
void client_seturgent(Client *, Bool);

Uicb uicb_client_kill;
Uicb uicb_client_moveresize;
Uicb uicb_client_settrans;
Uicb uicb_client_swapnext;
Uicb uicb_client_swapprev;
Uicb uicb_client_togglemax;
Uicb uicb_client_toggleverticalmax;
Uicb uicb_client_togglehorizontalmax;
Uicb uicb_client_zoom;
Uicb uicb_client_focusnext;
Uicb uicb_client_focusprev;
Uicb uicb_client_togglefloating;
Uicb uicb_client_togglescratch;
Uicb uicb_client_setscratch;

DO_SLIST(Client, client, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
