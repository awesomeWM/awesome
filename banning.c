/*
 * banning.c - client banning management
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include "banning.h"
#include "globalconf.h"
#include "objects/client.h"

/** Check all clients if they need to rebanned
 */
static gboolean
banning_refresh(gpointer unused)
{
    assert(globalconf.banning_update_id != 0);
    globalconf.banning_update_id = 0;

    foreach(c, globalconf.clients)
        if(client_isvisible(*c))
            client_unban(*c);

    /* Some people disliked the short flicker of background, so we first unban everything.
     * Afterwards we ban everything we don't want. This should avoid that. */
    foreach(c, globalconf.clients)
        if(!client_isvisible(*c))
            client_ban(*c);

    return G_SOURCE_REMOVE;
}

/** Reban windows following current selected tags.
 */
void
banning_need_update(void)
{
    /* We update the complete banning only once per main loop to avoid
     * excessive updates...  */
    if (globalconf.banning_update_id == 0)
        globalconf.banning_update_id = g_idle_add(banning_refresh, NULL);

    /* But if a client will be banned in our next update we unfocus it now. */
    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;

        if(!client_isvisible(c))
            client_ban_unfocus(c);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
