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
#include "util.h"
#include "statusbar.h"
#include "layouts/floating.h"

/* extern */
extern Client *clients, *sel;   /* global client list */

void
arrange(Display * disp, DC *drawcontext, awesome_config *awesomeconf)
{
    Client *c;

    for(c = clients; c; c = c->next)
    {
        if(isvisible(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags))
            unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == awesomeconf->screen)
            ban(c);
    }
    awesomeconf->current_layout->arrange(disp, awesomeconf);
    focus(disp, drawcontext, NULL, True, awesomeconf);
    restack(disp, drawcontext, awesomeconf);
}

void
uicb_focusnext(Display *disp __attribute__ ((unused)),
               DC *drawcontext,
               awesome_config * awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->next; c && !isvisible(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags); c = c->next);
    if(!c)
        for(c = clients; c && !isvisible(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags); c = c->next);
    if(c)
    {
        focus(c->display, drawcontext, c, True, awesomeconf);
        restack(c->display, drawcontext, awesomeconf);
    }
}

void
uicb_focusprev(Display *disp __attribute__ ((unused)),
               DC *drawcontext,
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->prev; c && !isvisible(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags); c = c->prev);
    if(!c)
    {
        for(c = clients; c && c->next; c = c->next);
        for(; c && !isvisible(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags); c = c->prev);
    }
    if(c)
    {
        focus(c->display, drawcontext, c, True, awesomeconf);
        restack(c->display, drawcontext, awesomeconf);
    }
}

void
loadawesomeprops(Display *disp, awesome_config * awesomeconf)
{
    int i;
    char *prop;

    prop = p_new(char, awesomeconf->ntags + 1);

    if(xgettextprop(disp, RootWindow(disp, awesomeconf->screen), AWESOMEPROPS_ATOM(disp), prop, awesomeconf->ntags + 1))
        for(i = 0; i < awesomeconf->ntags && prop[i]; i++)
            awesomeconf->selected_tags[i] = prop[i] == '1';

    p_delete(&prop);
}

void
restack(Display * disp, DC * drawcontext, awesome_config *awesomeconf)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawstatusbar(disp, awesomeconf->screen, drawcontext, awesomeconf);
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
            if(!IS_TILED(c, awesomeconf->screen, awesomeconf->selected_tags, awesomeconf->ntags) || c == sel)
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
    XChangeProperty(disp, RootWindow(disp, awesomeconf->screen),
                    AWESOMEPROPS_ATOM(disp), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

void
uicb_setlayout(Display *disp,
               DC *drawcontext,
               awesome_config * awesomeconf,
               const char *arg)
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
        arrange(disp, drawcontext, awesomeconf);
    else
        drawstatusbar(disp, awesomeconf->screen, drawcontext, awesomeconf);

    saveawesomeprops(disp, awesomeconf);

    for(j = 0; j < awesomeconf->ntags; j++)
        if (awesomeconf->selected_tags[j])
            awesomeconf->tag_layouts[j] = awesomeconf->current_layout;
}

static void
maximize(int x, int y, int w, int h, DC *drawcontext, awesome_config *awesomeconf)
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

    drawstatusbar(sel->display, sel->screen, drawcontext, awesomeconf);

    while(XCheckMaskEvent(sel->display, EnterWindowMask, &ev));
}

void
uicb_togglemax(Display *disp,
               DC *drawcontext,
               awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    maximize(get_windows_area_x(awesomeconf->statusbar),
             get_windows_area_y(awesomeconf->statusbar),
             get_windows_area_width(disp, awesomeconf->statusbar) - 2 * awesomeconf->borderpx,
             get_windows_area_height(disp, awesomeconf->statusbar) - 2 * awesomeconf->borderpx,
             drawcontext,
             awesomeconf);
}

void
uicb_toggleverticalmax(Display *disp,
                       DC *drawcontext,
                       awesome_config *awesomeconf,
                       const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(sel->x,
                 get_windows_area_y(awesomeconf->statusbar),
                 sel->w,
                 get_windows_area_height(disp, awesomeconf->statusbar) - 2 * awesomeconf->borderpx,
                 drawcontext,
                 awesomeconf);
}


void
uicb_togglehorizontalmax(Display *disp,
                         DC *drawcontext,
                         awesome_config *awesomeconf,
                         const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(get_windows_area_x(awesomeconf->statusbar),
                 sel->y,
                 get_windows_area_height(disp, awesomeconf->statusbar) - 2 * awesomeconf->borderpx,
                 sel->h,
                 drawcontext,
                 awesomeconf);
}

void 
uicb_zoom(Display *disp __attribute__ ((unused)), 
          DC *drawcontext __attribute__ ((unused)),
          awesome_config *awesomeconf,
          const char *arg __attribute__ ((unused))) 
{ 
    if(!sel)
        return;
    detach(sel);
    attach(sel);
    focus(sel->display, drawcontext, sel, True, awesomeconf);
    arrange(sel->display, drawcontext, awesomeconf);
} 

