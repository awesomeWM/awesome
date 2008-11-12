/*
 * stack.c - client stack management
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

#include "stack.h"
#include "cnode.h"
#include "ewmh.h"

extern awesome_t globalconf;

/** Push the client at the beginning of the client stack.
 * \param c The client to push.
 */
void
stack_client_push(client_t *c)
{
    client_node_t *node = client_node_client_add(&globalconf.stack, c);
    client_node_list_push(&globalconf.stack, node);
    ewmh_update_net_client_list_stacking(c->phys_screen);
}

/** Push the client at the end of the client stack.
 * \param c The client to push.
 */
void
stack_client_append(client_t *c)
{
    client_node_t *node = client_node_client_add(&globalconf.stack, c);
    client_node_list_append(&globalconf.stack, node);
    ewmh_update_net_client_list_stacking(c->phys_screen);
}

/** Remove a client from stack history.
 * \param c The client.
 */
void
stack_client_delete(client_t *c)
{
    client_node_t *node = client_node_client_getby(globalconf.stack, c);

    if(node)
    {
        client_node_list_detach(&globalconf.stack, node);
        p_delete(&node);
        ewmh_update_net_client_list_stacking(c->phys_screen);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
