/*
 * cnode.c - client node lists management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include "cnode.h"

/** Get the client's node focus.
 * \param list The client list.
 * \param c The client.
 * \return The client node focus.
 */
client_node_t *
client_node_client_getby(client_node_t *list, client_t *c)
{
    client_node_t *node;

    for(node = list; node; node = node->next)
        if(node->client == c)
            return node;

    return NULL;
}

/** Create a client node for focus.
 * \param list The client list.
 * \param c The client
 * \return The client focus node.
 */
client_node_t *
client_node_client_add(client_node_t **list, client_t *c)
{
    client_node_t *node;

    /* if we don't find this node, create a new one */
    if(!(node = client_node_client_getby(*list, c)))
    {
        node = p_new(client_node_t, 1);
        node->client = c;
    }
    else /* if we've got a node, detach it */
        client_node_list_detach(list, node);

    return node;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
