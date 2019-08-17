/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
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
#ifndef AWESOME_X11_DRAWIN_H
#define AWESOME_X11_DRAWIN_H

#include "objects/drawin.h"

struct x11_drawin
{
    /** The X window number */
    xcb_window_t window;
    /** The frame window, might be XCB_NONE */
    xcb_window_t frame_window;
};

xcb_window_t x11_get_xcb_window(struct drawin_t *drawin);
drawin_t *x11_get_drawin_by_window(xcb_window_t win);

void x11_drawin_allocate(struct drawin_t *drawin);
void x11_drawin_cleanup(struct drawin_t *drawin);
void x11_drawin_moveresize(struct drawin_t *drawin);
void x11_drawin_refresh(struct drawin_t *drawin, int16_t x, int16_t y,
        uint16_t w, uint16_t h);
void x11_drawin_map(struct drawin_t *drawin);
void x11_drawin_unmap(struct drawin_t *drawin);
double x11_drawin_get_opacity(struct drawin_t *drawin);
void x11_drawin_set_cursor(struct drawin_t *drawin, const char *cursor);
void x11_drawin_systray_kickout(struct drawin_t *drawin);

void x11_drawin_set_shape_bounding(struct drawin_t *drawin,
        cairo_surface_t *surface);
void x11_drawin_set_shape_clip(struct drawin_t *drawin,
        cairo_surface_t *surface);
void x11_drawin_set_shape_input(struct drawin_t *drawin,
        cairo_surface_t *surface);
cairo_surface_t *x11_drawin_get_shape_bounding(struct drawin_t *drawin);
cairo_surface_t *x11_drawin_get_shape_clip(struct drawin_t *drawin);
cairo_surface_t *x11_drawin_get_shape_input(struct drawin_t *drawin);

#endif // AWESOME_X11_DRAWIN_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
