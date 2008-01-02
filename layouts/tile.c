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

#include "util.h"
#include "screen.h"
#include "awesome.h"
#include "tag.h"
#include "layout.h"
#include "client.h"
#include "layouts/tile.h"

extern AwesomeConf globalconf;

void
uicb_tag_setnmaster(int screen, char * arg)
{
    Tag **curtags = get_current_tags(screen);
    Layout *curlay = curtags[0]->layout;

    if(!arg || (curlay->arrange != layout_tile && curlay->arrange != layout_tileleft))
        return;

    if((curtags[0]->nmaster = (int) compute_new_value_from_arg(arg, (double) curtags[0]->nmaster)) < 0)
        curtags[0]->nmaster = 0;

    p_delete(&curtags);

    arrange(screen);
}

void
uicb_tag_setncol(int screen, char * arg)
{
    Tag **curtags = get_current_tags(screen);
    Layout *curlay = curtags[0]->layout;

    if(!arg || (curlay->arrange != layout_tile && curlay->arrange != layout_tileleft))
        return;

    if((curtags[0]->ncol = (int) compute_new_value_from_arg(arg, (double) curtags[0]->ncol)) < 1)
        curtags[0]->ncol = 1;

    p_delete(&curtags);

    arrange(screen);
}

void
uicb_tag_setmwfact(int screen, char *arg)
{
    char *newarg;
    Tag **curtags = get_current_tags(screen);
    Layout *curlay = curtags[0]->layout;

    if(!arg || (curlay->arrange != layout_tile && curlay->arrange != layout_tileleft))
        return;

    newarg = a_strdup(arg);
    if(curlay->arrange == layout_tileleft)
    {
        if(newarg[0] == '+')
            newarg[0] = '-';
        else if(arg[0] == '-')
            newarg[0] = '+';
    }

    if((curtags[0]->mwfact = compute_new_value_from_arg(newarg, curtags[0]->mwfact)) < 0.1)
        curtags[0]->mwfact = 0.1;
    else if(curtags[0]->mwfact > 0.9)
        curtags[0]->mwfact = 0.9;

    p_delete(&newarg);
    p_delete(&curtags);
    arrange(screen);
}

static void
_tile(int screen, const Bool right)
{
    /* windows area geometry */
    int wah = 0, waw = 0, wax = 0, way = 0;
    /* new coordinates */
    unsigned int nx, ny, nw, nh;
    /* master size */
    unsigned int mw = 0, mh = 0;
    int n, i, masterwin = 0, otherwin = 0;
    int real_ncol = 1, win_by_col = 1, current_col = 0;
    Area area;
    Client *c;
    Tag **curtags = get_current_tags(screen);

    area = get_screen_area(screen,
                           globalconf.screens[screen].statusbar,
                            &globalconf.screens[screen].padding);

    for(n = 0, c = globalconf.clients; c; c = c->next)
        if(IS_TILED(c, screen))
            n++;

    wah = area.height;
    waw = area.width;
    wax = area.x;
    way = area.y;

    masterwin = MIN(n, curtags[0]->nmaster);

    otherwin = n - masterwin;

    if(otherwin < 0)
        otherwin = 0;

    if(curtags[0]->nmaster)
    {
        mh = masterwin ? wah / masterwin : waw;
        mw = otherwin ? waw * curtags[0]->mwfact : waw;
    }
    else
        mh = mw = 0;

    real_ncol = curtags[0]->ncol > 0 ? MIN(otherwin, curtags[0]->ncol) : MIN(otherwin, 1);

    for(i = 0, c = globalconf.clients; c; c = c->next)
    {
        if(!IS_TILED(c, screen))
            continue;

        c->ismax = False;
        if(i < curtags[0]->nmaster)
        {                       /* master */
            ny = way + i * mh;
            nx = wax + (right ? 0 : waw - mw);
            client_resize(c, nx, ny, mw - 2 * c->border, mh - 2 * c->border, globalconf.screens[screen].resize_hints, False);
        }
        else
        {                       /* tile window */
            if(real_ncol)
                win_by_col = otherwin / real_ncol;

            if((i - curtags[0]->nmaster) && (i - curtags[0]->nmaster) % win_by_col == 0 && current_col < real_ncol - 1)
                current_col++;

            if(current_col == real_ncol - 1)
                win_by_col += otherwin % real_ncol;

            if(otherwin <= real_ncol)
                nh = wah - 2 * c->border;
            else
                nh = (wah / win_by_col) - 2 * c->border;

            nw = (waw - mw) / real_ncol - 2 * c->border;

            if(i == curtags[0]->nmaster || otherwin <= real_ncol || (i - curtags[0]->nmaster) % win_by_col == 0)
                ny = way;
            else
                ny = way + ((i - curtags[0]->nmaster) % win_by_col) * (nh + 2 * c->border);

            nx = wax + current_col * (nw + 2 * c->border) + (right ? mw : 0);
            client_resize(c, nx, ny, nw, nh, globalconf.screens[screen].resize_hints, False);
        }
        i++;
    }

    p_delete(&curtags);
}

void
layout_tile(int screen)
{
    _tile(screen, True);
}

void
layout_tileleft(int screen)
{
    _tile(screen, False);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
