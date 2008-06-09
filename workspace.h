/*
 * workspace.h - workspace management header
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

#ifndef AWESOME_WORKSPACE_H
#define AWESOME_WORKSPACE_H

#include "structs.h"
#include "common/refcount.h"

/** Check if a client is tiled */
#define IS_TILED(client, screen)            (client && !client->isfloating && !client->ismax && client_isvisible(client, screen))

/* Contructor, destructor and referencors */
workspace_t * workspace_new(const char *, layout_t *, double, int, int);

static inline void
workspace_delete(workspace_t **workspace)
{
    p_delete(&(*workspace)->name);
    p_delete(workspace);
}

void workspace_client_set(client_t *, workspace_t *);
workspace_t * workspace_client_get(client_t *);
void workspace_client_remove(client_t *);
int workspace_screen_get(workspace_t *);

int luaA_workspace_userdata_new(workspace_t *);

DO_RCNT(workspace_t, workspace, workspace_delete)
DO_SLIST(workspace_t, workspace, workspace_delete)

DO_SLIST(workspace_client_node_t, workspace_client_node, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
