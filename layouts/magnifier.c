/*
 * magnifier.c - magnifier layout
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

#include <math.h>

#include "client.h"
#include "tag.h"
#include "screen.h"
#include "layouts/magnifier.h"

extern awesome_t globalconf;

void
layout_magnifier(int screen)
{
    int n = 0;
    client_t *c, *focus;
    tag_t **curtags = tags_get_current(screen);
    area_t geometry, area = screen_area_get(&globalconf.screens[screen].geometry,
                                            globalconf.screens[screen].statusbar,
                                            &globalconf.screens[screen].padding);

    focus = globalconf.screens[screen].client_focus;

    /* If focused window is not tiled, take the first one which is tiled. */
    if(!IS_TILED(focus, screen))
        for(focus = globalconf.clients; focus && !IS_TILED(focus, screen); focus = focus->next);

    /* No windows is tiled, nothing to do. */
    if(!focus)
        goto bailout;

    geometry.width = sqrt(area.width * area.width * curtags[0]->mwfact);
    geometry.height = sqrt(area.height * area.height * curtags[0]->mwfact);
    geometry.x = area.x + (area.width - geometry.width) / 2;
    geometry.y = area.y + (area.height - geometry.height) / 2;
    client_resize(focus, geometry, focus->honorsizehints);
    client_raise(focus);

    for(c = client_list_prev_cycle(&globalconf.clients, focus);
        c && c != focus;
        c = client_list_prev_cycle(&globalconf.clients, c))
        if(IS_TILED(c, screen) && c != focus)
            n++;

    /* No other clients. */
    if(!n)
        goto bailout;

    geometry.x = area.x;
    geometry.y = area.y;
    geometry.height = area.height / n;
    geometry.width = area.width;

    for(c = client_list_prev_cycle(&globalconf.clients, focus);
        c && c != focus;
        c = client_list_prev_cycle(&globalconf.clients, c))
        if(IS_TILED(c, screen) && c != focus)
        {
            geometry.height -= 2 * c->border;
            geometry.width -= 2 * c->border;
            client_resize(c, geometry, c->honorsizehints);
            geometry.height += 2 * c->border;
            geometry.width += 2 * c->border;
            geometry.y += geometry.height;
        }

bailout:
    p_delete(&curtags);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
