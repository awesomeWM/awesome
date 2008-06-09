/*
 * tile.c - tile layout
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

#include "screen.h"
#include "workspace.h"
#include "client.h"
#include "layouts/tile.h"
#include "common/util.h"

extern awesome_t globalconf;

static void
_tile(workspace_t *ws, const position_t position)
{
    /* windows area geometry */
    int wah = 0, waw = 0, wax = 0, way = 0;
    /* master size */
    unsigned int mw = 0, mh = 0;
    int n, i, masterwin = 0, otherwin = 0;
    int screen, real_ncol = 1, win_by_col = 1, current_col = 0;
    area_t area, geometry = { 0, 0, 0, 0, NULL, NULL };
    client_t *c;

    screen = workspace_screen_get(ws);

    area = screen_area_get(screen,
                           globalconf.screens[screen].statusbar,
                           &globalconf.screens[screen].padding);

    for(n = 0, c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
            n++;

    wah = area.height;
    waw = area.width;
    wax = area.x;
    way = area.y;

    masterwin = MIN(n, ws->nmaster);

    otherwin = MAX(n - masterwin, 0);

    if(ws->nmaster)
        switch(position)
        {
          case Right:
          case Left:
            mh = masterwin ? wah / masterwin : wah;
            mw = otherwin ? waw * ws->mwfact : waw;
            break;
          default:
            mh = otherwin ? wah * ws->mwfact : wah;
            mw = masterwin ? waw / masterwin : waw;
            break;
        }
    else
        mh = mw = 0;

    real_ncol = ws->ncol > 0 ? MIN(otherwin, ws->ncol) : MIN(otherwin, 1);

    for(i = 0, c = globalconf.clients; c; c = c->next)
    {
        if(!IS_TILED(c, screen))
            continue;

        if(i < ws->nmaster)
        {
            switch(position)
            {
              case Right:
                geometry.y = way + i * mh;
                geometry.x = wax;
                break;
              case Left:
                geometry.y = way + i * mh;
                geometry.x = wax + (waw - mw);
                break;
              case Top:
                geometry.x = wax + i * mw;
                geometry.y = way + (wah - mh);
                break;
              case Bottom:
              default:
                geometry.x = wax + i * mw;
                geometry.y = way;
                break;
                break;
            }

            geometry.width = mw - 2 * c->border;
            geometry.height =  mh - 2 * c->border;

            client_resize(c, geometry, globalconf.resize_hints);
        }
        else
        {
            if(real_ncol)
                win_by_col = otherwin / real_ncol;

            if((i - ws->nmaster)
               && (i - ws->nmaster) % win_by_col == 0
               && current_col < real_ncol - 1)
                current_col++;

            if(current_col == real_ncol - 1)
                win_by_col += otherwin % real_ncol;

            if(position == Right || position == Left)
            {
                if(otherwin <= real_ncol)
                    geometry.height = wah - 2 * c->border;
                else
                    geometry.height = (wah / win_by_col) - 2 * c->border;

                geometry.width = (waw - mw) / real_ncol - 2 * c->border;

                if(i == ws->nmaster || otherwin <= real_ncol || (i - ws->nmaster) % win_by_col == 0)
                    geometry.y = way;
                else
                    geometry.y = way + ((i - ws->nmaster) % win_by_col) * (geometry.height + 2 * c->border);

                geometry.x = wax + current_col * (geometry.width + 2 * c->border);

                if(position == Right)
                    geometry.x += mw;
            }
            else
            {
                if(otherwin <= real_ncol)
                    geometry.width = waw - 2 * c->border;
                else
                    geometry.width = (waw / win_by_col) - 2 * c->border;

                geometry.height = (wah - mh) / real_ncol - 2 * c->border;

                if(i == ws->nmaster || otherwin <= real_ncol || (i - ws->nmaster) % win_by_col == 0)
                    geometry.x = wax;
                else
                    geometry.x = wax + ((i - ws->nmaster) % win_by_col) * (geometry.width + 2 * c->border);

                geometry.y = way + current_col * (geometry.height + 2 * c->border);

                if(position == Bottom)
                    geometry.y += mh;
            }
            client_resize(c, geometry, globalconf.resize_hints);
        }
        i++;
    }
}

void
layout_tile(workspace_t *ws)
{
    _tile(ws, Right);
}

void
layout_tileleft(workspace_t *ws)
{
    _tile(ws, Left);
}

void
layout_tilebottom(workspace_t *ws)
{
    _tile(ws, Bottom);
}

void
layout_tiletop(workspace_t *ws)
{
    _tile(ws, Top);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
