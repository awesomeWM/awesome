/*
 * fibonacci.c - fibonacci layout
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include "util.h"
#include "layouts/fibonacci.h"

extern awesome_config globalconf;

static void
layout_fibonacci(int screen, int shape)
{
    int n = 0, i = 0, nx, ny, nw, nh;
    Client *c;
    ScreenInfo *si = get_screen_info(globalconf.display, screen,
                                     &globalconf.screens[screen].statusbar,
                                     &globalconf.screens[screen].padding);

    nx = si[screen].x_org;
    ny = si[screen].y_org;
    nw = si[screen].width;
    nh = si[screen].height;

    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
            n++;
    for(c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
        {
            c->ismax = False;
            if((i % 2 && nh / 2 > 2 * c->border)
               || (!(i % 2) && nw / 2 > 2 * c->border))
            {
                if(i < n - 1)
                {
                    if(i % 2)
                        nh /= 2;
                    else
                        nw /= 2;
                    if((i % 4) == 2 && !shape)
                        nx += nw;
                    else if((i % 4) == 3 && !shape)
                        ny += nh;
                }
                if((i % 4) == 0)
                {
                    if(shape)
                        ny += nh;
                    else
                        ny -= nh;
                }
                else if((i % 4) == 1)
                    nx += nw;
                else if((i % 4) == 2)
                    ny += nh;
                else if((i % 4) == 3)
                {
                    if(shape)
                        nx += nw;
                    else
                        nx -= nw;
                }
                if(i == 0)
                    ny = si[screen].y_org;
                i++;
            }
            client_resize(c, nx, ny, nw - 2 * c->border, nh - 2 * c->border,
                          globalconf.screens[screen].resize_hints, False);
        }
    p_delete(&si);
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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
