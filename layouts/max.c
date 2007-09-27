/*  
 * max.c - max layout
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
#include "screen.h"
#include "layouts/max.h"

/* extern */
extern Client *clients;         /* global client */

void
layout_max(Display *disp, awesome_config *awesomeconf)
{
    Client *c;
    int screen_number = 0, use_screen = 0;
    ScreenInfo *si = get_screen_info(disp, awesomeconf->screen, &awesomeconf->statusbar, &screen_number);

    for(c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags))
        {
            /* if xinerama */
            if(screen_number > 1)
                use_screen = (use_screen == screen_number - 1) ? 0 : use_screen + 1;
            else
                use_screen = awesomeconf->screen;
            resize(c, si[use_screen].x_org, si[use_screen].y_org,
                   si[use_screen].width - 2 * c->border,
                   si[use_screen].height - 2 * c->border, awesomeconf->resize_hints);
        }
    XFree(si);
}
