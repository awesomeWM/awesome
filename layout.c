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
#include "statusbar.h"
#include "layouts/floating.h"

/** Arrange windows following current selected layout
 * \param disp display ref
 * \param awesomeconf awesome config
 */
void
arrange(awesome_config *awesomeconf)
{
    Client *c;

    for(c = *awesomeconf->clients; c; c = c->next)
    {
        if(isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags))
            unban(c);
        /* we don't touch other screens windows */
        else if(c->screen == awesomeconf->screen)
            ban(c);
    }
    get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange(awesomeconf);
    focus(NULL, True, awesomeconf);
    restack(awesomeconf);
}


Layout *
get_current_layout(Tag *tags, int ntags)
{
    int i;

    for(i = 0; i < ntags; i++)
        if(tags[i].selected)
            return tags[i].layout;

    return NULL;
}

void
uicb_focusnext(awesome_config * awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!*awesomeconf->client_sel)
        return;
    for(c = (*awesomeconf->client_sel)->next; c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); c = c->next);
    if(!c)
        for(c = *awesomeconf->clients; c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); c = c->next);
    if(c)
    {
        focus(c, True, awesomeconf);
        restack(awesomeconf);
    }
}

void
uicb_focusprev(awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!*awesomeconf->client_sel)
        return;
    for(c = (*awesomeconf->client_sel)->prev; c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); c = c->prev);
    if(!c)
    {
        for(c = *awesomeconf->clients; c && c->next; c = c->next);
        for(; c && !isvisible(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags); c = c->prev);
    }
    if(c)
    {
        focus(c, True, awesomeconf);
        restack(awesomeconf);
    }
}

void
loadawesomeprops(awesome_config * awesomeconf)
{
    int i;
    char *prop;

    prop = p_new(char, awesomeconf->ntags + 1);

    if(xgettextprop(awesomeconf->display, RootWindow(awesomeconf->display, awesomeconf->phys_screen),
                    AWESOMEPROPS_ATOM(awesomeconf->display), prop, awesomeconf->ntags + 1))
        for(i = 0; i < awesomeconf->ntags && prop[i]; i++)
            if(prop[i] == '1')
                awesomeconf->tags[i].selected = True;
            else
                awesomeconf->tags[i].selected = False;

    p_delete(&prop);
}

void
restack(awesome_config *awesomeconf)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawstatusbar(awesomeconf);
    if(!*awesomeconf->client_sel)
        return;
    if(awesomeconf->allow_lower_floats)
        XRaiseWindow(awesomeconf->display, (*awesomeconf->client_sel)->win);
    else
    {
        if((*awesomeconf->client_sel)->isfloating ||
           (get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange == layout_floating))
            XRaiseWindow(awesomeconf->display, (*awesomeconf->client_sel)->win);
        if(!(get_current_layout(awesomeconf->tags, awesomeconf->ntags)->arrange == layout_floating))
        {
            wc.stack_mode = Below;
            wc.sibling = awesomeconf->statusbar.window;
            if(!(*awesomeconf->client_sel)->isfloating)
            {
                XConfigureWindow(awesomeconf->display, (*awesomeconf->client_sel)->win, CWSibling | CWStackMode, &wc);
                wc.sibling = (*awesomeconf->client_sel)->win;
            }
            for(c = *awesomeconf->clients; c; c = c->next)
            {
                if(!IS_TILED(c, awesomeconf->screen, awesomeconf->tags, awesomeconf->ntags) || c == *awesomeconf->client_sel)
                    continue;
                XConfigureWindow(awesomeconf->display, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
        }
    }
    if(awesomeconf->focus_move_pointer)
        XWarpPointer(awesomeconf->display, None, (*awesomeconf->client_sel)->win, 0, 0, 0, 0, (*awesomeconf->client_sel)->w / 2, (*awesomeconf->client_sel)->h / 2);
    XSync(awesomeconf->display, False);
    while(XCheckMaskEvent(awesomeconf->display, EnterWindowMask, &ev));
}

void
saveawesomeprops(awesome_config *awesomeconf)
{
    int i;
    char *prop;

    prop = p_new(char, awesomeconf->ntags + 1);
    for(i = 0; i < awesomeconf->ntags; i++)
        prop[i] = awesomeconf->tags[i].selected ? '1' : '0';
    prop[i] = '\0';
    XChangeProperty(awesomeconf->display, RootWindow(awesomeconf->display, awesomeconf->phys_screen),
                    AWESOMEPROPS_ATOM(awesomeconf->display), XA_STRING, 8,
                    PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

void
uicb_setlayout(awesome_config * awesomeconf,
               const char *arg)
{
    int i, j;

    if(arg)
    {
        /* compute current index */
        for(i = 0; i < awesomeconf->nlayouts &&
            &awesomeconf->layouts[i] != get_current_layout(awesomeconf->tags, awesomeconf->ntags); i++);
        i = compute_new_value_from_arg(arg, (double) i);
        if(i >= awesomeconf->nlayouts)
            i = 0;
        else if(i < 0)
            i = awesomeconf->nlayouts - 1;
    }
    else
        i = 0;

    for(j = 0; j < awesomeconf->ntags; j++)
        if (awesomeconf->tags[j].selected)
            awesomeconf->tags[j].layout = &awesomeconf->layouts[i];

    if(*awesomeconf->client_sel)
        arrange(awesomeconf);
    else
        drawstatusbar(awesomeconf);

    saveawesomeprops(awesomeconf);
}

static void
maximize(int x, int y, int w, int h, awesome_config *awesomeconf)
{
    Client *sel = *awesomeconf->client_sel;

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
        resize(sel, x, y, w, h, awesomeconf, True);
    }
    else if(sel->wasfloating)
        resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True);
    else
        sel->isfloating = False;

    arrange(awesomeconf);
}

void
uicb_togglemax(awesome_config *awesomeconf,
               const char *arg __attribute__ ((unused)))
{
    ScreenInfo *si = get_screen_info(awesomeconf->display, awesomeconf->screen, &awesomeconf->statusbar);

    maximize(si[awesomeconf->screen].x_org, si[awesomeconf->screen].y_org,
             si[awesomeconf->screen].width - 2 * awesomeconf->borderpx,
             si[awesomeconf->screen].height - 2 * awesomeconf->borderpx,
             awesomeconf);
    p_delete(&si);
}

void
uicb_toggleverticalmax(awesome_config *awesomeconf,
                       const char *arg __attribute__ ((unused)))
{
    ScreenInfo *si = get_screen_info(awesomeconf->display, awesomeconf->screen, &awesomeconf->statusbar);

    if(*awesomeconf->client_sel)
        maximize((*awesomeconf->client_sel)->x,
                 si[awesomeconf->screen].y_org,
                 (*awesomeconf->client_sel)->w,
                 si[awesomeconf->screen].height - 2 * awesomeconf->borderpx,
                 awesomeconf);
    p_delete(&si);
}


void
uicb_togglehorizontalmax(awesome_config *awesomeconf,
                         const char *arg __attribute__ ((unused)))
{
    ScreenInfo *si = get_screen_info(awesomeconf->display, awesomeconf->screen, &awesomeconf->statusbar);

    if(*awesomeconf->client_sel)
        maximize(si[awesomeconf->screen].x_org,
                 (*awesomeconf->client_sel)->y,
                 si[awesomeconf->screen].height - 2 * awesomeconf->borderpx,
                 (*awesomeconf->client_sel)->h,
                 awesomeconf);
    p_delete(&si);
}

void
uicb_zoom(awesome_config *awesomeconf,
          const char *arg __attribute__ ((unused)))
{
    Client *c, *sel = *awesomeconf->client_sel;

    if(!*awesomeconf->client_sel)
        return;

    client_detach(awesomeconf->clients, sel);
    client_attach(awesomeconf->clients, sel);

    /* reattach tabbed windows just after the master one */
    for(c = sel->tab.next; c; c = c->tab.next)
        client_reattach_after(sel, c);

    focus(sel, True, awesomeconf);
    arrange(awesomeconf);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
