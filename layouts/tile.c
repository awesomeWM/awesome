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

#include "screen.h"
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
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;

    if(!arg)
        nmaster = awesomeconf->nmaster;
    else
    {
        nmaster = (int) compute_new_value_from_arg(arg, (double) nmaster);
        if(nmaster < 0)
            nmaster = 0;
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
    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;

    if(!arg)
        mwfact = awesomeconf->mwfact;
    else
    {
        mwfact = compute_new_value_from_arg(arg, mwfact);
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
    /* windows area geometry */
    int wah = 0, waw = 0, wax = 0, way = 0;
    /* new coordinates */
    unsigned int nx, ny, nw, nh;
    /* master size */
    unsigned int mw = 0, mh = 0;
    int n, i, li, last_i = 0, nmaster_screen = 0, otherwin_screen = 0;
    int screen_numbers = 1, use_screen = -1;
    ScreenInfo *screens_info = NULL;
    Client *c;

    screens_info = get_screen_info(disp, awesomeconf->statusbar, &screen_numbers);
 
    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;

        if(use_screen == -1 || (screen_numbers > 1 && i && ((i - last_i) >= nmaster_screen + otherwin_screen || n == screen_numbers)))
        {
            use_screen++;
            last_i = i;

            wah = screens_info[use_screen].height;
            waw = screens_info[use_screen].width;
            wax = screens_info[use_screen].x_org;
            way = screens_info[use_screen].y_org;

            if(n >= nmaster * screen_numbers)
            {
                nmaster_screen = nmaster;
                otherwin_screen = (n - (nmaster * screen_numbers)) / screen_numbers;
                if(use_screen == 0)
                    otherwin_screen += (n - (nmaster * screen_numbers)) % screen_numbers;
            }
            else
            {
                nmaster_screen = n / screen_numbers;
                /* first screen takes more master */
                if(use_screen == 0)
                    nmaster_screen += n % screen_numbers;
                otherwin_screen = 0;
            }

            mh = nmaster_screen ? wah / nmaster_screen : waw;
            mw = otherwin_screen ? waw * mwfact : waw;
        }

        c->ismax = False;
        li = last_i ? i - last_i : i;
        if(li < nmaster)
        {                       /* master */
            ny = way + li * mh;
            if(right)
                nx = wax;
            else
                nx = wax + (waw - mw);
            nw = mw - 2 * c->border;
            nh = mh - 2 * c->border;
        }
        else
        {                       /* tile window */
            nh = wah / otherwin_screen - 2 * c->border;
            if(nmaster)
                nw = waw - mw - 2 * c->border;
            else
                nw = waw - 2 * c->border;
            if(li == nmaster)
                ny = way;
            else
                ny = way + (wah / otherwin_screen) * (li - nmaster_screen);
            if(right && nmaster)
                nx = mw + wax;
            else
                nx = wax;
        }
        resize(c, nx, ny, nw, nh, awesomeconf->resize_hints);
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
