/*
 * screen.c - screen management
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

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "screen.h"
#include "focus.h"
#include "client.h"
#include "workspace.h"
#include "layouts/floating.h"

extern awesome_t globalconf;

/** Get screens info.
 * \param screen Screen number.
 * \param statusbar Statusbar list to remove.
 * \param padding Padding.
 * \return The screen area.
 */
area_t
screen_area_get(int screen, statusbar_t *statusbar, padding_t *padding)
{
    area_t area = globalconf.screens_info->geometry[screen];
    statusbar_t *sb;

    /* make padding corrections */
    if(padding)
    {
        area.x += padding->left;
        area.y += padding->top;
        area.width -= padding->left + padding->right;
        area.height -= padding->top + padding->bottom;
    }

    for(sb = statusbar; sb; sb = sb->next)
        switch(sb->position)
        {
          case Top:
            area.y += sb->height;
          case Bottom:
            area.height -= sb->height;
            break;
          case Left:
            area.x += sb->height;
          case Right:
            area.width -= sb->height;
            break;
          default:
            break;
        }

    return area;
}

/** Get display info.
 * \param phys_screen Physical screen number.
 * \param statusbar The statusbars.
 * \param padding Padding.
 * \return The display area.
 */
area_t
display_area_get(int phys_screen, statusbar_t *statusbar, padding_t *padding)
{
    area_t area = { 0, 0, 0, 0, NULL, NULL };
    statusbar_t *sb;
    xcb_screen_t *s = xcb_aux_get_screen(globalconf.connection, phys_screen);

    area.width = s->width_in_pixels;
    area.height = s->height_in_pixels;

    for(sb = statusbar; sb; sb = sb->next)
    {
        area.y += sb->position == Top ? sb->height : 0;
        area.height -= (sb->position == Top || sb->position == Bottom) ? sb->height : 0;
    }

    /* make padding corrections */
    if(padding)
    {
            area.x += padding->left;
            area.y += padding->top;
            area.width -= padding->left + padding->right;
            area.height -= padding->top + padding->bottom;
    }
    return area;
}

/** This returns the real X screen number for a logical
 * screen if Xinerama is active.
 * \param screen The logical screen.
 * \return The X screen.
 */
int
screen_virttophys(int screen)
{
    if(globalconf.screens_info->xinerama_is_active)
        return globalconf.default_screen;
    return screen;
}

/** Move a client to a virtual screen.
 * \param c The client to move.
 * \param new_screen The new screen.
 */
void
screen_client_moveto(client_t *c, int new_screen)
{
    area_t from, to, new_geometry, new_f_geometry;
    int old_screen;

    old_screen = screen_get_bycoord(globalconf.screens_info, c->phys_screen,
                                    c->geometry.x, c->geometry.y);

    /* nothing to do */
    if(old_screen == new_screen)
        return;

    new_f_geometry = c->f_geometry;

    from = screen_area_get(old_screen, NULL, NULL);
    to = screen_area_get(new_screen, NULL, NULL);

    /* compute new coords in new screen */
    new_f_geometry.x = (c->f_geometry.x - from.x) + to.x;
    new_f_geometry.y = (c->f_geometry.y - from.y) + to.y;

    /* check that new coords are still in the screen */
    if(new_f_geometry.width > to.width)
        new_f_geometry.width = to.width;
    if(new_f_geometry.height > to.height)
        new_f_geometry.height = to.height;
    if(new_f_geometry.x + new_f_geometry.width >= to.x + to.width)
        new_f_geometry.x = to.x + to.width - new_f_geometry.width - 2 * c->border;
    if(new_f_geometry.y + new_f_geometry.height >= to.y + to.height)
        new_f_geometry.y = to.y + to.height - new_f_geometry.height - 2 * c->border;

    if(c->ismax)
    {
        new_geometry = c->geometry;

        /* compute new coords in new screen */
        new_geometry.x = (c->geometry.x - from.x) + to.x;
        new_geometry.y = (c->geometry.y - from.y) + to.y;

        /* check that new coords are still in the screen */
        if(new_geometry.width > to.width)
            new_geometry.width = to.width;
        if(new_geometry.height > to.height)
            new_geometry.height = to.height;
        if(new_geometry.x + new_geometry.width >= to.x + to.width)
            new_geometry.x = to.x + to.width - new_geometry.width - 2 * c->border;
        if(new_geometry.y + new_geometry.height >= to.y + to.height)
            new_geometry.y = to.y + to.height - new_geometry.height - 2 * c->border;

        /* compute new coords for max in new screen */
        c->m_geometry.x = (c->m_geometry.x - from.x) + to.x;
        c->m_geometry.y = (c->m_geometry.y - from.y) + to.y;

        /* check that new coords are still in the screen */
        if(c->m_geometry.width > to.width)
            c->m_geometry.width = to.width;
        if(c->m_geometry.height > to.height)
            c->m_geometry.height = to.height;
        if(c->m_geometry.x + c->m_geometry.width >= to.x + to.width)
            c->m_geometry.x = to.x + to.width - c->m_geometry.width - 2 * c->border;
        if(c->m_geometry.y + c->m_geometry.height >= to.y + to.height)
            c->m_geometry.y = to.y + to.height - c->m_geometry.height - 2 * c->border;

        client_resize(c, new_geometry, false);
    }
    /* if floating, move to this new coords */
    else if(c->isfloating)
        client_resize(c, new_f_geometry, false);
    /* otherwise just register them */
    else
        c->f_geometry = new_f_geometry;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
