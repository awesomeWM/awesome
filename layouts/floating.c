/*  
 * floating.c - floating layout
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

#include "tag.h"
#include "layouts/floating.h"

/* extern */
extern Client *clients;         /* global client */

void
layout_floating(Display *disp __attribute__ ((unused)), awesome_config *awesomeconf)
{                               /* default floating layout */
    Client *c;

    for(c = clients; c; c = c->next)
        if(isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags))
        {
            if(c->ftview)
            {
                resize(c, c->rx, c->ry, c->rw, c->rh, True);
                c->ftview = False;
            }
            else
                resize(c, c->x, c->y, c->w, c->h, True);
        }
}
