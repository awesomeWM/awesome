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

#include "tag.h"
#include "focus.h"
#include "client.h"

extern AwesomeConf globalconf;

static client_node_t *
focus_get_node_by_client(client_t *c)
{
    client_node_t *node;

    for(node = globalconf.focus; node; node = node->next)
        if(node->client == c)
            return node;

    return NULL;
}

void
focus_add_client(client_t *c)
{
    client_node_t *node;

    /* if we don't find this node, create a new one */
    if(!(node = focus_get_node_by_client(c)))
    {
        node = p_new(client_node_t, 1);
        node->client = c;
    }
    else /* if we've got a node, detach it */
        client_node_list_detach(&globalconf.focus, node);

    client_node_list_push(&globalconf.focus, node);
}

void
focus_delete_client(client_t *c)
{
    client_node_t *node = focus_get_node_by_client(c);

    if(node)
    {
        client_node_list_detach(&globalconf.focus, node);
        p_delete(&node);
    }
}

static client_t *
focus_get_latest_client_for_tags(tag_t **t, int nindex)
{
    client_node_t *node;
    tag_t **tags;
    int i = 0;

    for(node = globalconf.focus; node; node = node->next)
        if(node->client && ((!node->client->skip
           && node->client != globalconf.scratch.client)
           || globalconf.scratch.isvisible))
            for(tags = t; *tags; tags++)
                if(is_client_tagged(node->client, *tags))
                {
                    if(i == nindex)
                        return node->client;
                    else
                        i--;
                }

    return NULL;
}

client_t *
focus_get_current_client(int screen)
{
    tag_t **curtags = tags_get_current(screen);
    client_t *sel = focus_get_latest_client_for_tags(curtags, 0);
    p_delete(&curtags);

    return sel;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
