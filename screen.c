/*
 * screen.c - screen management
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

#include "util.h"
#include "screen.h"
#include "tag.h"
#include "layout.h"

/** Get screens info
 * \param disp Display ref
 * \param screen Screen number
 * \param statusbar statusbar
 * \return ScreenInfo struct array with all screens info
 */
ScreenInfo *
get_screen_info(Display *disp, int screen, Statusbar *statusbar)
{
    int i, screen_number = 0;
    ScreenInfo *si;

    if(XineramaIsActive(disp))
        si = XineramaQueryScreens(disp, &screen_number);
    else
    {
        /* emulate Xinerama info but only fill the screen we want */
        si = p_new(ScreenInfo, screen + 1);
        si[screen].width = DisplayWidth(disp, screen);
        si[screen].height = DisplayHeight(disp, screen);
        si[screen].x_org = 0;
        si[screen].y_org = 0;
        screen_number = screen + 1;
    }

    if(statusbar)
        for(i = 0; i < screen_number; i++)
        {
            if(statusbar->position == BarTop
               || statusbar->position == BarBot)
                si[i].height -= statusbar->height;
            if(statusbar->position == BarTop)
                si[i].y_org += statusbar->height;
        }

    return si;
}

/** Get display info
 * \param disp Display ref
 * \param screen Screen number
 * \param statusbar the statusbar
 * \return ScreenInfo struct pointer with all display info
 */
ScreenInfo *
get_display_info(Display *disp, int screen, Statusbar *statusbar)
{
    ScreenInfo *si;

    si = p_new(ScreenInfo, 1);

    si->x_org = 0;
    si->y_org = statusbar && statusbar->position == BarTop ? statusbar->height : 0;
    si->width = DisplayWidth(disp, screen);
    si->height = DisplayHeight(disp, screen) -
        (statusbar && (statusbar->position == BarTop || statusbar->position == BarBot) ? statusbar->height : 0);

    return si;
}

/** Return the Xinerama screen number where the coordinates belongs to
 * \param disp Display ref
 * \param x x coordinate of the window
 * \param y y coordinate of the window
 * \return screen number or DefaultScreen of disp on no match
 */
int
get_screen_bycoord(Display *disp, int x, int y)
{
    ScreenInfo *si;
    int i;

    /* don't waste our time */
    if(!XineramaIsActive(disp))
        return DefaultScreen(disp);

    si = get_screen_info(disp, 0, NULL);

    for(i = 0; i < get_screen_count(disp); i++)
        if(x >= si[i].x_org && x < si[i].x_org + si[i].width
           && y >= si[i].y_org && y < si[i].y_org + si[i].height)
        {
            XFree(si);
            return i;
        }

    XFree(si);
    return DefaultScreen(disp);
}

/** Return the actual screen count
 * \param disp Display ref
 * \return the number of screen available
 */
int
get_screen_count(Display *disp)
{
    int screen_number;

    if(XineramaIsActive(disp))
        XineramaQueryScreens(disp, &screen_number);
    else
        return ScreenCount(disp);

    return screen_number;
}

/** This returns the real X screen number for a logical
 * screen if Xinerama is active.
 * \param disp Display ref
 * \param screen the logical screen
 * \return the X screen
 */
int
get_phys_screen(Display *disp, int screen)
{
    if(XineramaIsActive(disp))
        return DefaultScreen(disp);
    return screen;
}

/** Move a client to a virtual screen
 * \param c the client
 * \param acf_new the awesome_config for the new screen
 * \param doresize set to True if we also move the client to the new x_org and
 *         y_org of the new screen
 */
void
move_client_to_screen(Client *c, awesome_config *acf_new, Bool doresize)
{
    int i;
    ScreenInfo *si;

    c->screen = acf_new->screen;
    p_realloc(&c->tags, acf_new->ntags);
    for(i = 0; i < acf_new->ntags; i++)
        c->tags[i] = acf_new->tags[i].selected;

    si = get_screen_info(c->display, c->screen, &acf_new->statusbar);
    c->rx = si[c->screen].x_org;
    c->ry = si[c->screen].y_org;
    if(doresize)
        resize(c, c->rx, c->ry, c->rw, c->rh, acf_new, True);
    XFree(si);
}

/** Move mouse pointer to x_org and y_xorg of specified screen
 * \param disp display ref
 * \param screen screen number
 */
static void
move_mouse_pointer_to_screen(Display *disp, int screen)
{
    if(XineramaIsActive(disp))
    {
        ScreenInfo *si = get_screen_info(disp, screen, NULL);
        XWarpPointer(disp, None, DefaultRootWindow(disp), 0, 0, 0, 0, si[screen].x_org, si[screen].y_org);
        XFree(si);
    }
    else
        XWarpPointer(disp, None, RootWindow(disp, screen), 0, 0, 0, 0, 0, 0);
}

void
uicb_focusnextscreen(awesome_config * awesomeconf,
                     const char *arg __attribute__ ((unused)))
{
    Client *c;
    int next_screen = awesomeconf->screen + 1 >= get_screen_count(awesomeconf->display) ? 0 : awesomeconf->screen + 1;

    for(c = *awesomeconf->clients; c && !isvisible(c, next_screen, awesomeconf[next_screen - awesomeconf->screen].tags, awesomeconf[next_screen - awesomeconf->screen].ntags); c = c->next);
    if(c)
    {
        focus(c->display, c, True, &awesomeconf[next_screen - awesomeconf->screen]);
        restack(c->display, &awesomeconf[next_screen - awesomeconf->screen]);
    }
    move_mouse_pointer_to_screen(awesomeconf->display, next_screen);
}

void
uicb_focusprevscreen(awesome_config * awesomeconf,
                     const char *arg __attribute__ ((unused)))
{
    Client *c;
    int prev_screen = awesomeconf->screen - 1 < 0 ? get_screen_count(awesomeconf->display) - 1 : awesomeconf->screen - 1;

    for(c = *awesomeconf->clients; c && !isvisible(c, prev_screen, awesomeconf[prev_screen - awesomeconf->screen].tags, awesomeconf[prev_screen - awesomeconf->screen].ntags); c = c->next);
    if(c)
    {
        focus(c->display, c, True, &awesomeconf[prev_screen - awesomeconf->screen]);
        restack(c->display, &awesomeconf[prev_screen - awesomeconf->screen]);
    }
    move_mouse_pointer_to_screen(awesomeconf->display, prev_screen);
}

/** Move client to a virtual screen (if Xinerama is active)
 * \param awesomeconf awesome config
 * \param arg screen number
 * \ingroup ui_callback
 */
void
uicb_movetoscreen(awesome_config * awesomeconf,
                  const char *arg)
{
    int new_screen, prev_screen;

    if(!*awesomeconf->client_sel || !XineramaIsActive(awesomeconf->display))
        return;

    if(arg)
        new_screen = compute_new_value_from_arg(arg, (*awesomeconf->client_sel)->screen);
    else
        new_screen = (*awesomeconf->client_sel)->screen + 1;

    if(new_screen >= get_screen_count(awesomeconf->display))
        new_screen = 0;
    else if(new_screen < 0)
        new_screen = get_screen_count(awesomeconf->display) - 1;

    prev_screen = (*awesomeconf->client_sel)->screen;
    move_client_to_screen(*awesomeconf->client_sel, &awesomeconf[new_screen - awesomeconf->screen], True);
    move_mouse_pointer_to_screen(awesomeconf->display, new_screen);
    arrange(awesomeconf->display, &awesomeconf[prev_screen - awesomeconf->screen]);
    arrange(awesomeconf->display, &awesomeconf[new_screen - awesomeconf->screen]);
}
