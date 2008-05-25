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

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "tag.h"
#include "focus.h"
#include "widget.h"
#include "window.h"
#include "client.h"
#include "screen.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/fibonacci.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

#include "layoutgen.h"

/** Arrange windows following current selected layout
 * \param screen the screen to arrange
 */
static void
arrange(int screen)
{
    Client *c;
    Layout *curlay = layout_get_current(screen);
    unsigned int dui;
    int di, x, y, fscreen, phys_screen = screen_virttophys(screen);
    Window rootwin, childwin;

    for(c = globalconf.clients; c; c = c->next)
    {
        if(client_isvisible(c, screen) && !c->newcomer)
            client_unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == screen || c->newcomer)
            client_ban(c);
    }

    curlay->arrange(screen);

    for(c = globalconf.clients; c; c = c->next)
        if(c->newcomer && client_isvisible(c, screen))
        {
            c->newcomer = False;
            client_unban(c);
            if(globalconf.screens[screen].new_get_focus
               && !c->skip
               && (!globalconf.focus->client || !globalconf.focus->client->ismax))
                client_focus(c, screen, True);
            else if(globalconf.focus->client && globalconf.focus->client->ismax)
                client_stack(globalconf.focus->client);
        }

    /* check that the mouse is on a window or not */
    if(XQueryPointer(globalconf.display,
                     RootWindow(globalconf.display, phys_screen),
                     &rootwin, &childwin, &x, &y, &di, &di, &dui))
    {
       if(rootwin == None || childwin == None || childwin == rootwin)
           window_root_grabbuttons(phys_screen);

        globalconf.pointer_x = x;
        globalconf.pointer_y = y;

        /* no window have focus, let's try to see if mouse is on
         * the screen we just rearranged */
        if(!globalconf.focus->client)
        {
            fscreen = screen_get_bycoord(globalconf.screens_info,
                                         screen, x, y);
            /* if the mouse in on the same screen we just rearranged, and no
             * client are currently focused, pick the first one in history */
            if(fscreen == screen
               && (c = focus_get_current_client(screen)))
                   client_focus(c, screen, True);
        }
    }

    /* reset status */
    globalconf.screens[screen].need_arrange = False;
}

/** Refresh the screen disposition
 * \return true if the screen was arranged, false otherwise
 */
int
layout_refresh(void)
{
    int screen;
    int arranged = 0;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        if(globalconf.screens[screen].need_arrange)
        {
            arrange(screen);
            arranged++;
        }

    return arranged;
}

/** Get current layout used on screen
 * \param screen screen id
 * \return layout used on that screen
 */
Layout *
layout_get_current(int screen)
{
    Tag **curtags = tags_get_current(screen);
    Layout *l = curtags[0]->layout;
    p_delete(&curtags);
    return l;
}

/** Set the layout of the current tag.
 * Argument must be a relative or absolute integer of available layouts.
 * \param screen Screen ID
 * \param arg Layout specifier
 * \ingroup ui_callback
 */
void
uicb_tag_setlayout(int screen, char *arg)
{
    Layout *l = globalconf.screens[screen].layouts;
    Tag *tag, **curtags;
    int i;

    if(arg)
    {
        curtags = tags_get_current(screen);
        for(i = 0; l && l != curtags[0]->layout; i++, l = l->next);
        p_delete(&curtags);

        if(!l)
            i = 0;

        i = compute_new_value_from_arg(arg, (double) i);

        if(i >= 0)
            for(l = globalconf.screens[screen].layouts; l && i > 0; i--)
                 l = l->next;
        else
            for(l = globalconf.screens[screen].layouts; l && i < 0; i++)
                 l = layout_list_prev_cycle(&globalconf.screens[screen].layouts, l);

        if(!l)
            l = globalconf.screens[screen].layouts;
    }

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected)
            tag->layout = l;

    if(globalconf.focus->client)
        arrange(screen);

    widget_invalidate_cache(screen, WIDGET_CACHE_LAYOUTS);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
