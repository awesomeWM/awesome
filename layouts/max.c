/*
 * max.c - max layout
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

#include "workspace.h"
#include "screen.h"
#include "client.h"
#include "focus.h"
#include "layouts/max.h"

extern awesome_t globalconf;

void
layout_max(workspace_t *ws)
{
    client_t *c, *focus;
    int screen = workspace_screen_get(ws);
    area_t area = screen_area_get(screen,
                                  globalconf.screens[screen].statusbar,
                                  &globalconf.screens[screen].padding);

    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
        {
            area.width -= 2 * c->border;
            area.height -= 2 * c->border;
            client_resize(c, area, false);
            area.width += 2 * c->border;
            area.height += 2 * c->border;
        }

    if((focus = focus_client_getcurrent(ws))
       && IS_TILED(focus, screen))
        client_raise(focus);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
