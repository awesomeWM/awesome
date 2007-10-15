/*
 * tab.c - tab management
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

#include "tab.h"
#include "event.h"
#include "layout.h"

void
client_untab(Client *c)
{
    Client *tmp;

    if(c->tab.next)
        c->tab.next->tab.isvisible = True;
    else if(c->tab.prev)
        c->tab.prev->tab.isvisible = True;

    c->tab.isvisible = True;

    if(c->tab.next)
        c->tab.next->tab.prev = c->tab.prev;

    tmp = c->tab.next;
    c->tab.next = NULL;

    if(c->tab.prev)
        c->tab.prev->tab.next = tmp;

    c->tab.prev = NULL;
}

void
uicb_tab(awesome_config *awesomeconf,
         const char *arg __attribute__ ((unused)))
{
    Window dummy, child;
    int x1, y1, di;
    unsigned int dui;
    XEvent ev;
    Client *sel = *awesomeconf->client_sel, *c = NULL, *tmp;

    if(XGrabPointer(awesomeconf->display, RootWindow(awesomeconf->display, awesomeconf->phys_screen),
                    False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None,
                    awesomeconf[awesomeconf->screen].cursor[CurMove], CurrentTime) != GrabSuccess)
        return;

    for(;;)
    {
        XMaskEvent(awesomeconf->display, ButtonPressMask, &ev);
        if(ev.type == ButtonPress)
        {
            XUngrabPointer(awesomeconf->display, CurrentTime);
            break;
        }
    }

    XQueryPointer(awesomeconf->display,
                  RootWindow(awesomeconf->display, awesomeconf->phys_screen),
                  &dummy, &child, &x1, &y1, &di, &di, &dui);

    if((c = get_client_bywin(awesomeconf->clients, child))
       && c != sel && !c->isfloating)
    {
        /* take the last tabbed window */
        for(tmp = sel; tmp->tab.next; tmp = tmp->tab.next);
        tmp->tab.next = c;
        c->tab.prev = tmp;

        c->tab.isvisible = False;
        arrange(awesomeconf);
        focus(sel, True, awesomeconf);
    }
}

void
uicb_untab(awesome_config *awesomeconf,
           const char *arg __attribute__ ((unused)))
{
    Client *sel = *awesomeconf->client_sel;

    if(!sel)
        return;

    client_untab(sel);
    arrange(awesomeconf);
    focus(sel, True, awesomeconf);
}

void
uicb_viewnexttab(awesome_config *awesomeconf,
                 const char *arg __attribute__ ((unused)))
{
    Client *sel = *awesomeconf->client_sel;

    if(!sel || !sel->tab.next)
        return;

    sel->tab.isvisible = False;
    sel->tab.next->tab.isvisible = True;
    arrange(awesomeconf);
    focus(sel->tab.next, True, awesomeconf);
}

void
uicb_viewprevtab(awesome_config *awesomeconf,
                 const char *arg __attribute__ ((unused)))
{
    Client *sel = *awesomeconf->client_sel;

    if(!sel || !sel->tab.prev)
        return;

    sel->tab.isvisible = False;
    sel->tab.prev->tab.isvisible = True;
    arrange(awesomeconf);
    focus(sel->tab.prev, True, awesomeconf);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
