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
#include "common/refcount.h"

/** Check if a client is tiled */
#define IS_TILED(client, screen)            (client && !client->isfloating && client_isvisible(client, screen))

/* Contructor, destructor and referencors */
tag_t * tag_new(const char *, layout_t *, double, int, int);

static inline void
tag_delete(tag_t **tag)
{
    p_delete(&(*tag)->name);
    p_delete(tag);
}

tag_t ** tags_get_current(int);
void tag_client(client_t *, tag_t *);
void untag_client(client_t *, tag_t *);
bool is_client_tagged(client_t *, tag_t *);
void tag_client_with_current_selected(client_t *);
void tag_view_only_byindex(int, int);
void tag_append_to_screen(tag_t *, int);
int luaA_tag_userdata_new(tag_t *);

DO_RCNT(tag_t, tag, tag_delete)
DO_SLIST(tag_t, tag, tag_delete)
DO_SLIST_UNREF(tag_t, tag, tag_delete)

DO_SLIST(tag_client_node_t, tag_client_node, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
