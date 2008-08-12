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

#include <xcb/xcb.h>

#include "common/swindow.h"
#include "common/xutil.h"

/** Create a simple window.
 * \param conn Connection ref.
 * \param phys_screen Physical screen number.
 * \param x x coordinate.
 * \param y y coordinate.
 * \param w Width.
 * \param h Height.
 * \param border_width Window border width.
 * \return A pointer to a newly allocated simple window, which must be deleted
 *         with simplewindow_delete().
 */
simple_window_t *
simplewindow_new(xcb_connection_t *conn, int phys_screen, int x, int y,
                 unsigned int w, unsigned int h,
                 unsigned int border_width)
{
    simple_window_t *sw;
    xcb_screen_t *s = xutil_screen_get(conn, phys_screen);
    uint32_t create_win_val[3];
    const uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
    const uint32_t gc_values[2] = { s->black_pixel, s->white_pixel };

    sw = p_new(simple_window_t, 1);

    sw->geometry.x = x;
    sw->geometry.y = y;
    sw->geometry.width = w;
    sw->geometry.height = h;
    sw->connection = conn;
    sw->phys_screen = phys_screen;

    create_win_val[0] = XCB_BACK_PIXMAP_PARENT_RELATIVE;
    create_win_val[1] = 1;
    create_win_val[2] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_EXPOSURE;

    sw->window = xcb_generate_id(conn);
    xcb_create_window(conn, s->root_depth, sw->window, s->root, x, y, w, h,
                      border_width, XCB_COPY_FROM_PARENT, s->root_visual,
                      XCB_CW_BACK_PIXMAP | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      create_win_val);

    sw->pixmap = xcb_generate_id(conn);
    xcb_create_pixmap(conn, s->root_depth, sw->pixmap, s->root, w, h);

    /* The default GC is just a newly created associated to the root
     * window */
    sw->gc = xcb_generate_id(sw->connection);
    xcb_create_gc(sw->connection, sw->gc, s->root, gc_mask, gc_values);

    sw->border_width = border_width;

    return sw;
}

/** Move a simple window.
 * \param sw The simple window to move.
 * \param x New x coordinate.
 * \param y New y coordinate.
 */
void
simplewindow_move(simple_window_t *sw, int x, int y)
{
    const uint32_t move_win_vals[] = { x, y };

    sw->geometry.x = x;
    sw->geometry.y = y;
    xcb_configure_window(sw->connection, sw->window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                         move_win_vals);
}

/** Resize a simple window.
 * \param sw The simple_window_t to resize.
 * \param w New width.
 * \param h New height.
 */
void
simplewindow_resize(simple_window_t *sw, int w, int h)
{
    xcb_screen_t *s = xutil_screen_get(sw->connection, sw->phys_screen);
    uint32_t resize_win_vals[2];
    xcb_pixmap_t d;

    if(w > 0 && h > 0 && (sw->geometry.width != w || sw->geometry.height != h))
    {
        sw->geometry.width = resize_win_vals[0] = w;
        sw->geometry.height = resize_win_vals[1] = h;
        d = sw->pixmap;
        sw->pixmap = xcb_generate_id(sw->connection);
        xcb_create_pixmap(sw->connection, s->root_depth, sw->pixmap, s->root, w, h);
        xcb_configure_window(sw->connection, sw->window,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             resize_win_vals);
        xcb_free_pixmap(sw->connection, d);
    }
}

/** Move and resize a window in one call.
 * \param sw The simple window to move and resize.
 * \param x The new x coordinate.
 * \param y The new y coordinate.
 * \param w The new width.
 * \param h The new height.
 */
void
simplewindow_moveresize(simple_window_t *sw, int x, int y, int w, int h)
{
    uint32_t moveresize_win_vals[4], mask_vals = 0;
    xcb_pixmap_t d;
    xcb_screen_t *s = xutil_screen_get(sw->connection, sw->phys_screen);

    if(sw->geometry.x != x || sw->geometry.y != y)
    {
        sw->geometry.x = moveresize_win_vals[0] = x;
        sw->geometry.y = moveresize_win_vals[1] = y;
        mask_vals |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    }

    if(sw->geometry.width != w || sw->geometry.height != h)
    {
        if(mask_vals)
        {
            sw->geometry.width = moveresize_win_vals[2] = w;
            sw->geometry.height = moveresize_win_vals[3] = h;
        }
        else
        {
            sw->geometry.width = moveresize_win_vals[0] = w;
            sw->geometry.height = moveresize_win_vals[1] = h;
        }
        mask_vals |= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        d = sw->pixmap;
        sw->pixmap = xcb_generate_id(sw->connection);
        xcb_create_pixmap(sw->connection, s->root_depth, sw->pixmap, s->root, w, h);
        xcb_free_pixmap(sw->connection, d);
    }

    xcb_configure_window(sw->connection, sw->window, mask_vals, moveresize_win_vals);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
