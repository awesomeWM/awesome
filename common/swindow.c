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
#include <xcb/xcb_aux.h>

#include "common/swindow.h"

/** Create a simple window
 * \param conn Connection ref
 * \param phys_screen physical screen id
 * \param x x coordinate
 * \param y y coordinate
 * \param w width
 * \param h height
 * \param border_width window's border width
 * \return pointer to a simple_window_t
 */
simple_window_t *
simplewindow_new(xcb_connection_t *conn, int phys_screen, int x, int y,
                 unsigned int w, unsigned int h,
                 unsigned int border_width)
{
    simple_window_t *sw;
    xcb_screen_t *s = xcb_aux_get_screen(conn, phys_screen);
    uint32_t create_win_val[3];
    const uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
    const uint32_t gc_values[2] = { s->black_pixel, s->white_pixel };
    /* The default GC is just a newly created associated to the root
     * window */
    xcb_drawable_t gc_draw = s->root;

    sw = p_new(simple_window_t, 1);

    sw->geometry.x = x;
    sw->geometry.y = y;
    sw->geometry.width = w;
    sw->geometry.height = h;
    sw->connection = conn;
    sw->phys_screen = phys_screen;

    create_win_val[0] = XCB_BACK_PIXMAP_PARENT_RELATIVE;
    create_win_val[1] = 1;
    create_win_val[2] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW |
        XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE;

    sw->window = xcb_generate_id(conn);
    xcb_create_window(conn, s->root_depth, sw->window, s->root, x, y, w, h,
                      border_width, XCB_COPY_FROM_PARENT, s->root_visual,
                      XCB_CW_BACK_PIXMAP | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      create_win_val);

    sw->drawable = xcb_generate_id(conn);
    xcb_create_pixmap(conn, s->root_depth, sw->drawable, s->root, w, h);

    sw->gc = xcb_generate_id(sw->connection);
    xcb_create_gc(sw->connection, sw->gc, gc_draw, gc_mask, gc_values);

    return sw;
}

/** Destroy a simple window and all its resources
 * \param sw the simple_window_t to delete
 */
void
simplewindow_delete(simple_window_t **sw)
{
    xcb_destroy_window((*sw)->connection, (*sw)->window);
    xcb_free_pixmap((*sw)->connection, (*sw)->drawable);
    xcb_free_gc((*sw)->connection, (*sw)->gc);
    p_delete(sw);
}

/** Move a simple window
 * \param sw the simple_window_t to move
 * \param x x coordinate
 * \param y y coordinate
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

/** Resize a simple window
 * \param sw the simple_window_t to resize
 * \param w new width
 * \param h new height
 */
void
simplewindow_resize(simple_window_t *sw, unsigned int w, unsigned int h)
{
    xcb_screen_t *s = xcb_aux_get_screen(sw->connection, sw->phys_screen);
    const uint32_t resize_win_vals[] = { w, h };

    sw->geometry.width = w;
    sw->geometry.height = h;
    xcb_free_pixmap(sw->connection, sw->drawable);
    sw->drawable = xcb_generate_id(sw->connection);
    xcb_create_pixmap(sw->connection, s->root_depth, sw->drawable, s->root, w, h);
    xcb_configure_window(sw->connection, sw->window,
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         resize_win_vals);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
