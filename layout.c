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

#include "layout.h"
#include "tag.h"
#include "window.h"
#include "screen.h"

extern awesome_t globalconf;

/** Arrange windows following current selected layout
 * \param screen the screen to arrange
 */
static void
arrange(int screen)
{
    client_t *c;
    int phys_screen = screen_virttophys(screen);
    xcb_query_pointer_cookie_t qp_c;
    xcb_query_pointer_reply_t *qp_r;

    for(c = globalconf.clients; c; c = c->next)
    {
        if(client_isvisible(c, screen))
            client_unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == screen)
            client_ban(c);
    }

    /* call hook */
    if(globalconf.hooks.arrange != LUA_REFNIL)
    {
        lua_pushnumber(globalconf.L, screen + 1);
        luaA_dofunction(globalconf.L, globalconf.hooks.arrange, 1, 0);
    }

    qp_c = xcb_query_pointer_unchecked(globalconf.connection,
                                       xutil_screen_get(globalconf.connection,
                                                          phys_screen)->root);

    /* check that the mouse is on a window or not */
    if((qp_r = xcb_query_pointer_reply(globalconf.connection, qp_c, NULL)))
    {
        if(qp_r->child == XCB_NONE || qp_r->root == qp_r->child)
            window_root_buttons_grab(qp_r->root);
        else if ((c = client_getbywin(qp_r->child)))
            window_buttons_grab(c->win, qp_r->root, &c->buttons);

        globalconf.pointer_x = qp_r->root_x;
        globalconf.pointer_y = qp_r->root_y;

        p_delete(&qp_r);
    }

    /* reset status */
    globalconf.screens[screen].need_arrange = false;
}

/** Refresh the screen disposition
 * \return true if the screen was arranged, false otherwise
 */
void
layout_refresh(void)
{
    int screen;

    for(screen = 0; screen < globalconf.nscreen; screen++)
        if(globalconf.screens[screen].need_arrange)
            arrange(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
