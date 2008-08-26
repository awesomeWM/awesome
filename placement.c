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
#include "layouts/floating.h"

extern awesome_t globalconf;

const name_func_link_t FloatingPlacementList[] =
{
    { "smart", placement_smart },
    { "under_mouse", placement_under_mouse },
    { NULL, NULL }
};

static area_t
placement_fix_offscreen(area_t geometry, int screen)
{
    area_t screen_geometry;

    screen_geometry = screen_area_get(&globalconf.screens[screen].geometry,
                                      globalconf.screens[screen].statusbar,
                                      &globalconf.screens[screen].padding);

    /* fix offscreen */
    if(AREA_RIGHT(geometry) > AREA_RIGHT(screen_geometry))
        geometry.x = screen_geometry.x + screen_geometry.width - geometry.width;
    else if(AREA_LEFT(geometry) < AREA_LEFT(screen_geometry))
        geometry.x = screen_geometry.x;

    if(AREA_BOTTOM(geometry) > AREA_BOTTOM(screen_geometry))
        geometry.y = screen_geometry.y + screen_geometry.height - geometry.height;
    else if(AREA_TOP(geometry) < AREA_TOP(screen_geometry))
        geometry.y = screen_geometry.y;

    return geometry;
}

/** Compute smart coordinates for a client window
 * \param c The client to place.
 * \return new geometry
 */
area_t
placement_smart(client_t *c)
{
    client_t *client;
    area_t newgeometry = { 0, 0, 0, 0 };
    area_t screen_geometry;
    area_array_t areas;
    bool found = false;
    layout_t *layout;
    int i;

    area_array_init(&areas);
    screen_geometry = screen_area_get(&globalconf.screens[c->screen].geometry,
                                      globalconf.screens[c->screen].statusbar,
                                      &globalconf.screens[c->screen].padding);

    layout = layout_get_current(c->screen);
    area_array_append(&areas, screen_geometry);

    for(client = globalconf.clients; client; client = client->next)
        if(client != c && (client->isfloating || layout == layout_floating)
           && client_isvisible(client, c->screen))
        {
            newgeometry = client->f_geometry;
            newgeometry = titlebar_geometry_add(c->titlebar, c->border, newgeometry);
            area_array_remove(&areas, newgeometry);
        }

    newgeometry.x = c->f_geometry.x;
    newgeometry.y = c->f_geometry.y;
    newgeometry.width = 0;
    newgeometry.height = 0;

    for(i = areas.len - 1; i >= 0; i--)
    {
        area_t *r = &areas.tab[i];

        if(r->width >= c->f_geometry.width && r->height >= c->f_geometry.height
           && r->width * r->height > newgeometry.width * newgeometry.height)
        {
            found = true;
            newgeometry = *r;
        }
    }

    /* we did not found a space with enough space for our size:
     * just take the biggest available and go in */
    if(!found)
        for(i = 0; i < areas.len; i++)
        {
            area_t *r = &areas.tab[i];
            if(r->width * r->height > newgeometry.width * newgeometry.height)
                newgeometry = *r;
        }

    /* restore height and width */
    newgeometry.width = c->f_geometry.width;
    newgeometry.height = c->f_geometry.height;

    newgeometry = titlebar_geometry_add(c->titlebar, c->border, newgeometry);
    newgeometry = placement_fix_offscreen(newgeometry, c->screen);
    newgeometry = titlebar_geometry_remove(c->titlebar, c->border, newgeometry);

    area_array_wipe(&areas);
    return newgeometry;
}

/** Compute placement for a window centered under the mouse.
 * \param c The client.
 * \return The proposed geometry.
 */
area_t
placement_under_mouse(client_t *c)
{
    xcb_query_pointer_cookie_t qp_c;
    xcb_query_pointer_reply_t *qp_r;
    area_t finalgeometry = c->f_geometry;

    qp_c = xcb_query_pointer_unchecked(globalconf.connection,
                                       xutil_screen_get(globalconf.connection,
                                                        c->phys_screen)->root);
    if((qp_r = xcb_query_pointer_reply(globalconf.connection, qp_c, NULL)))
    {
        finalgeometry.x = qp_r->root_x - c->f_geometry.width / 2;
        finalgeometry.y = qp_r->root_y - c->f_geometry.height / 2;

        p_delete(&qp_r);
    }

    finalgeometry = titlebar_geometry_add(c->titlebar, c->border, finalgeometry);
    finalgeometry = placement_fix_offscreen(finalgeometry, c->screen);
    finalgeometry = titlebar_geometry_remove(c->titlebar, c->border, finalgeometry);

    return finalgeometry;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
