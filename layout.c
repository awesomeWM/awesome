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

#include "screen.h"
#include "layout.h"
#include "tag.h"
#include "util.h"
#include "xutil.h"
#include "focus.h"
#include "statusbar.h"
#include "ewmh.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/fibonacci.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

const NameFuncLink LayoutsList[] =
{
    {"tile", layout_tile},
    {"tileleft", layout_tileleft},
    {"max", layout_max},
    {"spiral", layout_spiral},
    {"dwindle", layout_dwindle},
    {"floating", layout_floating},
    {NULL, NULL}
};

/** Arrange windows following current selected layout
 * \param screen the screen to arrange
 */
void
arrange(int screen)
{
    Client *c;
    Tag **curtags = get_current_tags(screen);

    for(c = globalconf.clients; c; c = c->next)
    {
        if(client_isvisible(c, screen))
            client_unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == screen)
            client_ban(c);
    }

    curtags[0]->layout->arrange(screen);
    focus(focus_get_current_client(screen), True, screen);
    p_delete(&curtags);
    restack(screen);
}

/** Send focus to next client in stack
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusnext(int screen, char *arg __attribute__ ((unused)))
{
    Client *c, *sel = globalconf.focus->client;

    if(!sel)
        return;
    for(c = sel->next; c && (c->skip || !client_isvisible(c, screen)); c = c->next);
    if(!c)
        for(c = globalconf.clients; c && (c->skip || !client_isvisible(c, screen)); c = c->next);
    if(c)
    {
        focus(c, True, screen);
        restack(screen);
    }
}

/** Send focus to previous client in stack
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusprev(int screen, char *arg __attribute__ ((unused)))
{
    Client *c, *sel = globalconf.focus->client;

    if(!sel)
        return;
    for(c = sel->prev; c && (c->skip || !client_isvisible(c, screen)); c = c->prev);
    if(!c)
    {
        for(c = globalconf.clients; c && c->next; c = c->next);
        for(; c && (c->skip || !client_isvisible(c, screen)); c = c->prev);
    }
    if(c)
    {
        focus(c, True, screen);
        restack(screen);
    }
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

    if(xgettextprop(globalconf.display,
                    RootWindow(globalconf.display, get_phys_screen(screen)),
                    AWESOMEPROPS_ATOM(globalconf.display), prop, ntags + 1))
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
    XEvent ev;
    XWindowChanges wc;
    Tag **curtags;

    statusbar_draw(screen);

    if(!sel)
        return;

    if(globalconf.screens[screen].allow_lower_floats)
        XRaiseWindow(globalconf.display, sel->win);
    else
    {
        curtags = get_current_tags(screen);
        if(sel->isfloating ||
           curtags[0]->layout->arrange == layout_floating)
            XRaiseWindow(globalconf.display, sel->win);
        if(!(curtags[0]->layout->arrange == layout_floating))
        {
            wc.stack_mode = Below;
            wc.sibling = globalconf.screens[screen].statusbar->window;
            if(!sel->isfloating)
            {
                XConfigureWindow(globalconf.display, sel->win, CWSibling | CWStackMode, &wc);
                wc.sibling = sel->win;
            }
            for(c = globalconf.clients; c; c = c->next)
            {
                if(!IS_TILED(c, screen) || c == sel)
                    continue;
                XConfigureWindow(globalconf.display, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
        }
        p_delete(&curtags);
    }
    if(globalconf.screens[screen].focus_move_pointer)
        XWarpPointer(globalconf.display, None, sel->win, 0, 0, 0, 0, sel->w / 2, sel->h / 2);
    XSync(globalconf.display, False);
    while(XCheckMaskEvent(globalconf.display, EnterWindowMask, &ev));
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
                    AWESOMEPROPS_ATOM(globalconf.display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
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
        for(i = compute_new_value_from_arg(arg, (double) i),
            l = globalconf.screens[screen].layouts; l && i > 0; i--)
            l = l->next;
        if(!l)
            l = globalconf.screens[screen].layouts;
    }

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected)
            tag->layout = l;

    if(globalconf.focus->client)
        arrange(screen);
    else
        statusbar_draw(screen);

    saveawesomeprops(screen);
}

static void
maximize(int x, int y, int w, int h, int screen)
{
    Client *sel = globalconf.focus->client;

    if(!sel)
        return;

    if((sel->ismax = !sel->ismax))
    {
        sel->wasfloating = sel->isfloating;
        sel->isfloating = True;
        client_resize(sel, x, y, w, h, True, True);
    }
    else if(sel->wasfloating)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True, False);
    else
        sel->isfloating = False;

    arrange(screen);
}

/** Toggle maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglemax(int screen, char *arg __attribute__ ((unused)))
{
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);
    maximize(area.x, area.y,
             area.width - 2 * globalconf.screens[screen].borderpx,
             area.height - 2 * globalconf.screens[screen].borderpx,
             screen);
}

/** Toggle vertical maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_toggleverticalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
        maximize(sel->x,
                 area.y,
                 sel->w,
                 area.height - 2 * globalconf.screens[screen].borderpx,
                 screen);
}


/** Toggle horizontal maximize for client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglehorizontalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
        maximize(area.x,
                 sel->y,
                 area.height - 2 * globalconf.screens[screen].borderpx,
                 sel->h,
                 screen);
}

/** Zoom client
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_zoom(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;

    if(globalconf.clients == sel)
         for(sel = sel->next; sel && !client_isvisible(sel, screen); sel = sel->next);

    if(!sel)
        return;

    client_detach(sel);
    client_attach(sel);

    focus(sel, True, screen);
    arrange(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
