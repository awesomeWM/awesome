/* 
 * tile.c - tile layout
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info> 
 * Copyright © 2007 Ross Mohn <rpmohn@waxandwane.org>
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

#include "awesome.h"
#include "tag.h"
#include "layout.h"
#include "layouts/tile.h"

/* extern */
extern Client *sel, *clients;

/* static */

static double mwfact = 0.6;
static int nmaster = 2;

void
uicb_setnmaster(Display *disp,
                DC * drawcontext,
                awesome_config *awesomeconf,
                const char * arg)
{
    int delta;
    int wah = get_windows_area_height(disp, awesomeconf->statusbar);

    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;
    if(!arg)
        nmaster = awesomeconf->nmaster;
    else if(sscanf(arg, "%d", &delta))
    {
        if((arg[0] == '+' || arg[0] == '-')
           && !((nmaster + delta) < 1 || wah / (nmaster + delta) <= 2 * awesomeconf->borderpx))
            nmaster += delta;
        else if(delta >= 1 && wah / delta <= 2 * awesomeconf->borderpx)
            nmaster = delta;
        else
            return;
    }
    if(sel)
        arrange(disp, drawcontext, awesomeconf);
    else
        drawstatus(disp, drawcontext, awesomeconf);
}

void
uicb_setmwfact(Display *disp,
                DC *drawcontext,
               awesome_config * awesomeconf,
               const char *arg)
{
    double delta;

    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;

    /* arg handling, manipulate mwfact */
    if(!arg)
        mwfact = awesomeconf->mwfact;
    else if(sscanf(arg, "%lf", &delta))
    {
        if(arg[0] != '+' && arg[0] != '-')
            mwfact = delta;
        else
            mwfact += delta;
        if(mwfact < 0.1)
            mwfact = 0.1;
        else if(mwfact > 0.9)
            mwfact = 0.9;
    }
    arrange(disp, drawcontext, awesomeconf);
}

static void
_tile(Display *disp, awesome_config *awesomeconf, const Bool right)
{
    int wah = get_windows_area_height(disp, awesomeconf->statusbar);
    int waw = get_windows_area_width(disp, awesomeconf->statusbar);
    int wax = get_windows_area_x(awesomeconf->statusbar);
    int way = get_windows_area_y(awesomeconf->statusbar);
    unsigned int nx, ny, nw, nh, mw;
    int n, th, i, mh;
    Client *c;

    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    /* window geoms */
    mh = (n <= nmaster) ? wah / (n > 0 ? n : 1) : wah / nmaster;
    mw = (n <= nmaster) ? waw : mwfact * waw;
    th = (n > nmaster) ? wah / (n - nmaster) : 0;
    if(n > nmaster && th < awesomeconf->statusbar.height)
        th = wah;

    nx = wax;
    ny = way;
    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;
        
        c->ismax = False;
        if(i < nmaster)
        {                       /* master */
            ny = way + i * mh;
            if(!right && i == 0)
                nx += (waw - mw);
            nw = mw - 2 * c->border;
            nh = mh;
            if(i + 1 == (n < nmaster ? n : nmaster)) /* remainder */
                nh = wah - mh * i;
            nh -= 2 * c->border;
        }
        else
        {                       /* tile window */
            if(i == nmaster)
            {
                ny = way;
                if(right)
                    nx += mw;
                else
                    nx = 0;
            }
            nw = waw - mw - 2 * c->border;
            if(i + 1 == n)      /* remainder */
                nh = (way + wah) - ny - 2 * c->border;
            else
                nh = th - 2 * c->border;
        }
        resize(c, nx, ny, nw, nh, awesomeconf->resize_hints);
        if(n > nmaster && th != wah)
            ny += nh + 2 * c->border;
        i++;
    }
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


static void
_bstack(Display *disp, awesome_config *awesomeconf, Bool portrait)
{
    int i, n, nx, ny, nw, nh, mw, mh, tw, th;
    int wah = get_windows_area_height(disp, awesomeconf->statusbar);
    int waw = get_windows_area_width(disp, awesomeconf->statusbar);
    Client *c;

    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    /* window geoms */
    mh = (n > nmaster) ? (wah * mwfact) / nmaster : wah / (n > 0 ? n : 1);
    mw = waw;
    th = (n > nmaster) ? (wah * (1 - mwfact)) / (portrait ? 1 : n - nmaster) : 0;
    tw = (n > nmaster) ? waw / (portrait ? n - nmaster : 1) : 0;

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;

        c->ismax = False;
        nx = get_windows_area_x(awesomeconf->statusbar);
        ny = get_windows_area_y(awesomeconf->statusbar);
        if(i < nmaster)
        {
            ny += i * mh;
            nw = mw - 2 * c->border;
            nh = mh - 2 * c->border;
        }
        else if(portrait)
        {
            nx += (i - nmaster) * tw;
            ny += mh * nmaster;
            nw = tw - 2 * c->border;
            nh = th - 2 * c->border + 1;
        }
        else
        {
            ny += mh * nmaster;
            nw = tw - 2 * c->border;
            if(th > 2 * c->border)
            {
                ny += (i - nmaster) * th;
                nh = th - 2 * c->border;
                if (i == n - 1)
                    nh += (n > nmaster) ? wah - mh - th * (n - nmaster) : 0;
            }
            else
                nh = wah - 2 * c->border;
        }
        resize(c, nx, ny, nw, nh, False);
        i++;
    }
}

void
bstack(Display *disp, awesome_config *awesomeconf)
{
    _bstack(disp, awesomeconf, False);
}

void
bstackportrait(Display *disp, awesome_config *awesomeconf)
{
    _bstack(disp, awesomeconf, True);
}
