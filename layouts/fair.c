/*
 * fair.c - fair layout
 *
 * Copyright © 2008 Alex Cornejo <acornejo@gmail.com>
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

#include "screen.h"
#include "tag.h"
#include "client.h"
#include "layouts/fair.h"
#include "common/util.h"

extern awesome_t globalconf;

static void
layout_fair(int screen, const orientation_t orientation)
{
    int strips=1, cells = 1,
        strip = 0, cell = 0, n = 0,
        full_strips;
    client_t *c;
    area_t geometry, area;

    area = screen_area_get(screen,
                           &globalconf.screens[screen].wiboxes,
                           &globalconf.screens[screen].padding,
                           true);

    for(c = globalconf.clients ; c; c = c->next)
        if(IS_TILED(c, screen))
            ++n;

    if(n > 0)
    {
        while(cells * cells < n)
            ++cells;

        strips = (cells * (cells - 1) >= n) ? cells - 1 : cells;
        full_strips = n - strips * (cells - 1);

        for(c = globalconf.clients; c; c = c->next)
            if(IS_TILED(c, screen))
            {
                if (((orientation == East) && (n > 2))
                    || ((orientation == South) && (n <= 2)))
                {
                    geometry.width = area.width / cells;
                    geometry.height = area.height / strips;
                    geometry.x = area.x + cell * geometry.width;
                    geometry.y = area.y + strip * geometry.height;
                }
                else
                {
                    geometry.width = area.width / strips;
                    geometry.height = area.height / cells;
                    geometry.x = area.x + strip * geometry.width;
                    geometry.y = area.y + cell * geometry.height;
                }
                geometry.width -= 2 * c->border;
                geometry.height -= 2 * c->border;

                client_resize(c, geometry, c->honorsizehints);

                if(++cell == cells)
                {
                    cell = 0;
                    if(++strip == full_strips)
                        cells--;
                }
            }
    }
}

void
layout_fairh(int screen)
{
    layout_fair(screen, East);
}

void
layout_fairv(int screen)
{
    layout_fair(screen, South);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
