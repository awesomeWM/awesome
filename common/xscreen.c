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

#include <xcb/xcb.h>
#include <xcb/xinerama.h>

#include "common/xscreen.h"
#include "common/xutil.h"

static inline area_t
screen_xsitoarea(xcb_xinerama_screen_info_t si)
{
    area_t a;

    a.x = si.x_org;
    a.y = si.y_org;
    a.width = si.width;
    a.height = si.height;

    return a;
}

void
screensinfo_delete(screens_info_t **si)
{
    p_delete(&(*si)->geometry);
    p_delete(si);
}

/** Get screens informations.
 * \param conn X connection.
 * \return A pointer to complete screens_info_t structure.
 */
screens_info_t *
screensinfo_new(xcb_connection_t *conn)
{
    screens_info_t *si;
    xcb_xinerama_query_screens_reply_t *xsq;
    xcb_xinerama_screen_info_t *xsi;
    int xinerama_screen_number, screen, screen_to_test;
    xcb_screen_t *s;
    bool drop;
    xcb_xinerama_is_active_reply_t *xia = NULL;

    si = p_new(screens_info_t, 1);

    /* Check for extension before checking for Xinerama */
    if(xcb_get_extension_data(conn, &xcb_xinerama_id)->present)
    {
        xia = xcb_xinerama_is_active_reply(conn, xcb_xinerama_is_active(conn), NULL);
        si->xinerama_is_active = xia->state;
        p_delete(&xia);
    }

    if(si->xinerama_is_active)
    {
        xsq = xcb_xinerama_query_screens_reply(conn,
                                               xcb_xinerama_query_screens_unchecked(conn),
                                               NULL);

        xsi = xcb_xinerama_query_screens_screen_info(xsq);
        xinerama_screen_number = xcb_xinerama_query_screens_screen_info_length(xsq);

        si->geometry = p_new(area_t, xinerama_screen_number);
        si->nscreen = 0;

        /* now check if screens overlaps (same x,y): if so, we take only the biggest one */
        for(screen = 0; screen < xinerama_screen_number; screen++)
        {
            drop = false;
            for(screen_to_test = 0; screen_to_test < si->nscreen; screen_to_test++)
                if(xsi[screen].x_org == si->geometry[screen_to_test].x
                   && xsi[screen].y_org == si->geometry[screen_to_test].y)
                    {
                        /* we already have a screen for this area, just check if
                         * it's not bigger and drop it */
                        drop = true;
                        si->geometry[screen_to_test].width =
                            MAX(xsi[screen].width, xsi[screen_to_test].width);
                        si->geometry[screen_to_test].height =
                            MAX(xsi[screen].height, xsi[screen_to_test].height);
                    }
            if(!drop)
                si->geometry[si->nscreen++] = screen_xsitoarea(xsi[screen]);
        }

        /* realloc smaller if xinerama_screen_number != screen registered */
        if(xinerama_screen_number != si->nscreen)
        {
            area_t *newgeometry = p_new(area_t, si->nscreen);
            memcpy(newgeometry, si->geometry, si->nscreen * sizeof(area_t));
            p_delete(&si->geometry);
            si->geometry = newgeometry;
        }

        p_delete(&xsq);
    }
    else
    {
        si->nscreen = xcb_setup_roots_length(xcb_get_setup(conn));
        si->geometry = p_new(area_t, si->nscreen);
        for(screen = 0; screen < si->nscreen; screen++)
        {
            s = xutil_screen_get(conn, screen);
            si->geometry[screen].x = 0;
            si->geometry[screen].y = 0;
            si->geometry[screen].width = s->width_in_pixels;
            si->geometry[screen].height = s->height_in_pixels;
        }
    }

    return si;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
