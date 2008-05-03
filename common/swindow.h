/*
 * swindow.h - simple window handling functions header
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

#ifndef AWESOME_COMMON_SWINDOW_H
#define AWESOME_COMMON_SWINDOW_H

#include "common/draw.h"

/** A simple window. */
typedef struct simple_window_t
{
    /** The display connection. */
    xcb_connection_t *connection;
    /** The physical screen number the window is on. */
    int phys_screen;
    /** The window object. */
    xcb_window_t window;
    /** The drawable copied to the window object. */
    xcb_drawable_t drawable;
    /** The graphic context. */
    xcb_gcontext_t gc;
    /** The window geometry. */
    area_t geometry;
    /** The window border width */
    int border_width;
} simple_window_t;

simple_window_t * simplewindow_new(xcb_connection_t *, int, int, int, unsigned int, unsigned int, unsigned int);
void simplewindow_delete(simple_window_t **);
void simplewindow_move(simple_window_t *, int, int);
void simplewindow_resize(simple_window_t *, unsigned int, unsigned int);

/** Move and resize a window in one call.
 * \param sw The simple window to move and resize.
 * \param x The new x coordinate.
 * \param y The new y coordinate.
 * \param w The new width.
 * \param h The new height.
 */
static inline void
simplewindow_move_resize(simple_window_t *sw, int x, int y,
                         unsigned int w, unsigned int h)
{
  simplewindow_move(sw, x, y);
  simplewindow_resize(sw, w, h);
}

/** Refresh the window content by copying its drawable data to its window.
 * \param sw The simple window to refresh.
 */
static inline void
simplewindow_refresh_drawable(simple_window_t *sw)
{
    xcb_copy_area(sw->connection, sw->drawable,
                  sw->window, sw->gc, 0, 0, 0, 0,
                  sw->geometry.width,
                  sw->geometry.height);
}

/** Set a simple window border width.
 * \param sw The simple window to change border width.
 * \param border_width The border width in pixel.
 */
static inline void
simplewindow_border_width_set(simple_window_t *sw, uint32_t border_width)
{
    xcb_configure_window(sw->connection, sw->window, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         &border_width);
    sw->border_width = border_width;
}

/** Set a simple window border color.
 * \param sw The simple window to change border width.
 * \param border_color The border color.
 */
static inline void
simplewindow_border_color_set(simple_window_t *sw, xcolor_t *color)
{
    xcb_change_window_attributes(sw->connection, sw->window,
                                 XCB_CW_BORDER_PIXEL, &color->pixel);
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
