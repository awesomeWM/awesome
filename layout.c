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

#include "tag.h"
#include "util.h"
#include "xutil.h"
#include "focus.h"
#include "widget.h"
#include "window.h"
#include "ewmh.h"
#include "client.h"
#include "screen.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/fibonacci.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

#include "layoutgen.h"

/** Arrange windows following current selected layout
 * \param screen the screen to arrange
 */
static void
arrange(int screen)
{
    Client *c;
    Tag **curtags = get_current_tags(screen);
    Window client_win, root_win;
    int x, y, d;
    unsigned int m;

    for(c = globalconf.clients; c; c = c->next)
    {
        if(client_isvisible(c, screen) && !c->newcomer)
            client_unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == screen || c->newcomer)
            client_ban(c);
    }

    curtags[0]->layout->arrange(screen);
    for(c = globalconf.clients; c; c = c->next)
        if(c->newcomer && client_isvisible(c, screen))
        {
            c->newcomer = False;
            client_unban(c);
        }
    c = focus_get_current_client(screen);
    focus(c, True, screen);
    if(c && XQueryPointer(globalconf.display, RootWindow(globalconf.display, get_phys_screen(screen)),
                          &root_win, &client_win, &x, &y, &d, &d, &m) &&
            (root_win == None || client_win == None || client_win == root_win))
            window_grabbuttons(c->screen, c->win, False, False);

    p_delete(&curtags);
    restack(screen);

    /* reset status */
    globalconf.screens[screen].need_arrange = False;
}

Bool
layout_refresh(void)
{
    int screen;
    Bool arranged = False;

    for(screen = 0; screen < get_screen_count(); screen++)
        if(globalconf.screens[screen].need_arrange)
        {
            arrange(screen);
            arranged = True;
        }

    return arranged;
}

Layout *
get_current_layout(int screen)
{
    Tag **curtags = get_current_tags(screen);
    Layout *l = curtags[0]->layout;
    p_delete(&curtags);
    return l;
}

void
loadawesomeprops(int screen)
{
    int i, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 1);

    if(xgettextprop(RootWindow(globalconf.display, get_phys_screen(screen)),
                    XInternAtom(globalconf.display, "_AWESOME_PROPERTIES", False),
                    prop, ntags + 1))
        for(i = 0, tag = globalconf.screens[screen].tags; tag && prop[i]; i++, tag = tag->next)
            if(prop[i] == '1')
                tag->selected = True;
            else
                tag->selected = False;

    p_delete(&prop);

    ewmh_update_net_current_desktop(get_phys_screen(screen));
}

void
restack(int screen)
{
    Client *c, *sel = globalconf.focus->client;
    XWindowChanges wc;
    Layout *curlay = get_current_layout(screen);

    if(!sel)
        return;

    if(globalconf.screens[screen].allow_lower_floats)
        XRaiseWindow(globalconf.display, sel->win);
    else
    {
        if(sel->isfloating || curlay->arrange == layout_floating)
            XRaiseWindow(globalconf.display, sel->win);
        if(curlay->arrange != layout_floating)
        {
            wc.stack_mode = Below;
            if(!sel->isfloating)
                XConfigureWindow(globalconf.display, sel->win, CWStackMode, &wc);
            for(c = globalconf.clients; c; c = c->next)
            {
                if(!IS_TILED(c, screen) || c == sel)
                    continue;
                XConfigureWindow(globalconf.display, c->win, CWStackMode, &wc);
            }
        }
    }
    if(globalconf.screens[screen].focus_move_pointer)
        XWarpPointer(globalconf.display, None, sel->win, 0, 0, 0, 0,
                     sel->geometry.width / 2, sel->geometry.height / 2);
}

void
saveawesomeprops(int screen)
{
    int i, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 1);

    for(i = 0, tag = globalconf.screens[screen].tags; tag; tag = tag->next, i++)
        prop[i] = tag->selected ? '1' : '0';

    prop[i] = '\0';
    XChangeProperty(globalconf.display,
                    RootWindow(globalconf.display, get_phys_screen(screen)),
                    XInternAtom(globalconf.display, "_AWESOME_PROPERTIES", False),
                    XA_STRING, 8, PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

/** Set layout for tag
 * \param screen Screen ID
 * \param arg Layout specifier
 * \ingroup ui_callback
 */
void
uicb_tag_setlayout(int screen, char *arg)
{
    Layout *l = globalconf.screens[screen].layouts;
    Tag *tag, **curtags;
    int i;

    if(arg)
    {
        curtags = get_current_tags(screen);
        for(i = 0; l && l != curtags[0]->layout; i++, l = l->next);
        p_delete(&curtags);

        if(!l)
            i = 0;

        i = compute_new_value_from_arg(arg, (double) i);

        if(i >= 0)
            for(l = globalconf.screens[screen].layouts; l && i > 0; i--)
                 l = l->next;
        else
            for(l = globalconf.screens[screen].layouts; l && i < 0; i++)
                 l = layout_list_prev_cycle(&globalconf.screens[screen].layouts, l);

        if(!l)
            l = globalconf.screens[screen].layouts;
    }

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected)
            tag->layout = l;

    if(globalconf.focus->client)
        arrange(screen);

    widget_invalidate_cache(screen, WIDGET_CACHE_LAYOUTS);

    saveawesomeprops(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
