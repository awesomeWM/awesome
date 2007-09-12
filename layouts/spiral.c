/* 
 * spiral.c - spiral layout
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info> 
 * Copyright © 2007 Jeroen Schot <schot@a-eskwadraat.nl>
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

#include "awesome.h"
#include "layout.h"
#include "tag.h"
#include "spiral.h"

extern Client *clients;         /* global client list */
extern DC dc;

static void
fibonacci(Display *disp, awesome_config *awesomeconf, int shape)
{
    int n, nx, ny, nh, nw, i;
    Client *c;

    nx = get_windows_area_x(awesomeconf->statusbar);
    ny = 0;
    nw = get_windows_area_width(disp, awesomeconf->statusbar);
    nh = get_windows_area_height(disp, awesomeconf->statusbar);
    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;
    for(i = 0, c = clients; c; c = c->next)
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
                ny = get_windows_area_y(awesomeconf->statusbar);
            i++;
        }
        resize(c, nx, ny, nw - 2 * c->border, nh - 2 * c->border, False);
    }
}


void
dwindle(Display *disp, awesome_config *awesomeconf)
{
    fibonacci(disp, awesomeconf, 1);
}

void
spiral(Display *disp, awesome_config *awesomeconf)
{
    fibonacci(disp, awesomeconf, 0);
}
