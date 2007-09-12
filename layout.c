/*  
 * layout.c - layout management
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

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "awesome.h"
#include "layout.h"
#include "tag.h"
#include "layouts/floating.h"

/* extern */
extern int wax, way, wah, waw;  /* windowarea geometry */
extern Client *clients, *sel;   /* global client list */
extern DC dc;

void
arrange(Display * disp, awesome_config *awesomeconf)
{
    Client *c;

    for(c = clients; c; c = c->next)
        if(isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags))
            unban(c);
        else
            ban(c);
    awesomeconf->current_layout->arrange(disp, awesomeconf);
    focus(disp, &dc, NULL, True, awesomeconf);
    restack(disp, awesomeconf);
}

void
uicb_focusnext(Display *disp __attribute__ ((unused)),
               awesome_config * awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->next; c && !isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags); c = c->next);
    if(!c)
        for(c = clients; c && !isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags); c = c->next);
    if(c)
    {
        focus(c->display, &dc, c, True, awesomeconf);
        restack(c->display, awesomeconf);
    }
}

void
uicb_focusprev(Display *disp __attribute__ ((unused)),
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->prev; c && !isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags); c = c->prev);
    if(!c)
    {
        for(c = clients; c && c->next; c = c->next);
        for(; c && !isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags); c = c->prev);
    }
    if(c)
    {
        focus(c->display, &dc, c, True, awesomeconf);
        restack(c->display, awesomeconf);
    }
}

void
loadawesomeprops(Display *disp, awesome_config * awesomeconf)
{
    int i;
    char *prop;

    prop = p_new(char, awesomeconf->ntags + 1);

    if(xgettextprop(disp, DefaultRootWindow(disp), AWESOMEPROPS_ATOM(disp), prop, awesomeconf->ntags + 1))
        for(i = 0; i < awesomeconf->ntags && prop[i]; i++)
            awesomeconf->selected_tags[i] = prop[i] == '1';

    p_delete(&prop);
}

void
restack(Display * disp, awesome_config *awesomeconf)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawstatus(disp, awesomeconf);
    if(!sel)
        return;
    if(sel->isfloating || IS_ARRANGE(floating))
        XRaiseWindow(disp, sel->win);
    if(!IS_ARRANGE(floating))
    {
        wc.stack_mode = Below;
        wc.sibling = awesomeconf->statusbar.window;
        if(!sel->isfloating)
        {
            XConfigureWindow(disp, sel->win, CWSibling | CWStackMode, &wc);
            wc.sibling = sel->win;
        }
        for(c = clients; c; c = c->next)
        {
            if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags) || c == sel)
                continue;
            XConfigureWindow(disp, c->win, CWSibling | CWStackMode, &wc);
            wc.sibling = c->win;
        }
    }
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

void
saveawesomeprops(Display *disp, awesome_config *awesomeconf)
{
    int i;
    char *prop;

    prop = p_new(char, awesomeconf->ntags + 1);
    for(i = 0; i < awesomeconf->ntags; i++)
        prop[i] = awesomeconf->selected_tags[i] ? '1' : '0';
    prop[i] = '\0';
    XChangeProperty(disp, DefaultRootWindow(disp),
                    AWESOMEPROPS_ATOM(disp), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

void
uicb_setlayout(Display *disp, awesome_config * awesomeconf, const char *arg)
{
    int i, j;
    Client *c;

    if(!arg)
    {
        if(!(++awesomeconf->current_layout)->symbol)
            awesomeconf->current_layout = &awesomeconf->layouts[0];
    }
    else
    {
        i = strtol(arg, NULL, 10);
        if(i < 0 || i >= awesomeconf->nlayouts)
            return;
        awesomeconf->current_layout = &awesomeconf->layouts[i];
    }

    for(c = clients; c; c = c->next)
        c->ftview = True;

    if(sel)
        arrange(disp, awesomeconf);
    else
        drawstatus(disp, awesomeconf);

    saveawesomeprops(disp, awesomeconf);

    for(j = 0; j < awesomeconf->ntags; j++)
        if (awesomeconf->selected_tags[j])
            awesomeconf->tag_layouts[j] = awesomeconf->current_layout;
}

void
uicb_togglebar(Display *disp,
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    if(awesomeconf->statusbar.position == BarOff)
        awesomeconf->statusbar.position = (awesomeconf->statusbar.position == BarOff) ? BarTop : awesomeconf->statusbar_default_position;
    else
        awesomeconf->statusbar.position = BarOff;
    updatebarpos(disp, awesomeconf->statusbar);
    arrange(disp, awesomeconf);
}

static void
maximize(int x, int y, int w, int h, awesome_config *awesomeconf)
{
    XEvent ev;

    if(!sel)
        return;

    if((sel->ismax = !sel->ismax))
    {
        sel->wasfloating = sel->isfloating;
        sel->isfloating = True;
        sel->rx = sel->x;
        sel->ry = sel->y;
        sel->rw = sel->w;
        sel->rh = sel->h;
        resize(sel, x, y, w, h, True);
    }
    else if(sel->isfloating)
        resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True);
    else
        sel->isfloating = False;

    drawstatus(sel->display, awesomeconf);

    while(XCheckMaskEvent(sel->display, EnterWindowMask, &ev));
}

void
uicb_togglemax(Display *disp __attribute__ ((unused)),
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    maximize(wax, way, waw - 2 * awesomeconf->borderpx, wah - 2 * awesomeconf->borderpx, awesomeconf);
}

void
uicb_toggleverticalmax(Display *disp __attribute__ ((unused)),
                       awesome_config *awesomeconf,
                       const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(sel->x, way, sel->w, wah - 2 * awesomeconf->borderpx, awesomeconf);
}


void
uicb_togglehorizontalmax(Display *disp __attribute__ ((unused)),
                         awesome_config *awesomeconf,
                         const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(wax, sel->y, waw - 2 * awesomeconf->borderpx, sel->h, awesomeconf);
}

void 
uicb_zoom(Display *disp __attribute__ ((unused)), 
          awesome_config *awesomeconf,
          const char *arg __attribute__ ((unused))) 
{ 
    if(!sel)
        return;
    detach(sel);
    attach(sel);
    focus(sel->display, &dc, sel, True, awesomeconf);
    arrange(sel->display, awesomeconf);
} 

