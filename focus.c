/*
 * focus.c - focus management
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

#include "focus.h"
#include "cnode.h"
#include "workspace.h"

extern awesome_t globalconf;

/** Push the client at the beginning of the client focus history stack.
 * \param c The client to push.
 */
void
focus_client_push(client_t *c)
{
    client_node_t *node = client_node_client_add(&globalconf.focus, c);
    client_node_list_push(&globalconf.focus, node);
}

/** Append the client at the end of the client focus history stack.
 * \param c The client to append.
 */
void
focus_client_append(client_t *c)
{
    client_node_t *node = client_node_client_add(&globalconf.focus, c);
    client_node_list_append(&globalconf.focus, node);
}

/** Remove a client from focus history.
 * \param c The client.
 */
void
focus_client_delete(client_t *c)
{
    client_node_t *node = client_node_client_getby(globalconf.focus, c);

    if(node)
    {
        client_node_list_detach(&globalconf.focus, node);
        p_delete(&node);
    }
}

static client_t *
focus_get_latest_client_for_workspace(workspace_t *ws, int nindex)
{
    client_node_t *node;
    int i = 0;

    for(node = globalconf.focus; node; node = node->next)
        if(node->client && !node->client->skip)
            if(workspace_client_get(node->client) == ws)
                if(i-- == nindex)
                    return node->client;

    return NULL;
}

/** Get the latest focused client on a workspace.
 * \param ws The workspace.
 * \return A pointer to an existing client.
 */
client_t *
focus_client_getcurrent(workspace_t *ws)
{
    return focus_get_latest_client_for_workspace(ws, 0);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
