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

/* extern */
extern Client *sel, *clients;

void
uicb_setnmaster(Display *disp,
                DC * drawcontext,
                awesome_config *awesomeconf,
                const char * arg)
{
    if(!arg || (!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft)))
        return;

    if((awesomeconf->nmaster = (int) compute_new_value_from_arg(arg, (double) awesomeconf->nmaster)) < 0)
        awesomeconf->nmaster = 0;

    arrange(disp, drawcontext, awesomeconf);
}

void
uicb_setncols(Display *disp,
              DC * drawcontext,
              awesome_config *awesomeconf,
              const char * arg)
{
    if(!arg || (!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft)))
        return;

    if((awesomeconf->ncols = (int) compute_new_value_from_arg(arg, (double) awesomeconf->ncols)) < 1)
        awesomeconf->ncols = 1;

    arrange(disp, drawcontext, awesomeconf);
}

void
uicb_setmwfact(Display *disp,
               DC *drawcontext,
               awesome_config * awesomeconf,
               const char *arg)
{
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft))
        return;

    if((awesomeconf->mwfact = compute_new_value_from_arg(arg, awesomeconf->mwfact)) < 0.1)
        awesomeconf->mwfact = 0.1;
    else if(awesomeconf->mwfact > 0.9)
        awesomeconf->mwfact = 0.9;

    arrange(disp, drawcontext, awesomeconf);
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
    int n, i, li, last_i = 0, masterwin = 0, otherwin = 0;
    int screen_numbers = 1, use_screen = -1;
    int real_ncols = 1, win_by_col = 1, current_col = 0;
    ScreenInfo *screens_info = NULL;
    Client *c;

    screens_info = get_screen_info(disp, awesomeconf->screen, awesomeconf->statusbar, &screen_numbers);
 
    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;

        if(use_screen == -1
           || (screen_numbers > 1 
               && i
               && ((i - last_i) >= masterwin + otherwin
                   || n == screen_numbers)))
        {
            use_screen++;
            last_i = i;

            wah = screens_info[use_screen].height;
            waw = screens_info[use_screen].width;
            wax = screens_info[use_screen].x_org;
            way = screens_info[use_screen].y_org;

            if(n >= awesomeconf->nmaster * screen_numbers)
            {
                masterwin = awesomeconf->nmaster;
                otherwin = (n - (awesomeconf->nmaster * screen_numbers)) / screen_numbers;
                if(use_screen == 0)
                    otherwin += (n - (awesomeconf->nmaster * screen_numbers)) % screen_numbers;
            }
            else
            {
                masterwin = n / screen_numbers;
                /* first screen takes more master */
                if(use_screen == 0)
                    masterwin += n % screen_numbers;
                otherwin = 0;
            }

            if(awesomeconf->nmaster)
            {
                mh = masterwin ? wah / masterwin : waw;
                mw = otherwin ? waw * awesomeconf->mwfact : waw;
            }
            else
                mh = mw = 0;

            mw -= 2 * c->border;
            mh -= 2 * c->border;

            if(otherwin < awesomeconf->ncols)
                real_ncols = otherwin;
            else
                real_ncols = awesomeconf->ncols;

            current_col = 0;
        }

        c->ismax = False;
        li = last_i ? i - last_i : i;
        if(li < awesomeconf->nmaster)
        {                       /* master */
            ny = way + li * (mh + 2 * c->border);
            nx = wax + (right ? 0 : waw - (mw + 2 * c->border));
            resize(c, nx, ny, mw, mh, awesomeconf->resize_hints);
        }
        else
        {                       /* tile window */
            win_by_col = otherwin / real_ncols;

            if((li - awesomeconf->nmaster) && (li - awesomeconf->nmaster) % win_by_col == 0 && current_col < real_ncols - 1)
                current_col++;

            if(current_col == real_ncols - 1)
                win_by_col += otherwin % real_ncols;

            if(otherwin <= real_ncols)
                nh = wah - 2 * c->border;
            else
                nh = (wah / win_by_col) - 2 * c->border;

            nw = (waw - (mw + 2 * c->border)) / real_ncols - 2 * c->border;

            if(li == awesomeconf->nmaster || otherwin <= real_ncols || (li - awesomeconf->nmaster) % win_by_col == 0)
                ny = way;
            else
                ny = way + ((li - awesomeconf->nmaster) % win_by_col) * (nh + 2 * c->border);

            nx = wax + current_col * nw + (right ? mw + 2 * c->border : 0);
            resize(c, nx, ny, nw, nh, awesomeconf->resize_hints);
        }
        i++;
    }
    XFree(screens_info);
}

void
tile(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, True);
}

void
tileleft(Display *disp, awesome_config *awesomeconf)
{
    _tile(disp, awesomeconf, False);
}
