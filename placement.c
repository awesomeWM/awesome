/*
 * placement.c - client placement management
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

#include "placement.h"
#include "screen.h"
#include "client.h"

extern AwesomeConf globalconf;

name_func_link_t FloatingPlacementList[] =
{
    { "smart", placement_smart },
    { "under_mouse", placement_under_mouse },
    { NULL, NULL }
};

static area_t
placement_fix_offscreen(area_t geometry, int screen, int border)
{
    area_t screen_geometry, newgeometry = geometry;

    screen_geometry = screen_get_area(screen,
                                      globalconf.screens[screen].statusbar,
                                      &globalconf.screens[screen].padding);

    /* fix offscreen */
    if(AREA_RIGHT(newgeometry) > AREA_RIGHT(screen_geometry))
        newgeometry.x = screen_geometry.x + screen_geometry.width - (newgeometry.width + 2 * border);
    else if(AREA_LEFT(newgeometry) < AREA_LEFT(screen_geometry))
        newgeometry.x = screen_geometry.x;

    if(AREA_BOTTOM(newgeometry) > AREA_BOTTOM(screen_geometry))
        newgeometry.y = screen_geometry.y + screen_geometry.height - (newgeometry.height + 2 * border);
    else if(AREA_TOP(newgeometry) < AREA_TOP(screen_geometry))
        newgeometry.y = screen_geometry.y;

    return newgeometry;
}

/** Compute smart coordinates for a client window
 * \param geometry current/requested client geometry
 * \param screen screen used
 * \return new geometry
 */
area_t
placement_smart(area_t geometry, int border, int screen)
{
    Client *c;
    area_t newgeometry = { 0, 0, 0, 0, NULL, NULL };
    area_t *screen_geometry, *arealist = NULL, *r;
    Bool found = False;

    screen_geometry = p_new(area_t, 1);

    *screen_geometry = screen_get_area(screen,
                                       globalconf.screens[screen].statusbar,
                                       &globalconf.screens[screen].padding);

    area_list_push(&arealist, screen_geometry);

    for(c = globalconf.clients; c; c = c->next)
        if(client_isvisible(c, screen))
        {
            newgeometry = c->f_geometry;
            newgeometry.width += 2 * c->border;
            newgeometry.height += 2 * c->border;
            area_list_remove(&arealist, &newgeometry);
        }

    newgeometry.x = geometry.x;
    newgeometry.y = geometry.y;
    newgeometry.width = 0;
    newgeometry.height = 0;

    for(r = arealist; r; r = r->next)
        if(r->width >= geometry.width && r->height >= geometry.height
           && r->width * r->height > newgeometry.width * newgeometry.height)
        {
            found = True;
            newgeometry = *r;
        }

    /* we did not found a space with enough space for our size:
     * just take the biggest available and go in */
    if(!found)
        for(r = arealist; r; r = r->next)
           if(r->width * r->height > newgeometry.width * newgeometry.height)
               newgeometry = *r;

    /* restore height and width */
    newgeometry.width = geometry.width;
    newgeometry.height = geometry.height;

    newgeometry = placement_fix_offscreen(newgeometry, screen, border);

    area_list_wipe(&arealist);

    return newgeometry;
}

area_t
placement_under_mouse(area_t geometry, int border, int screen)
{
    Window dummy;
    unsigned int m;
    int x, y, d;
    area_t finalgeometry = geometry;

    if(XQueryPointer(globalconf.display, RootWindow(globalconf.display, get_phys_screen(screen)),
                     &dummy, &dummy, &x, &y, &d, &d, &m))
    {
        finalgeometry.x = x - geometry.width / 2;
        finalgeometry.y = y - geometry.height / 2;
    }

    finalgeometry = placement_fix_offscreen(finalgeometry, screen, border);

    return finalgeometry;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
