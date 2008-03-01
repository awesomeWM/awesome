/*
 * swindow.c - simple window handling functions
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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


#include "common/swindow.h"
#include "common/util.h"

/** Create a simple window
 * \param disp Display ref
 * \param phys_screen physical screen id
 * \param x x coordinate
 * \param y y coordinate
 * \param w width
 * \param h height
 * \param border_width window's border width
 * \return pointer to a SimpleWindow
 */
SimpleWindow *
simplewindow_new(Display *disp, int phys_screen, int x, int y,
                 unsigned int w, unsigned int h,
                 unsigned int border_width)
{
    XSetWindowAttributes wa;
    SimpleWindow *sw;

    sw = p_new(SimpleWindow, 1);

    sw->geometry.x = x;
    sw->geometry.y = y;
    sw->geometry.width = w;
    sw->geometry.height = h;
    sw->display = disp;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    sw->window = XCreateWindow(disp,
                               RootWindow(disp, phys_screen),
                               x, y, w, h,
                               border_width,
                               DefaultDepth(disp, phys_screen),
                               CopyFromParent,
                               DefaultVisual(disp, phys_screen),
                               CWOverrideRedirect | CWBackPixmap | CWEventMask,
                               &wa);

    sw->drawable = XCreatePixmap(disp,
                                 RootWindow(disp, phys_screen),
                                 w, h,
                                 DefaultDepth(disp, phys_screen));
    return sw;
}

/** Destroy a simple window and all its resources
 * \param sw the SimpleWindow to delete
 */
void
simplewindow_delete(SimpleWindow *sw)
{
    XDestroyWindow(sw->display, sw->window);
    XFreePixmap(sw->display, sw->drawable);
    p_delete(&sw);
}

/** Move a simple window
 * \param sw the SimpleWindow to move
 * \param x x coordinate
 * \param y y coordinate
 * \return status
 */
int
simplewindow_move(SimpleWindow *sw, int x, int y)
{
    sw->geometry.x = x;
    sw->geometry.y = y;
    return XMoveWindow(sw->display, sw->window, x, y);
}

/** Refresh the window content
 * \param sw the SimpleWindow to refresh
 * \param phys_screen physical screen id
 * \return status
 */
int
simplewindow_refresh_drawable(SimpleWindow *sw, int phys_screen)
{
    return XCopyArea(sw->display, sw->drawable,
                     sw->window,
                     DefaultGC(sw->display, phys_screen), 0, 0,
                     sw->geometry.width,
                     sw->geometry.height,
                     0, 0);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
