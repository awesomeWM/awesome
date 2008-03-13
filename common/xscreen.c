/*
 * common/xscreen.c - common X screen management
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

#include <X11/extensions/Xinerama.h>

#include "common/xscreen.h"

/** Return the Xinerama screen number where the coordinates belongs to
 * \param disp Display ref
 * \param x x coordinate of the window
 * \param y y coordinate of the window
 * \return screen number or DefaultScreen of disp on no match
 */
int
screen_get_bycoord(ScreensInfo *si, int screen, int x, int y)
{
    int i;

    /* don't waste our time */
    if(!si->xinerama_is_active)
        return screen;

    for(i = 0; i < si->nscreen; i++)
        if((x < 0 || (x >= si->geometry[i].x && x < si->geometry[i].x + si->geometry[i].width))
           && (y < 0 || (y >= si->geometry[i].y && y < si->geometry[i].y + si->geometry[i].height)))
            return i;

    return screen;
}

static inline Area
screen_xsi_to_area(XineramaScreenInfo si)
{
    Area a;

    a.x = si.x_org;
    a.y = si.y_org;
    a.width = si.width;
    a.height = si.height;
    a.next = a.prev = NULL;

    return a;
}

void
screensinfo_delete(ScreensInfo **si)
{
    p_delete(&(*si)->geometry);
    p_delete(si);
}

ScreensInfo *
screensinfo_new(Display *disp)
{
    ScreensInfo *si;
    XineramaScreenInfo *xsi;
    int xinerama_screen_number, screen, screen_to_test;
    Bool drop;

    si = p_new(ScreensInfo, 1);

    if((si->xinerama_is_active = XineramaIsActive(disp)))
    {
        xsi = XineramaQueryScreens(disp, &xinerama_screen_number);
        si->geometry = p_new(Area, xinerama_screen_number);
        si->nscreen = 0;

        /* now check if screens overlaps (same x,y): if so, we take only the biggest one */
        for(screen = 0; screen < xinerama_screen_number; screen++)
        {
            drop = False;
            for(screen_to_test = 0; screen_to_test < si->nscreen; screen_to_test++)
                if(xsi[screen].x_org == si->geometry[screen_to_test].x
                   && xsi[screen].y_org == si->geometry[screen_to_test].y)
                    {
                        /* we already have a screen for this area, just check if
                         * it's not bigger and drop it */
                        drop = True;
                        si->geometry[screen_to_test].width =
                            MAX(xsi[screen].width, xsi[screen_to_test].width);
                        si->geometry[screen_to_test].height =
                            MAX(xsi[screen].height, xsi[screen_to_test].height);
                    }
            if(!drop)
                si->geometry[si->nscreen++] = screen_xsi_to_area(xsi[screen]);
        }

        /* realloc smaller if xinerama_screen_number != screen registered */
        if(xinerama_screen_number != si->nscreen)
        {
            Area *newgeometry = p_new(Area, si->nscreen);
            memcpy(newgeometry, si->geometry, si->nscreen * sizeof(Area));
            p_delete(&si->geometry);
            si->geometry = newgeometry;
        }

        XFree(xsi);
    }
    else
    {
        si->nscreen = ScreenCount(disp);
        si->geometry = p_new(Area, si->nscreen);
        for(screen = 0; screen < si->nscreen; screen++)
        {
            si->geometry[screen].x = 0;
            si->geometry[screen].y = 0;
            si->geometry[screen].width = DisplayWidth(disp, screen);
            si->geometry[screen].height = DisplayHeight(disp, screen);
        }
    }

    return si;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
