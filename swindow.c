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

#include <math.h>

#include <xcb/xcb.h>

#include "structs.h"
#include "swindow.h"
#include "draw.h"
#include "common/xutil.h"

static void
simplewindow_draw_context_update(simple_window_t *sw, xcb_screen_t *s)
{
    xcolor_t fg = sw->ctx.fg, bg = sw->ctx.bg;
    int phys_screen = sw->ctx.phys_screen;

    draw_context_wipe(&sw->ctx);

    /* update draw context */
    switch(sw->orientation)
    {
      case South:
      case North:
        /* we need a new pixmap this way [     ] to render */
        sw->ctx.pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection,
                          s->root_depth,
                          sw->ctx.pixmap, s->root,
                          sw->geometry.height, sw->geometries.internal.width);
        draw_context_init(&sw->ctx, phys_screen,
                          sw->geometry.height, sw->geometries.internal.width,
                          sw->ctx.pixmap, &fg, &bg);
        break;
      case East:
        draw_context_init(&sw->ctx, phys_screen,
                          sw->geometries.internal.width, sw->geometries.internal.height,
                          sw->pixmap, &fg, &bg);
        break;
    }
}

/** Initialize a simple window.
 * \param sw The simple window to initialize.
 * \param phys_screen Physical screen number.
 * \param geometry Window geometry.
 * \param border_width Window border width.
 * \param orientation The rendering orientation.
 * \param bg Default foreground color.
 * \param bg Default background color.
 */
void
simplewindow_init(simple_window_t *sw,
                  int phys_screen,
                  area_t geometry,
                  uint16_t border_width,
                  orientation_t orientation,
                  const xcolor_t *fg, const xcolor_t *bg)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);
    uint32_t create_win_val[3];
    const uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
    const uint32_t gc_values[2] = { s->black_pixel, s->white_pixel };

    sw->geometry.x = geometry.x;
    sw->geometry.y = geometry.y;
    sw->geometry.width = geometry.width;
    sw->geometry.height = geometry.height;
    sw->border.width = border_width;

    /* The real protocol window. */
    sw->geometries.internal.x = geometry.x;
    sw->geometries.internal.y = geometry.y;
    sw->geometries.internal.width = geometry.width;
    sw->geometries.internal.height = geometry.height;

    sw->orientation = orientation;
    sw->ctx.fg = *fg;
    sw->ctx.bg = *bg;

    create_win_val[0] = XCB_BACK_PIXMAP_PARENT_RELATIVE;
    create_win_val[1] = 1;
    create_win_val[2] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE;

    sw->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, s->root_depth, sw->window, s->root,
                      sw->geometries.internal.x, sw->geometries.internal.y, sw->geometries.internal.width, sw->geometries.internal.height,
                      border_width, XCB_COPY_FROM_PARENT, s->root_visual,
                      XCB_CW_BACK_PIXMAP | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
                      create_win_val);

    sw->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, s->root_depth, sw->pixmap, s->root,
                      sw->geometries.internal.width, sw->geometries.internal.height);

    sw->ctx.phys_screen = phys_screen;
    simplewindow_draw_context_update(sw, s);

    /* The default GC is just a newly created associated to the root window */
    sw->gc = xcb_generate_id(globalconf.connection);
    xcb_create_gc(globalconf.connection, sw->gc, s->root, gc_mask, gc_values);
}

/** Destroy all resources of a simple window.
 * \param sw The simple_window_t to wipe.
 */
void
simplewindow_wipe(simple_window_t *sw)
{
    if(sw->window)
    {
        xcb_destroy_window(globalconf.connection, sw->window);
        sw->window = XCB_NONE;
    }
    if(sw->pixmap)
    {
        xcb_free_pixmap(globalconf.connection, sw->pixmap);
        sw->pixmap = XCB_NONE;
    }
    if(sw->gc)
    {
        xcb_free_gc(globalconf.connection, sw->gc);
        sw->gc = XCB_NONE;
    }
    draw_context_wipe(&sw->ctx);
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

    if(x != sw->geometries.internal.x || y != sw->geometries.internal.y)
    {
        sw->geometry.x = sw->geometries.internal.x = x;
        sw->geometry.y = sw->geometries.internal.y = y;
        xcb_configure_window(globalconf.connection, sw->window,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             move_win_vals);
    }
}

/** Resize a simple window.
 * \param sw The simple_window_t to resize.
 * \param w New width.
 * \param h New height.
 */
void
simplewindow_resize(simple_window_t *sw, int w, int h)
{
    int iw = w - 2 * sw->border.width;
    int ih = h - 2 * sw->border.width;

    if(iw > 0 && ih > 0 &&
        (sw->geometries.internal.width != iw || sw->geometries.internal.height != ih))
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection, sw->ctx.phys_screen);
        uint32_t resize_win_vals[2];

        sw->geometries.internal.width = resize_win_vals[0] = iw;
        sw->geometries.internal.height = resize_win_vals[1] = ih;
        sw->geometry.width = w;
        sw->geometry.height = h;
        xcb_free_pixmap(globalconf.connection, sw->pixmap);
        /* orientation != East */
        if(sw->pixmap != sw->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, sw->ctx.pixmap);
        sw->pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth, sw->pixmap, s->root, iw, ih);
        xcb_configure_window(globalconf.connection, sw->window,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             resize_win_vals);
        simplewindow_draw_context_update(sw, s);
    }
}

/** Move and resize a window in one call.
 * \param sw The simple window to move and resize.
 * \param geom The new geometry.
 */
void
simplewindow_moveresize(simple_window_t *sw, area_t geom)
{
    uint32_t moveresize_win_vals[4], mask_vals = 0;
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, sw->ctx.phys_screen);

    area_t geom_internal = geom;
    geom_internal.width -= 2 * sw->border.width;
    geom_internal.height -= 2* sw->border.width;

    if(sw->geometries.internal.x != geom_internal.x || sw->geometries.internal.y != geom_internal.y)
    {
        sw->geometries.internal.x = moveresize_win_vals[0] = geom_internal.x;
        sw->geometries.internal.y = moveresize_win_vals[1] = geom_internal.y;
        mask_vals |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
    }

    if(sw->geometry.width != geom.width || sw->geometry.height != geom.height)
    {
        if(mask_vals)
        {
            sw->geometries.internal.width = moveresize_win_vals[2] = geom_internal.width;
            sw->geometries.internal.height = moveresize_win_vals[3] = geom_internal.height;
        }
        else
        {
            sw->geometries.internal.width = moveresize_win_vals[0] = geom_internal.width;
            sw->geometries.internal.height = moveresize_win_vals[1] = geom_internal.height;
        }
        mask_vals |= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        xcb_free_pixmap(globalconf.connection, sw->pixmap);
        /* orientation != East */
        if(sw->pixmap != sw->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, sw->ctx.pixmap);
        sw->pixmap = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth, sw->pixmap, s->root, geom_internal.width, geom_internal.height);
        simplewindow_draw_context_update(sw, s);
    }

    /* Also save geometry including border. */
    sw->geometry = geom;

    xcb_configure_window(globalconf.connection, sw->window, mask_vals, moveresize_win_vals);
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param sw The simple window to refresh.
 */
void
simplewindow_refresh_pixmap_partial(simple_window_t *sw,
                                    int16_t x, int16_t y,
                                    uint16_t w, uint16_t h)
{
    xcb_copy_area(globalconf.connection, sw->pixmap,
                  sw->window, sw->gc, x, y, x, y,
                  w, h);
}

/** Set a simple window border width.
 * \param sw The simple window to change border width.
 * \param border_width The border width in pixel.
 */
void
simplewindow_border_width_set(simple_window_t *sw, uint32_t border_width)
{
    xcb_configure_window(globalconf.connection, sw->window, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         &border_width);
    sw->border.width = border_width;
}

/** Set a simple window border color.
 * \param sw The simple window to change border width.
 * \param color The border color.
 */
void
simplewindow_border_color_set(simple_window_t *sw, const xcolor_t *color)
{
    xcb_change_window_attributes(globalconf.connection, sw->window,
                                 XCB_CW_BORDER_PIXEL, &color->pixel);
    sw->border.color = *color;
}

/** Set simple window orientation.
 * \param sw The simple window.
 * \param o The new orientation
 */
void
simplewindow_orientation_set(simple_window_t *sw, orientation_t o)
{
    if(o != sw->orientation)
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection, sw->ctx.phys_screen);
        sw->orientation = o;
        /* orientation != East */
        if(sw->pixmap != sw->ctx.pixmap)
            xcb_free_pixmap(globalconf.connection, sw->ctx.pixmap);
        simplewindow_draw_context_update(sw, s);
    }
}

/** Set simple window cursor.
 * \param sw The simple window.
 * \param c The cursor.
 */
void
simplewindow_cursor_set(simple_window_t *sw, xcb_cursor_t c)
{
    if(sw->window)
    {
        const uint32_t change_win_vals[] = { c };
        xcb_change_window_attributes(globalconf.connection, sw->window, XCB_CW_CURSOR, change_win_vals);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
