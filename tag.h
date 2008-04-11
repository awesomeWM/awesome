/*
 * tag.h - tag management header
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

#ifndef AWESOME_TAG_H
#define AWESOME_TAG_H

#include "structs.h"

/** Check if a client is tiled */
#define IS_TILED(client, screen)            (client && !client->isfloating && !client->ismax && client_isvisible(client, screen))

Tag * tag_new(const char *, Layout *, double, int, int);
void tag_view(Tag *, bool);
void tag_view_byindex(int, int, bool);
void tag_push_to_screen(Tag *, int);
Tag ** tags_get_current(int);
void tag_client(Client *, Tag *);
void untag_client(Client *, Tag *);
bool is_client_tagged(Client *, Tag *);
void tag_client_with_rule(Client *, rule_t *r);
void tag_client_with_current_selected(Client *);
void tag_view_only_byindex(int, int);
void tag_append_to_screen(Tag *, int);

uicb_t uicb_client_tag;
uicb_t uicb_client_toggletag;
uicb_t uicb_tag_toggleview;
uicb_t uicb_tag_view;
uicb_t uicb_tag_prev_selected;
uicb_t uicb_tag_viewnext;
uicb_t uicb_tag_viewprev;
uicb_t uicb_tag_create;

DO_SLIST(Tag, tag, p_delete)
DO_SLIST(tag_client_node_t, tag_client_node, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
