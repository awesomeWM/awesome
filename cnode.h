/*
 * cnode.h - client node lists management header
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

#ifndef AWESOME_CNODE_H
#define AWESOME_CNODE_H

#include "client.h"
#include "common/list.h"

struct client_node
{
    /** The client */
    client_t *client;
    /** Next and previous client_nodes */
    client_node_t *prev, *next;
};

client_node_t * client_node_client_getby(client_node_t *, client_t *);
client_node_t * client_node_client_add(client_node_t **, client_t *);

DO_SLIST(client_node_t, client_node, p_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
