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

#include "tag.h"
#include "screen.h"
#include "client.h"
#include "layouts/max.h"

extern awesome_t globalconf;

static void
layout_fmax(int screen, bool fs)
{
    client_t *c;
    area_t area = screen_area_get(screen,
                                  fs ? NULL : &globalconf.screens[screen].wiboxes,
                                  fs ? NULL : &globalconf.screens[screen].padding,
                                  true);

    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
        {
            area.width -= 2 * c->border;
            area.height -= 2 * c->border;
            client_resize(c, area, false);
            area.width += 2 * c->border;
            area.height += 2 * c->border;
        }

    if(IS_TILED(globalconf.screens[screen].client_focus, screen))
        client_raise(globalconf.screens[screen].client_focus);
}

void
layout_max(int screen)
{
    return layout_fmax(screen, false);
}

void
layout_fullscreen(int screen)
{
    return layout_fmax(screen, true);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
