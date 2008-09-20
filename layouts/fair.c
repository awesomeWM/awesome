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

typedef enum
{
    HORIZONTAL,
    VERTICAL
} orientation_t;

static void
layout_fair(int screen, const orientation_t orientation)
{
    int u_divisions=1, v_divisions = 1,
        u = 0, v = 0, n = 0;
    client_t *c;
    area_t geometry, area;

    area = screen_area_get(screen,
                           &globalconf.screens[screen].statusbars,
                           &globalconf.screens[screen].padding,
                           true);

    for(c = globalconf.clients ; c; c = c->next)
        if(IS_TILED(c, screen))
            ++n;

    if(n > 0)
    {
        while(u_divisions * u_divisions < n)
            ++u_divisions;

        v_divisions = (u_divisions * (u_divisions - 1) >= n) ? u_divisions - 1 : u_divisions;

        for(c = globalconf.clients; c; c = c->next)
            if(IS_TILED(c, screen))
            {
                if (orientation == HORIZONTAL)
                {
                    geometry.width = area.width / u_divisions;
                    geometry.height = area.height / v_divisions;
                    geometry.x = area.x + u * geometry.width;
                    geometry.y = area.y + v * geometry.height;
                }
                else
                {
                    geometry.width = area.width / v_divisions;
                    geometry.height = area.height / u_divisions;
                    geometry.x = area.x + v * geometry.width;
                    geometry.y = area.y + u * geometry.height;
                }
                geometry.width -= 2 * c->border;
                geometry.height -= 2 * c->border;

                client_resize(c, geometry, c->honorsizehints);

                if(++u == u_divisions)
                {
                    u = 0;
                    if(++v == v_divisions - 1)
                        u_divisions = u_divisions - u_divisions * v_divisions + n;
                }
            }
    }
}

void
layout_fairh(int screen)
{
    layout_fair(screen, HORIZONTAL);
}

void
layout_fairv(int screen)
{
    layout_fair(screen, VERTICAL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
