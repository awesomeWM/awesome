/*
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2010 Uli Schlachter <psychon@znc.in>
 * Copyright ©      2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#include "drawin.h"

#include <xcb/shape.h>

#include "common/xcursor.h"
#include "objects/drawin.h"
#include "objects/client.h"
#include "x11/xwindow.h"
#include "x11/ewmh.h"

extern struct drawable_impl drawable_impl;

xcb_window_t x11_get_xcb_window(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;
    return x11_drawin->window;
}

/** Get a drawin by its window.
 * \param win The window id.
 * \return A drawin if found, NULL otherwise.
 */
drawin_t *x11_get_drawin_by_window(xcb_window_t win)
{
    foreach(w, globalconf.drawins)
    {
        struct x11_drawin *x11_drawin = (*w)->impl_data;
        if(x11_drawin->window == win)
            return *w;
    }
    return NULL;
}

void x11_drawin_allocate(struct drawin_t *drawin)
{
    if (!drawin->impl_data)
    {
        drawin->impl_data = calloc(1, sizeof(struct x11_drawin));
    }

    struct x11_drawin *x11_drawin = drawin->impl_data;
    xcb_screen_t *s = globalconf.screen;

    x11_drawin->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth,
            x11_drawin->window, s->root,
            drawin->geometry.x, drawin->geometry.y,
            drawin->geometry.width, drawin->geometry.height,
            drawin->border_width, XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
            XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY
            | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP
            | XCB_CW_CURSOR,
            (const uint32_t [])
            {
                drawin->border_color.pixel,
                    XCB_GRAVITY_NORTH_WEST,
                    1,
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                    | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_ENTER_WINDOW
                    | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                    | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS
                    | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE
                    | XCB_EVENT_MASK_PROPERTY_CHANGE,
                    globalconf.default_cmap,
                    xcursor_new(globalconf.cursor_ctx,
                            xcursor_font_fromstr(drawin->cursor))
                    });
    xwindow_set_class_instance(x11_drawin->window);
    xwindow_set_name_static(x11_drawin->window, "Awesome drawin");

    /* Set the right properties */
    ewmh_update_window_type(x11_drawin->window,
            window_translate_type(drawin->type));
    ewmh_update_strut(x11_drawin->window, &drawin->strut);
}

void x11_drawin_cleanup(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    if(x11_drawin && x11_drawin->window)
    {
        /* Make sure we don't accidentally kill the systray window */
        x11_drawin_systray_kickout(drawin);
        xcb_destroy_window(globalconf.connection, x11_drawin->window);
        x11_drawin->window = XCB_NONE;

        free(x11_drawin);
        drawin->impl_data = NULL;
    }
}

void x11_drawin_moveresize(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    client_ignore_enterleave_events();
    xcb_configure_window(globalconf.connection, x11_drawin->window,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
            | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
            (const uint32_t [])
            {
                drawin->geometry.x,
                    drawin->geometry.y,
                    drawin->geometry.width,
                    drawin->geometry.height
                    });
    client_restore_enterleave_events();
}

/** Refresh the window content by copying its pixmap data to its window.
 * \param drawin The drawin to refresh.
 * \param x The copy starting point x component.
 * \param y The copy starting point y component.
 * \param w The copy width from the x component.
 * \param h The copy height from the y component.
 */
void x11_drawin_refresh(struct drawin_t *drawin, int16_t x, int16_t y,
        uint16_t w, uint16_t h)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    if (!drawin->drawable
            || !drawable_impl.get_pixmap(drawin->drawable)
            || !drawin->drawable->refreshed)
        return;

    /* Make sure it really has the size it should have */
    drawin_apply_moveresize(drawin);

    /* Make cairo do all pending drawing */
    cairo_surface_flush(drawin->drawable->surface);
    xcb_copy_area(globalconf.connection, drawable_impl.get_pixmap(drawin->drawable),
            x11_drawin->window, globalconf.gc, x, y, x, y,
            w, h);
}

void x11_drawin_map(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    xcb_map_window(globalconf.connection, x11_drawin->window);
}

void x11_drawin_unmap(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    xcb_unmap_window(globalconf.connection, x11_drawin->window);
}

double x11_drawin_get_opacity(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    return xwindow_get_opacity(x11_drawin->window);
}

void x11_drawin_set_cursor(struct drawin_t *drawin, const char *cursor_name)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    uint16_t cursor_font = xcursor_font_fromstr(cursor_name);
    xcb_cursor_t cursor = xcursor_new(globalconf.cursor_ctx, cursor_font);
    p_delete(&drawin->cursor);
    drawin->cursor = a_strdup(cursor_name);
    xwindow_set_cursor(x11_drawin->window, cursor);
}

void x11_drawin_systray_kickout(struct drawin_t *drawin)
{
    if(globalconf.systray.parent == drawin)
    {
        /* Who! Check that we're not deleting a drawin with a systray, because it
         * may be its parent. If so, we reparent to root before, otherwise it will
         * hurt very much. */
        xcb_reparent_window(globalconf.connection,
                globalconf.systray.window,
                globalconf.screen->root,
                -512, -512);

        globalconf.systray.parent = NULL;
    }
}

void x11_drawin_set_shape_bounding(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    xwindow_set_shape(x11_drawin->window,
            drawin->geometry.width + 2*drawin->border_width,
            drawin->geometry.height + 2*drawin->border_width,
            XCB_SHAPE_SK_BOUNDING, surface, -drawin->border_width);
}

void x11_drawin_set_shape_input(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    xwindow_set_shape(x11_drawin->window,
            drawin->geometry.width + 2*drawin->border_width,
            drawin->geometry.height + 2*drawin->border_width,
            XCB_SHAPE_SK_INPUT, surface, -drawin->border_width);
}

void x11_drawin_set_shape_clip(struct drawin_t *drawin,
        cairo_surface_t *surface)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    xwindow_set_shape(x11_drawin->window, drawin->geometry.width,
            drawin->geometry.height, XCB_SHAPE_SK_CLIP, surface, 0);
}

cairo_surface_t *x11_drawin_get_shape_clip(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    return xwindow_get_shape(x11_drawin->window, XCB_SHAPE_SK_CLIP);
}

cairo_surface_t *x11_drawin_get_shape_bounding(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    return xwindow_get_shape(x11_drawin->window, XCB_SHAPE_SK_BOUNDING);
}

cairo_surface_t *x11_drawin_get_shape_input(struct drawin_t *drawin)
{
    struct x11_drawin *x11_drawin = drawin->impl_data;

    return xwindow_get_shape(x11_drawin->window, XCB_SHAPE_SK_INPUT);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
