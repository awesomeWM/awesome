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
#include "titlebar.h"

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
    area_t screen_geometry;

    screen_geometry = screen_get_area(screen,
                                      globalconf.screens[screen].statusbar,
                                      &globalconf.screens[screen].padding);

    /* fix offscreen */
    if(AREA_RIGHT(geometry) > AREA_RIGHT(screen_geometry))
        geometry.x = screen_geometry.x + screen_geometry.width - (geometry.width + 2 * border);
    else if(AREA_LEFT(geometry) < AREA_LEFT(screen_geometry))
        geometry.x = screen_geometry.x;

    if(AREA_BOTTOM(geometry) > AREA_BOTTOM(screen_geometry))
        geometry.y = screen_geometry.y + screen_geometry.height - (geometry.height + 2 * border);
    else if(AREA_TOP(geometry) < AREA_TOP(screen_geometry))
        geometry.y = screen_geometry.y;

    return geometry;
}

/** Compute smart coordinates for a client window
 * \param geometry current/requested client geometry
 * \param screen screen used
 * \return new geometry
 */
area_t
placement_smart(Client *c)
{
    Client *client;
    area_t newgeometry = { 0, 0, 0, 0, NULL, NULL };
    area_t *screen_geometry, *arealist = NULL, *r;
    Bool found = False;

    screen_geometry = p_new(area_t, 1);

    *screen_geometry = screen_get_area(c->screen,
                                       globalconf.screens[c->screen].statusbar,
                                       &globalconf.screens[c->screen].padding);

    area_list_push(&arealist, screen_geometry);

    for(client = globalconf.clients; client; client = client->next)
        if(client_isvisible(client, c->screen))
        {
            newgeometry = client->f_geometry;
            newgeometry.width += 2 * client->border;
            newgeometry.height += 2 * client->border;
            newgeometry = titlebar_geometry_add(&client->titlebar, newgeometry);
            area_list_remove(&arealist, &newgeometry);
        }

    newgeometry.x = c->f_geometry.x;
    newgeometry.y = c->f_geometry.y;
    newgeometry.width = 0;
    newgeometry.height = 0;

    for(r = arealist; r; r = r->next)
        if(r->width >= c->f_geometry.width && r->height >= c->f_geometry.height
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
    newgeometry.width = c->f_geometry.width;
    newgeometry.height = c->f_geometry.height;

    newgeometry = titlebar_geometry_add(&c->titlebar, newgeometry);
    newgeometry = placement_fix_offscreen(newgeometry, c->screen, c->border);
    newgeometry = titlebar_geometry_remove(&c->titlebar, newgeometry);

    area_list_wipe(&arealist);

    return newgeometry;
}

area_t
placement_under_mouse(Client *c)
{
    Window dummy;
    unsigned int m;
    int x, y, d;
    area_t finalgeometry = c->f_geometry;

    if(XQueryPointer(globalconf.display, RootWindow(globalconf.display, c->phys_screen),
                     &dummy, &dummy, &x, &y, &d, &d, &m))
    {
        finalgeometry.x = x - c->f_geometry.width / 2;
        finalgeometry.y = y - c->f_geometry.height / 2;
    }

    finalgeometry = titlebar_geometry_add(&c->titlebar, finalgeometry);
    finalgeometry = placement_fix_offscreen(finalgeometry, c->screen, c->border);
    finalgeometry = titlebar_geometry_remove(&c->titlebar, finalgeometry);

    return finalgeometry;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
