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

#include <xcb/xcb_icccm.h>

#include "structs.h"

bool client_isvisible(Client *, int);
Client * client_get_bywin(Client *, xcb_window_t);
Client * client_get_byname(Client *, char *);
bool client_focus(Client *, int, bool);
void client_stack(Client *);
void client_ban(Client *);
void client_unban(Client *);
void client_manage(xcb_window_t, xcb_get_geometry_reply_t *, int);
bool client_resize(Client *, area_t, bool);
void client_unmanage(Client *);
void client_updatewmhints(Client *);
xcb_size_hints_t *client_updatesizehints(Client *);
void client_updatetitle(Client *);
void client_saveprops(Client *);
void client_kill(Client *);
void client_setfloating(Client *, bool, Layer);

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
