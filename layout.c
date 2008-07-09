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

#include "tag.h"
#include "focus.h"
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
    layout_t *curlay = layout_get_current(screen);
    int fscreen, phys_screen = screen_virttophys(screen);
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

    qp_c = xcb_query_pointer_unchecked(globalconf.connection,
                                       xutil_screen_get(globalconf.connection,
                                                          phys_screen)->root);

    /* check that the mouse is on a window or not */
    if((qp_r = xcb_query_pointer_reply(globalconf.connection, qp_c, NULL)))
    {
        if(qp_r->child == XCB_NONE || qp_r->root == qp_r->child)
            window_root_grabbuttons(qp_r->root);
        else if ((c = client_getbywin(qp_r->child)))
            window_grabbuttons(c->win, qp_r->root, c->buttons);

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
               && (c = focus_get_current_client(screen)))
                   client_focus(c, screen);
        }

        p_delete(&qp_r);
    }

    if(curlay)
        curlay(screen);

    /* reset status */
    globalconf.screens[screen].need_arrange = false;

    /* call hook */
    lua_pushnumber(globalconf.L, screen + 1);
    luaA_dofunction(globalconf.L, globalconf.hooks.arrange, 1, 0);
}

/** Refresh the screen disposition
 * \return true if the screen was arranged, false otherwise
 */
void
layout_refresh(void)
{
    int screen;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        if(globalconf.screens[screen].need_arrange)
            arrange(screen);
}

/** Get current layout used on screen.
 * \param screen Virtual screen number.
 * \return layout used on that screen
 */
layout_t *
layout_get_current(int screen)
{
    layout_t *l = NULL;
    tag_t **curtags = tags_get_current(screen);

    if(curtags[0])
        l = curtags[0]->layout;
    p_delete(&curtags);

    return l;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
