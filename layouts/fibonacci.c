/*
 * fibonacci.c - fibonacci layout
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

#include "screen.h"
#include "tag.h"
#include "client.h"
#include "layouts/fibonacci.h"
#include "common/util.h"

extern AwesomeConf globalconf;

static void
layout_fibonacci(int screen, int shape)
{
    int n = 0, i = 0;
    Client *c;
    Area geometry, area;
    geometry = area = get_screen_area(screen,
                                      globalconf.screens[screen].statusbar,
                                      &globalconf.screens[screen].padding);

    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
            n++;
    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
        {
            c->ismax = False;
            if((i % 2 && geometry.height / 2 > 2 * c->border)
               || (!(i % 2) && geometry.width / 2 > 2 * c->border))
            {
                if(i < n - 1)
                {
                    if(i % 2)
                        geometry.height /= 2;
                    else
                        geometry.width /= 2;
                    if((i % 4) == 2 && !shape)
                        geometry.x += geometry.width;
                    else if((i % 4) == 3 && !shape)
                        geometry.y += geometry.height;
                }
                if((i % 4) == 0)
                {
                    if(shape)
                        geometry.y += geometry.height;
                    else
                        geometry.y -= geometry.height;
                }
                else if((i % 4) == 1)
                    geometry.x += geometry.width;
                else if((i % 4) == 2)
                    geometry.y += geometry.height;
                else if((i % 4) == 3)
                {
                    if(shape)
                        geometry.x += geometry.width;
                    else
                        geometry.x -= geometry.width;
                }
                if(i == 0)
                    geometry.y = area.y;
                i++;
            }
            geometry.width -= 2 * c->border;
            geometry.height -= 2 * c->border;
            client_resize(c, geometry, globalconf.screens[screen].resize_hints);
            geometry.width += 2 * c->border;
            geometry.height += 2 * c->border;
        }
}

void
layout_spiral(int screen)
{
    layout_fibonacci(screen, 0);
}

void
layout_dwindle(int screen)
{
    layout_fibonacci(screen, 1);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
