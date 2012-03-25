/*
 * stack.c - client stack management
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include "ewmh.h"
#include "stack.h"
#include "client.h"

void
stack_client_remove(client_t *c)
{
    foreach(client, globalconf.stack)
        if(*client == c)
        {
            client_array_remove(&globalconf.stack, client);
            break;
        }
    ewmh_update_net_client_list_stacking(c->phys_screen);
}

/** Push the client at the beginning of the client stack.
 * \param c The client to push.
 */
void
stack_client_push(client_t *c)
{
    stack_client_remove(c);
    client_array_push(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking(c->phys_screen);
}

/** Push the client at the end of the client stack.
 * \param c The client to push.
 */
void
stack_client_append(client_t *c)
{
    stack_client_remove(c);
    client_array_append(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking(c->phys_screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
