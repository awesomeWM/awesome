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
#include "statusbar.h"
#include "layouts/floating.h"

/** Find the index of the first currently selected tag
 * \param screen the screen to search
 * \return tag
 */
Tag *
get_current_tag(VirtScreen screen)
{
    Tag *tag;

    for(tag = screen.tags; tag; tag = tag->next)
        if(tag->selected)
            return tag;

    return NULL;
}

/** Arrange windows following current selected layout
 * \param disp display ref
 * \param awesomeconf awesome config
 */
void
arrange(awesome_config *awesomeconf, int screen)
{
    Client *c;
    Tag *curtag = get_current_tag(awesomeconf->screens[screen]);

    for(c = awesomeconf->clients; c; c = c->next)
    {
        if(client_isvisible(c, &awesomeconf->screens[screen], screen))
            client_unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == screen)
            client_ban(c);
    }

    curtag->layout->arrange(awesomeconf, screen);
    focus(curtag->client_sel, True, awesomeconf, screen);
    restack(awesomeconf, screen);
}

Layout *
get_current_layout(VirtScreen screen)
{
    Tag *curtag;

    if ((curtag = get_current_tag(screen)))
        return curtag->layout;

    return NULL;
}

void
uicb_client_focusnext(awesome_config * awesomeconf,
                      int screen,
                      const char *arg __attribute__ ((unused)))
{
    Client *c, *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(!sel)
        return;
    for(c = sel->next; c && !client_isvisible(c, &awesomeconf->screens[screen], screen); c = c->next);
    if(!c)
        for(c = awesomeconf->clients; c && !client_isvisible(c, &awesomeconf->screens[screen], screen); c = c->next);
    if(c)
    {
        focus(c, True, awesomeconf, screen);
        restack(awesomeconf, screen);
    }
}

void
uicb_client_focusprev(awesome_config *awesomeconf,
                      int screen,
                      const char *arg __attribute__ ((unused)))
{
    Client *c, *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(!sel)
        return;
    for(c = sel->prev; c && !client_isvisible(c, &awesomeconf->screens[screen], screen); c = c->prev);
    if(!c)
    {
        for(c = awesomeconf->clients; c && c->next; c = c->next);
        for(; c && !client_isvisible(c, &awesomeconf->screens[screen], screen); c = c->prev);
    }
    if(c)
    {
        focus(c, True, awesomeconf, screen);
        restack(awesomeconf, screen);
    }
}

void
loadawesomeprops(awesome_config *awesomeconf, int screen)
{
    int i, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 1);

    if(xgettextprop(awesomeconf->display,
                    RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)),
                    AWESOMEPROPS_ATOM(awesomeconf->display), prop, ntags + 1))
        for(i = 0, tag = awesomeconf->screens[screen].tags; tag && prop[i]; i++, tag = tag->next)
            if(prop[i] == '1')
                tag->selected = True;
            else
                tag->selected = False;

    p_delete(&prop);
}

void
restack(awesome_config *awesomeconf, int screen)
{
    Client *c, *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;
    XEvent ev;
    XWindowChanges wc;

    statusbar_draw(awesomeconf, screen);
    if(!sel)
        return;
    if(awesomeconf->screens[screen].allow_lower_floats)
        XRaiseWindow(awesomeconf->display, sel->win);
    else
    {
        if(sel->isfloating ||
           get_current_layout(awesomeconf->screens[screen])->arrange == layout_floating)
            XRaiseWindow(sel->display, sel->win);
        if(!(get_current_layout(awesomeconf->screens[screen])->arrange == layout_floating))
        {
            wc.stack_mode = Below;
            wc.sibling = awesomeconf->screens[screen].statusbar.window;
            if(!sel->isfloating)
            {
                XConfigureWindow(sel->display, sel->win, CWSibling | CWStackMode, &wc);
                wc.sibling = sel->win;
            }
            for(c = awesomeconf->clients; c; c = c->next)
            {
                if(!IS_TILED(c, &awesomeconf->screens[screen], screen) || c == sel)
                    continue;
                XConfigureWindow(awesomeconf->display, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
        }
    }
    if(awesomeconf->screens[screen].focus_move_pointer)
        XWarpPointer(awesomeconf->display, None, sel->win, 0, 0, 0, 0, sel->w / 2, sel->h / 2);
    XSync(awesomeconf->display, False);
    while(XCheckMaskEvent(awesomeconf->display, EnterWindowMask, &ev));
}

void
saveawesomeprops(awesome_config *awesomeconf, int screen)
{
    int i, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 1);

    for(i = 0, tag = awesomeconf->screens[screen].tags; tag; tag = tag->next, i++)
        prop[i] = tag->selected ? '1' : '0';

    prop[i] = '\0';
    XChangeProperty(awesomeconf->display,
                    RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)),
                    AWESOMEPROPS_ATOM(awesomeconf->display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

void
uicb_tag_setlayout(awesome_config * awesomeconf,
                   int screen,
                   const char *arg)
{
    Layout *l = awesomeconf->screens[screen].layouts;
    Tag *tag;
    int i;

    if(arg)
    {
        for(i = 0; l && l != get_current_layout(awesomeconf->screens[screen]); i++, l = l->next);
        if(!l)
            i = 0;
        for(i = compute_new_value_from_arg(arg, (double) i),
            l = awesomeconf->screens[screen].layouts; l && i > 0; i--)
            l = l->next;
        if(!l)
            l = awesomeconf->screens[screen].layouts;
    }

    for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
        if(tag->selected)
            tag->layout = l;

    if(get_current_tag(awesomeconf->screens[screen])->client_sel)
        arrange(awesomeconf, screen);
    else
        statusbar_draw(awesomeconf, screen);

    saveawesomeprops(awesomeconf, screen);
}

static void
maximize(int x, int y, int w, int h, awesome_config *awesomeconf, int screen)
{
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(!sel)
        return;

    if((sel->ismax = !sel->ismax))
    {
        sel->wasfloating = sel->isfloating;
        sel->isfloating = True;
        client_resize(sel, x, y, w, h, awesomeconf, True, !sel->isfloating);
    }
    else if(sel->wasfloating)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True, False);
    else
        sel->isfloating = False;

    arrange(awesomeconf, screen);
}

void
uicb_client_togglemax(awesome_config *awesomeconf,
                      int screen,
                      const char *arg __attribute__ ((unused)))
{
    ScreenInfo *si = get_screen_info(awesomeconf->display, screen, &awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);

    maximize(si[screen].x_org, si[screen].y_org,
             si[screen].width - 2 * awesomeconf->screens[screen].borderpx,
             si[screen].height - 2 * awesomeconf->screens[screen].borderpx,
             awesomeconf, screen);
    p_delete(&si);
}

void
uicb_client_toggleverticalmax(awesome_config *awesomeconf,
                              int screen,
                              const char *arg __attribute__ ((unused)))
{
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;
    ScreenInfo *si = get_screen_info(awesomeconf->display, screen, &awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);

    if(sel)
        maximize(sel->x,
                 si[screen].y_org,
                 sel->w,
                 si[screen].height - 2 * awesomeconf->screens[screen].borderpx,
                 awesomeconf, screen);
    p_delete(&si);
}


void
uicb_client_togglehorizontalmax(awesome_config *awesomeconf,
                                int screen,
                                const char *arg __attribute__ ((unused)))
{
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;
    ScreenInfo *si = get_screen_info(awesomeconf->display, screen, &awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);

    if(sel)
        maximize(si[screen].x_org,
                 sel->y,
                 si[screen].height - 2 * awesomeconf->screens[screen].borderpx,
                 sel->h,
                 awesomeconf, screen);
    p_delete(&si);
}

void
uicb_client_zoom(awesome_config *awesomeconf,
                 int screen,
                 const char *arg __attribute__ ((unused)))
{
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(awesomeconf->clients == sel)
         for(sel = sel->next; sel && !client_isvisible(sel, &awesomeconf->screens[screen], screen); sel = sel->next);

    if(!sel)
        return;

    client_detach(&awesomeconf->clients, sel);
    client_attach(&awesomeconf->clients, sel);

    focus(sel, True, awesomeconf, screen);
    arrange(awesomeconf, screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
