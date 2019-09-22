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

#include "root.h"
#include "globalconf.h"
#include "xwindow.h"
#include "common/atoms-extern.h"
#include "common/xutil.h"

#include <cairo-xcb.h>
#include <xcb/xtest.h>
#include <xcb/xcb_aux.h>

static void
root_set_wallpaper_pixmap(xcb_connection_t *c, xcb_pixmap_t p)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    const xcb_screen_t *screen = globalconf.screen;

    /* We now have the pattern painted to the pixmap p. Now turn p into the root
     * window's background pixmap.
     */
    xcb_change_window_attributes(c, screen->root, XCB_CW_BACK_PIXMAP, &p);
    xcb_clear_area(c, 0, screen->root, 0, 0, 0, 0);

    prop_c = xcb_get_property_unchecked(c, false,
            screen->root, ESETROOT_PMAP_ID, XCB_ATOM_PIXMAP, 0, 1);

    /* Theoretically, this should be enough to set the wallpaper. However, to
     * make pseudo-transparency work, clients need a way to get the wallpaper.
     * You can't query a window's back pixmap, so properties are (ab)used.
     */
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, screen->root, _XROOTPMAP_ID, XCB_ATOM_PIXMAP, 32, 1, &p);
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, screen->root, ESETROOT_PMAP_ID, XCB_ATOM_PIXMAP, 32, 1, &p);

    /* Now make sure that the old wallpaper is freed (but only do this for ESETROOT_PMAP_ID) */
    prop_r = xcb_get_property_reply(c, prop_c, NULL);
    if (prop_r && prop_r->value_len)
    {
        xcb_pixmap_t *rootpix = xcb_get_property_value(prop_r);
        if (rootpix)
            xcb_kill_client(c, *rootpix);
    }
    p_delete(&prop_r);
}

int x11_set_wallpaper(cairo_pattern_t *pattern)
{
    lua_State *L = globalconf_get_lua_State();
    xcb_connection_t *c = xcb_connect(NULL, NULL);
    xcb_pixmap_t p = xcb_generate_id(c);
    /* globalconf.connection should be connected to the same X11 server, so we
     * can just use the info from that other connection.
     */
    const xcb_screen_t *screen = globalconf.screen;
    uint16_t width = screen->width_in_pixels;
    uint16_t height = screen->height_in_pixels;
    bool result = false;
    cairo_surface_t *surface;
    cairo_t *cr;

    if (xcb_connection_has_error(c))
        goto disconnect;

    /* Create a pixmap and make sure it is already created, because we are going
     * to use it from the other X11 connection (Juggling with X11 connections
     * is a really, really bad idea).
     */
    xcb_create_pixmap(c, screen->root_depth, p, screen->root, width, height);
    xcb_aux_sync(c);

    /* Now paint to the picture from the main connection so that cairo sees that
     * it can tell the X server to copy between the (possible) old pixmap and
     * the new one directly and doesn't need GetImage and PutImage.
     */
    surface = cairo_xcb_surface_create(globalconf.connection, p, draw_default_visual(screen), width, height);
    cr = cairo_create(surface);
    /* Paint the pattern to the surface */
    cairo_set_source(cr, pattern);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_flush(surface);
    xcb_aux_sync(globalconf.connection);

    /* Change the wallpaper, without sending us a PropertyNotify event */
    xcb_grab_server(globalconf.connection);
    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 (uint32_t[]) { 0 });
    root_set_wallpaper_pixmap(globalconf.connection, p);
    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 ROOT_WINDOW_EVENT_MASK);
    xutil_ungrab_server(globalconf.connection);

    /* Make sure our pixmap is not destroyed when we disconnect. */
    xcb_set_close_down_mode(c, XCB_CLOSE_DOWN_RETAIN_PERMANENT);

    /* Tell Lua that the wallpaper changed */
    cairo_surface_destroy(globalconf.wallpaper);
    globalconf.wallpaper = surface;
    signal_object_emit(L, &global_signals, "wallpaper_changed", 0);

    result = true;
disconnect:
    xcb_aux_sync(c);
    xcb_disconnect(c);
    return result;
}

void x11_update_wallpaper(void)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;
    xcb_pixmap_t *rootpix;

    cairo_surface_destroy(globalconf.wallpaper);
    globalconf.wallpaper = NULL;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false,
            globalconf.screen->root, _XROOTPMAP_ID, XCB_ATOM_PIXMAP, 0, 1);
    prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL);

    if (!prop_r || !prop_r->value_len)
    {
        p_delete(&prop_r);
        return;
    }

    rootpix = xcb_get_property_value(prop_r);
    if (!rootpix)
    {
        p_delete(&prop_r);
        return;
    }

    geom_c = xcb_get_geometry_unchecked(globalconf.connection, *rootpix);
    geom_r = xcb_get_geometry_reply(globalconf.connection, geom_c, NULL);
    if (!geom_r)
    {
        p_delete(&prop_r);
        return;
    }

    /* Only the default visual makes sense, so just the default depth */
    if (geom_r->depth != draw_visual_depth(globalconf.screen, globalconf.default_visual->visual_id))
        warn("Got a pixmap with depth %d, but the default depth is %d, continuing anyway",
                geom_r->depth, draw_visual_depth(globalconf.screen, globalconf.default_visual->visual_id));

    globalconf.wallpaper = cairo_xcb_surface_create(globalconf.connection,
                                                    *rootpix,
                                                    globalconf.default_visual,
                                                    geom_r->width,
                                                    geom_r->height);

    p_delete(&prop_r);
    p_delete(&geom_r);
}

void x11_grab_keys(void)
{
    xcb_screen_t *s = globalconf.screen;
    xwindow_grabkeys(s->root, &globalconf.keys);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
