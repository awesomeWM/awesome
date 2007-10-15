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

void compileregs(Rule *, int);         /* initialize regexps of rules defined in config.h */
Bool isvisible(Client *, int, Tag *, int);
void applyrules(Client * c, awesome_config *);    /* applies rules to c */

UICB_PROTO(uicb_tag);
UICB_PROTO(uicb_togglefloating);
UICB_PROTO(uicb_toggletag);
UICB_PROTO(uicb_toggleview);
UICB_PROTO(uicb_view);
UICB_PROTO(uicb_tag_prev_selected);
UICB_PROTO(uicb_tag_viewnext);
UICB_PROTO(uicb_tag_viewprev);

#endif
