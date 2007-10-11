/*
 * tile.c - tile layout
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

#include <stdio.h>

#include "util.h"
#include "screen.h"
#include "awesome.h"
#include "tag.h"
#include "layout.h"
#include "layouts/tile.h"

void
uicb_setnmaster(awesome_config *awesomeconf,
                const char * arg)
{
    if(!arg || (!IS_ARRANGE(0, layout_tile) && !IS_ARRANGE(0, layout_tileleft)))
        return;


    if((awesomeconf->nmaster = (int) compute_new_value_from_arg(arg, (double) awesomeconf->nmaster)) < 0)
        awesomeconf->nmaster = 0;

    arrange(awesomeconf->display, awesomeconf);
}

void
uicb_setncol(awesome_config *awesomeconf,
             const char * arg)
{
    if(!arg || (!IS_ARRANGE(0, layout_tile) && !IS_ARRANGE(0, layout_tileleft)))
        return;

    if((awesomeconf->ncol = (int) compute_new_value_from_arg(arg, (double) awesomeconf->ncol)) < 1)
        awesomeconf->ncol = 1;

    arrange(awesomeconf->display, awesomeconf);
}

void
uicb_setmwfact(awesome_config * awesomeconf,
               const char *arg)
{
    char *newarg;

    if((!IS_ARRANGE(0, layout_tile) && !IS_ARRANGE(0, layout_tileleft)) || !arg)
        return;

    newarg = a_strdup(arg);
    if(IS_ARRANGE(0, layout_tileleft))
    {
        if(newarg[0] == '+')
            newarg[0] = '-';
        else if(arg[0] == '-')
            newarg[0] = '+';
    }

    if((awesomeconf->mwfact = compute_new_value_from_arg(newarg, awesomeconf->mwfact)) < 0.1)
        awesomeconf->mwfact = 0.1;
    else if(awesomeconf->mwfact > 0.9)
        awesomeconf->mwfact = 0.9;

    arrange(awesomeconf->display, awesomeconf);
    p_delete(&newarg);
}

static void
_tile(Display *disp, awesome_config *awesomeconf, const Bool right)
{
    /* windows area geometry */
    int wah = 0, waw = 0, wax = 0, way = 0;
    /* new coordinates */
    unsigned int nx, ny, nw, nh;
    /* master size */
    unsigned int mw = 0, mh = 0;
    int n, i, masterwin = 0, otherwin = 0;
    int real_ncol = 1, win_by_col = 1, current_col = 0;
    ScreenInfo *screens_info = NULL;
    Client *c;

    screens_info = get_screen_info(disp, awesomeconf->screen, &awesomeconf->statusbar);

    for(n = 0, c = *awesomeconf->clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags))
            n++;

    wah = screens_info[awesomeconf->screen].height;
    waw = screens_info[awesomeconf->screen].width;
    wax = screens_info[awesomeconf->screen].x_org;
    way = screens_info[awesomeconf->screen].y_org;

    masterwin = MIN(n, awesomeconf->nmaster);

    otherwin = n - masterwin;

    if(otherwin < 0)
        otherwin = 0;

    if(awesomeconf->nmaster)
    {
        mh = masterwin ? wah / masterwin : waw;
        mw = otherwin ? waw * awesomeconf->mwfact : waw;
    }
    else
        mh = mw = 0;

    real_ncol = MIN(otherwin, awesomeconf->ncol);

    for(i = 0, c = *awesomeconf->clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags))
            continue;

        c->ismax = False;
        if(i < awesomeconf->nmaster)
        {                       /* master */
            ny = way + i * mh;
            nx = wax + (right ? 0 : waw - mw);
            resize(c, nx, ny, mw - 2 * c->border, mh - 2 * c->border, awesomeconf, awesomeconf->resize_hints);
        }
        else
        {                       /* tile window */
            if(real_ncol)
                win_by_col = otherwin / real_ncol;

            if((i - awesomeconf->nmaster) && (i - awesomeconf->nmaster) % win_by_col == 0 && current_col < real_ncol - 1)
                current_col++;

            if(current_col == real_ncol - 1)
                win_by_col += otherwin % real_ncol;

            if(otherwin <= real_ncol)
                nh = wah - 2 * c->border;
            else
                nh = (wah / win_by_col) - 2 * c->border;

            nw = (waw - mw) / real_ncol - 2 * c->border;

            if(i == awesomeconf->nmaster || otherwin <= real_ncol || (i - awesomeconf->nmaster) % win_by_col == 0)
                ny = way;
            else
                ny = way + ((i - awesomeconf->nmaster) % win_by_col) * (nh + 2 * c->border);

            nx = wax + current_col * (nw + 2 * c->border) + (right ? mw : 0);
            resize(c, nx, ny, nw, nh, awesomeconf, awesomeconf->resize_hints);
        }
        i++;
    }
    XFree(screens_info);
}

void
layout_tile(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, True);
}

void
layout_tileleft(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, False);
}
