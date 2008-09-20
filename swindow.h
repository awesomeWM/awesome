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

#ifndef AWESOME_SWINDOW_H
#define AWESOME_SWINDOW_H

#include "draw.h"
#include "common/util.h"

/** A simple window. */
typedef struct simple_window_t
{
    /** The window object. */
    xcb_window_t window;
    /** The pixmap copied to the window object. */
    xcb_pixmap_t pixmap;
    /** The graphic context. */
    xcb_gcontext_t gc;
    /** The window geometry. */
    area_t geometry;
    /** The window border */
    struct
    {
        /** The window border width */
        int width;
        /** The window border color */
        xcolor_t color;
    } border;
    /** Draw context */
    draw_context_t ctx;
    /** Position */
    position_t position;
} simple_window_t;

void simplewindow_init(simple_window_t *s,
                       int, int, int, unsigned int, unsigned int, unsigned int,
                       position_t, const xcolor_t *, const xcolor_t *);

void simplewindow_wipe(simple_window_t *);

void simplewindow_move(simple_window_t *, int, int);
void simplewindow_resize(simple_window_t *, int, int);
void simplewindow_moveresize(simple_window_t *, int, int, int, int);
void simplewindow_refresh_pixmap_partial(simple_window_t *, int16_t, int16_t, uint16_t, uint16_t);
void simplewindow_border_width_set(simple_window_t *, uint32_t);
void simplewindow_border_color_set(simple_window_t *, const xcolor_t *);

/** Refresh the window content by copying its pixmap data to its window.
 * \param sw The simple window to refresh.
 */
static inline void
simplewindow_refresh_pixmap(simple_window_t *sw)
{
    simplewindow_refresh_pixmap_partial(sw, 0, 0, sw->geometry.width, sw->geometry.height);
}


#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
