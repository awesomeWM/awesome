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

bool client_isvisible(client_t *, int);
client_t * client_get_bywin(client_t *, xcb_window_t);
client_t * client_get_byname(client_t *, char *);
bool client_focus(client_t *, int, bool);
void client_stack(client_t *);
void client_ban(client_t *);
void client_unban(client_t *);
void client_manage(xcb_window_t, xcb_get_geometry_reply_t *, int);
bool client_resize(client_t *, area_t, bool);
void client_unmanage(client_t *);
void client_updatewmhints(client_t *);
xcb_size_hints_t *client_updatesizehints(client_t *);
void client_updatetitle(client_t *);
void client_saveprops(client_t *);
void client_kill(client_t *);
void client_setfloating(client_t *, bool, layer_t);
char * client_markup_parse(client_t *, const char *, ssize_t);
style_t * client_style_get(client_t *);

uicb_t uicb_client_kill;
uicb_t uicb_client_moveresize;
uicb_t uicb_client_settrans;
uicb_t uicb_client_swap;
uicb_t uicb_client_togglemax;
uicb_t uicb_client_zoom;
uicb_t uicb_client_focus;
uicb_t uicb_client_togglefloating;
uicb_t uicb_client_togglescratch;
uicb_t uicb_client_setscratch;

DO_SLIST(client_t, client, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
