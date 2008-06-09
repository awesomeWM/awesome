/*
 * layout.c - layout management
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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>

#include "focus.h"
#include "widget.h"
#include "window.h"
#include "client.h"
#include "screen.h"
#include "workspace.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/fibonacci.h"
#include "layouts/floating.h"

extern awesome_t globalconf;

/** Arrange windows following current selected layout.
 * \param ws The workspace to arrange.
 */
static void
arrange(workspace_t *ws)
{
    client_t *c;
    int fscreen, screen = workspace_screen_get(ws), phys_screen = screen_virttophys(screen);
    xcb_query_pointer_cookie_t qp_c;
    xcb_query_pointer_reply_t *qp_r;
    workspace_t *cws;

    for(c = globalconf.clients; c; c = c->next)
        if((cws = workspace_client_get(c)) == ws && !c->ishidden)
        {
            screen_client_moveto(c, screen);
            client_unban(c);
        }
        /* we don't touch other screens windows */
        else if(c->ishidden || workspace_screen_get(cws) == -1)
            client_ban(c);

    qp_c = xcb_query_pointer_unchecked(globalconf.connection,
                                       xcb_aux_get_screen(globalconf.connection,
                                                          phys_screen)->root);

    /* check that the mouse is on a window or not */
    if((qp_r = xcb_query_pointer_reply(globalconf.connection, qp_c, NULL)))
    {
        if(qp_r->root == XCB_NONE || qp_r->child == XCB_NONE || qp_r->root == qp_r->child)
            window_root_grabbuttons();

        globalconf.pointer_x = qp_r->root_x;
        globalconf.pointer_y = qp_r->root_y;

        /* no window have focus, let's try to see if mouse is on
         * the screen we just rearranged */
        if(!globalconf.focus->client)
        {
            fscreen = screen_get_bycoord(globalconf.screens_info,
                                         screen,
                                         qp_r->root_x, qp_r->root_y);
            /* if the mouse in on the same screen we just rearranged, and no
             * client are currently focused, pick the first one in history */
            if(fscreen == screen
               && (c = focus_client_getcurrent(ws)))
                   client_focus(c);
        }

        p_delete(&qp_r);
    }

    if(ws->layout)
        ws->layout(ws);

    /* reset status */
    ws->need_arrange = false;

    /* call hook */
    luaA_workspace_userdata_new(ws);
    luaA_dofunction(globalconf.L, globalconf.hooks.arrange, 1);
}

/** Refresh the screens disposition.
 */
void
layout_refresh(void)
{
    int screen;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        if(globalconf.screens[screen].workspace && globalconf.screens[screen].workspace->need_arrange)
            arrange(globalconf.screens[screen].workspace);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
