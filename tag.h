/*
 * tag.h - tag management header
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

#ifndef AWESOME_TAG_H
#define AWESOME_TAG_H

#include "client.h"

/** Check if a client is tiled */
#define IS_TILED(client, screen, tags, ntags)            (client && !client->isfloating && isvisible(client, screen, tags, ntags))

Bool isvisible(Client *, int, Tag *, int);
void tag_client_with_current_selected(Client *, awesome_config *, int);

UICB_PROTO(uicb_client_tag);
UICB_PROTO(uicb_client_togglefloating);
UICB_PROTO(uicb_client_toggletag);
UICB_PROTO(uicb_tag_toggleview);
UICB_PROTO(uicb_tag_view);
UICB_PROTO(uicb_tag_prev_selected);
UICB_PROTO(uicb_tag_viewnext);
UICB_PROTO(uicb_tag_viewprev);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
