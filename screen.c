/*
 * screen.c - screen management
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include <X11/extensions/Xinerama.h>

#include "screen.h"
#include "tag.h"
#include "focus.h"
#include "client.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

/** Get screens info
 * \param screen Screen number
 * \param statusbar statusbar
 * \param padding Padding
 * \return Area
 */
Area
screen_get_area(int screen, Statusbar *statusbar, Padding *padding)
{
    Area area;
    Statusbar *sb;

    area = globalconf.screens[screen].geometry;

    /* make padding corrections */
    if(padding)
    {
        area.x += padding->left;
        area.y += padding->top;
        area.width -= padding->left + padding->right;
        area.height -= padding->top + padding->bottom;
    }

    for(sb = statusbar; sb; sb = sb->next)
        switch(sb->position)
        {
          case Top:
            area.y += sb->height;
          case Bottom:
            area.height -= sb->height;
            break;
          case Left:
            area.x += sb->height;
          case Right:
            area.width -= sb->height;
            break;
          case Off:
            break;
        }

    return area;
}

/** Get display info
 * \param screen Screen number
 * \param statusbar the statusbar
 * \param padding Padding
 * \return Area
 */
Area
get_display_area(int screen, Statusbar *statusbar, Padding *padding)
{
    Area area = { 0, 0, 0, 0, NULL };
    Statusbar *sb;

    area.width = DisplayWidth(globalconf.display, screen);
    area.height = DisplayHeight(globalconf.display, screen);

    for(sb = statusbar; sb; sb = sb->next)
    {
        area.y += sb->position == Top ? sb->height : 0;
        area.height -= (sb->position == Top || sb->position == Bottom) ? sb->height : 0;
    }

    /* make padding corrections */
    if(padding)
    {
            area.x += padding->left;
            area.y += padding->top;
            area.width -= padding->left + padding->right;
            area.height -= padding->top + padding->bottom;
    }
    return area;
}

/** Return the Xinerama screen number where the coordinates belongs to
 * \param x x coordinate of the window
 * \param y y coordinate of the window
 * \return screen number or DefaultScreen of disp on no match
 */
int
screen_get_bycoord(int x, int y)
{
    int i;
    Area area;

    /* don't waste our time */
    if(!XineramaIsActive(globalconf.display))
        return DefaultScreen(globalconf.display);

    for(i = 0; i < globalconf.nscreen; i++)
    {
        area = screen_get_area(i, NULL, NULL);
        if((x < 0 || (x >= area.x && x < area.x + area.width))
           && (y < 0 || (y >= area.y && y < area.y + area.height)))
            return i;
    }
    return DefaultScreen(globalconf.display);
}


static inline Area
screen_xsi_to_area(XineramaScreenInfo si)
{
    Area a;

    a.x = si.x_org;
    a.y = si.y_org;
    a.width = si.width;
    a.height = si.height;
    a.next = NULL;

    return a;
}

void
screen_build_screens(void)
{
    XineramaScreenInfo *si;
    int xinerama_screen_number, screen, screen_to_test;
    Bool drop;

    if(XineramaIsActive(globalconf.display))
    {
        si = XineramaQueryScreens(globalconf.display, &xinerama_screen_number);
        globalconf.screens = p_new(VirtScreen, xinerama_screen_number);
        globalconf.nscreen = 0;

        /* now check if screens overlaps (same x,y): if so, we take only the biggest one */
        for(screen = 0; screen < xinerama_screen_number; screen++)
        {
            drop = False;
            for(screen_to_test = 0; screen_to_test < globalconf.nscreen; screen_to_test++)
                if(si[screen].x_org == globalconf.screens[screen_to_test].geometry.x
                   && si[screen].y_org == globalconf.screens[screen_to_test].geometry.y)
                    {
                        /* we already have a screen for this area, just check if
                         * it's not bigger and drop it */
                        drop = True;
                        globalconf.screens[screen_to_test].geometry.width =
                            MAX(si[screen].width, si[screen_to_test].width);
                        globalconf.screens[screen_to_test].geometry.height =
                            MAX(si[screen].height, si[screen_to_test].height);
                    }
            if(!drop)
                globalconf.screens[globalconf.nscreen++].geometry = screen_xsi_to_area(si[screen]);
        }

        /* realloc smaller if xinerama_screen_number != screen registered */
        if(xinerama_screen_number != globalconf.nscreen)
        {
            VirtScreen *newscreens = p_new(VirtScreen, globalconf.nscreen);
            memcpy(newscreens, globalconf.screens, globalconf.nscreen * sizeof(VirtScreen));
            p_delete(&globalconf.screens);
            globalconf.screens = newscreens;
        }

        XFree(si);
    }
    else
    {
        globalconf.nscreen = ScreenCount(globalconf.display);
        globalconf.screens = p_new(VirtScreen, globalconf.nscreen);
        for(screen = 0; screen < globalconf.nscreen; screen++)
        {
            globalconf.screens[screen].geometry.x = 0;
            globalconf.screens[screen].geometry.y = 0;
            globalconf.screens[screen].geometry.width =
                DisplayWidth(globalconf.display, screen);
            globalconf.screens[screen].geometry.height =
                DisplayHeight(globalconf.display, screen);
        }
    }

}

/** This returns the real X screen number for a logical
 * screen if Xinerama is active.
 * \param screen the logical screen
 * \return the X screen
 */
int
get_phys_screen(int screen)
{
    if(XineramaIsActive(globalconf.display))
        return DefaultScreen(globalconf.display);
    return screen;
}

/** Move a client to a virtual screen
 * \param c the client
 * \param new_screen The destinatiuon screen
 * \param doresize set to True if we also move the client to the new x and
 *        y of the new screen
 */
void
move_client_to_screen(Client *c, int new_screen, Bool doresize)
{
    Tag *tag;
    int old_screen = c->screen;
    Area from, to;

    for(tag = globalconf.screens[old_screen].tags; tag; tag = tag->next)
        untag_client(c, tag);

    c->screen = new_screen;

    /* tag client with new screen tags */
    tag_client_with_current_selected(c);

    /* resize the windows if it's floating */
    if(doresize && old_screen != c->screen)
    {
        Area new_geometry, new_f_geometry;
        new_f_geometry = c->f_geometry;

        to = screen_get_area(c->screen, NULL, NULL);
        from = screen_get_area(old_screen, NULL, NULL);

        /* compute new coords in new screen */
        new_f_geometry.x = (c->f_geometry.x - from.x) + to.x;
        new_f_geometry.y = (c->f_geometry.y - from.y) + to.y;

        /* check that new coords are still in the screen */
        if(new_f_geometry.width > to.width)
            new_f_geometry.width = to.width;
        if(new_f_geometry.height > to.height)
            new_f_geometry.height = to.height;
        if(new_f_geometry.x + new_f_geometry.width >= to.x + to.width)
            new_f_geometry.x = to.x + to.width - new_f_geometry.width - 2 * c->border;
        if(new_f_geometry.y + new_f_geometry.height >= to.y + to.height)
            new_f_geometry.y = to.y + to.height - new_f_geometry.height - 2 * c->border;

        if(c->ismax)
        {
            new_geometry = c->geometry;

            /* compute new coords in new screen */
            new_geometry.x = (c->geometry.x - from.x) + to.x;
            new_geometry.y = (c->geometry.y - from.y) + to.y;

            /* check that new coords are still in the screen */
            if(new_geometry.width > to.width)
                new_geometry.width = to.width;
            if(new_geometry.height > to.height)
                new_geometry.height = to.height;
            if(new_geometry.x + new_geometry.width >= to.x + to.width)
                new_geometry.x = to.x + to.width - new_geometry.width - 2 * c->border;
            if(new_geometry.y + new_geometry.height >= to.y + to.height)
                new_geometry.y = to.y + to.height - new_geometry.height - 2 * c->border;

            /* compute new coords for max in new screen */
            c->m_geometry.x = (c->m_geometry.x - from.x) + to.x;
            c->m_geometry.y = (c->m_geometry.y - from.y) + to.y;

            /* check that new coords are still in the screen */
            if(c->m_geometry.width > to.width)
                c->m_geometry.width = to.width;
            if(c->m_geometry.height > to.height)
                c->m_geometry.height = to.height;
            if(c->m_geometry.x + c->m_geometry.width >= to.x + to.width)
                c->m_geometry.x = to.x + to.width - c->m_geometry.width - 2 * c->border;
            if(c->m_geometry.y + c->m_geometry.height >= to.y + to.height)
                c->m_geometry.y = to.y + to.height - c->m_geometry.height - 2 * c->border;

            client_resize(c, new_geometry, False);
        }
        /* if floating, move to this new coords */
        else if(c->isfloating)
            client_resize(c, new_f_geometry, False);
        /* otherwise just register them */
        else
        {
            c->f_geometry = new_f_geometry;
            globalconf.screens[old_screen].need_arrange = True;
            globalconf.screens[c->screen].need_arrange = True;
        }
    }
}

/** Move mouse pointer to x_org and y_xorg of specified screen
 * \param screen screen number
 */
static void
move_mouse_pointer_to_screen(int screen)
{
    if(XineramaIsActive(globalconf.display))
    {
        Area area = screen_get_area(screen, NULL, NULL);
        XWarpPointer(globalconf.display,
                     None,
                     DefaultRootWindow(globalconf.display),
                     0, 0, 0, 0, area.x, area.y);
    }
    else
        XWarpPointer(globalconf.display,
                     None,
                     RootWindow(globalconf.display, screen),
                     0, 0, 0, 0, 0, 0);
}


/** Switch focus to a specified screen
 * \param screen Screen ID
 * \param arg screen number
 * \ingroup ui_callback
 */
void
uicb_screen_focus(int screen, char *arg)
{
    int new_screen;

    if(arg)
        new_screen = compute_new_value_from_arg(arg, screen);
    else
        new_screen = screen + 1;

    if (new_screen < 0)
        new_screen = globalconf.nscreen - 1;
    if (new_screen > (globalconf.nscreen - 1))
        new_screen = 0;

    client_focus(NULL, new_screen, True);

    move_mouse_pointer_to_screen(new_screen);
}

/** Move client to a virtual screen (if Xinerama is active)
 * \param screen Screen ID
 * \param arg screen number
 * \ingroup ui_callback
 */
void
uicb_client_movetoscreen(int screen __attribute__ ((unused)), char *arg)
{
    int new_screen, prev_screen;
    Client *sel = globalconf.focus->client;

    if(!sel || !XineramaIsActive(globalconf.display))
        return;

    if(arg)
        new_screen = compute_new_value_from_arg(arg, sel->screen);
    else
        new_screen = sel->screen + 1;

    if(new_screen >= globalconf.nscreen)
        new_screen = 0;
    else if(new_screen < 0)
        new_screen = globalconf.nscreen - 1;

    prev_screen = sel->screen;
    move_client_to_screen(sel, new_screen, True);
    move_mouse_pointer_to_screen(new_screen);
    client_focus(sel, sel->screen, True);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
