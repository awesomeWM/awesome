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
#include "tag.h"
#include "window.h"
#include "screen.h"
#include "titlebar.h"

/** Reban windows following current selected tags.
 * \param screen The screen to arrange.
 */
static void
reban(screen_t *screen)
{
    uint32_t select_input_val[] = { CLIENT_SELECT_INPUT_EVENT_MASK & ~(XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW) };

    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        /* Bob Marley v2:
         * While we arrange, we do not want to receive EnterNotify or LeaveNotify
         * events, or we would get spurious events. */
        xcb_change_window_attributes(globalconf.connection,
                                     c->win,
                                     XCB_CW_EVENT_MASK,
                                     select_input_val);

        /* Restore titlebar before client, so geometry is ok again. */
        if(titlebar_isvisible(c, screen))
            titlebar_unban(c->titlebar);

        if(client_isvisible(c, screen))
            client_unban(c);
    }

    /* Some people disliked the short flicker of background, so we first unban everything.
     * Afterwards we ban everything we don't want. This should avoid that. */
    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;

        if(!titlebar_isvisible(c, screen) && c->screen == screen)
            titlebar_ban(c->titlebar);

        /* we don't touch other screens windows */
        if(!client_isvisible(c, screen) && c->screen == screen)
            client_ban(c);
    }

    screen->need_reban = false;

    /* Now, we want to receive EnterNotify and LeaveNotify events back. */
    select_input_val[0] = CLIENT_SELECT_INPUT_EVENT_MASK;
    foreach(c, globalconf.clients)
        xcb_change_window_attributes(globalconf.connection,
                                     (*c)->win,
                                     XCB_CW_EVENT_MASK,
                                     select_input_val);
}

/** Refresh the client disposition.
 * \return true if the screen was arranged, false otherwise
 */
void
banning_refresh(void)
{
    foreach(screen, globalconf.screens)
        if(screen->need_reban)
            reban(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
