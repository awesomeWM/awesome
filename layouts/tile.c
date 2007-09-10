/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>

#include "layouts/tile.h"
#include "layout.h"
#include "tag.h"

/* extern */
extern int wax, way, wah, waw;  /* windowarea geometry */
extern Client *sel, *clients;

/* static */

static double mwfact = 0.6;
static void _tile(jdwm_config *, const Bool);    /* arranges all windows tiled */

static int nmaster = 2;

void
uicb_incnmaster(Display *disp,
           jdwm_config *jdwmconf,
           const char * arg)
{
    int i;

    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;
    if(!arg)
        nmaster = jdwmconf->nmaster;
    else
    {
        i = strtol(arg, (char **) NULL, 10);
        if((nmaster + i) < 1 || wah / (nmaster + i) <= 2 * jdwmconf->borderpx)
            return;
        nmaster += i;
    }
    if(sel)
        arrange(disp, jdwmconf);
    else
        drawstatus(disp, jdwmconf);
}

void
uicb_setmwfact(Display *disp,
          jdwm_config * jdwmconf,
          const char *arg)
{
    double delta;

    if(!IS_ARRANGE(tile) && !IS_ARRANGE(tileleft) && !IS_ARRANGE(bstack) && !IS_ARRANGE(bstackportrait))
        return;

    /* arg handling, manipulate mwfact */
    if(!arg)
        mwfact = jdwmconf->mwfact;
    else if(1 == sscanf(arg, "%lf", &delta))
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
    arrange(disp, jdwmconf);
}

static void
_tile(jdwm_config *jdwmconf, const Bool right)
{
    unsigned int nx, ny, nw, nh, mw;
    int n, th, i, mh;
    Client *c;

    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, jdwmconf->selected_tags, jdwmconf->ntags))
            n++;

    /* window geoms */
    mh = (n <= nmaster) ? wah / (n > 0 ? n : 1) : wah / nmaster;
    mw = (n <= nmaster) ? waw : mwfact * waw;
    th = (n > nmaster) ? wah / (n - nmaster) : 0;
    if(n > nmaster && th < jdwmconf->statusbar.height)
        th = wah;

    nx = wax;
    ny = way;
    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, jdwmconf->selected_tags, jdwmconf->ntags))
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
        resize(c, nx, ny, nw, nh, jdwmconf->resize_hints);
        if(n > nmaster && th != wah)
            ny += nh + 2 * c->border;
        i++;
    }
}

void
tile(Display *disp __attribute__ ((unused)), jdwm_config *jdwmconf)
{
    _tile(jdwmconf, True);
}

void
tileleft(Display *disp __attribute__ ((unused)), jdwm_config *jdwmconf)
{
    _tile(jdwmconf, False);
}


static void
_bstack(jdwm_config *jdwmconf, Bool portrait)
{
    int i, n, nx, ny, nw, nh, mw, mh, tw, th;
    Client *c;

    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, jdwmconf->selected_tags, jdwmconf->ntags))
            n++;

    /* window geoms */
    mh = (n > nmaster) ? (wah * mwfact) / nmaster : wah / (n > 0 ? n : 1);
    mw = waw;
    th = (n > nmaster) ? (wah * (1 - mwfact)) / (portrait ? 1 : n - nmaster) : 0;
    tw = (n > nmaster) ? waw / (portrait ? n - nmaster : 1) : 0;

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, jdwmconf->selected_tags, jdwmconf->ntags))
            continue;

        c->ismax = False;
        nx = wax;
        ny = way;
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
bstack(Display *disp __attribute__ ((unused)), jdwm_config *jdwmconf)
{
    _bstack(jdwmconf, False);
}

void
bstackportrait(Display *disp __attribute__ ((unused)), jdwm_config *jdwmconf)
{
    _bstack(jdwmconf, True);
}
