/*
 * mouse.c - mouse managing
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

#include <math.h>

#include "common/tokenize.h"
#include "mouse.h"
#include "screen.h"
#include "tag.h"
#include "client.h"
#include "titlebar.h"
#include "wibox.h"
#include "layouts/floating.h"
#include "layouts/tile.h"
#include "layouts/magnifier.h"

#define MOUSEMASK      (XCB_EVENT_MASK_BUTTON_PRESS \
                        | XCB_EVENT_MASK_BUTTON_RELEASE \
                        | XCB_EVENT_MASK_POINTER_MOTION)

extern awesome_t globalconf;

DO_LUA_NEW(extern, button_t, button, "button", button_ref)
DO_LUA_GC(button_t, button, "button", button_unref)
DO_LUA_EQ(button_t, button, "button")

/** Define corners. */
typedef enum
{
    AutoCorner,
    TopRightCorner,
    TopLeftCorner,
    BottomLeftCorner,
    BottomRightCorner
} corner_t;

/** Convert a corner name into a corner type.
 * \param str A string.
 * \param len String length.
 * \return A corner type.
 */
static corner_t
a_strtocorner(const char *str, size_t len)
{
    switch (a_tokenize(str, len))
    {
      case A_TK_BOTTOMRIGHT: return BottomRightCorner;
      case A_TK_BOTTOMLEFT:  return BottomLeftCorner;
      case A_TK_TOPLEFT:     return TopLeftCorner;
      case A_TK_TOPRIGHT:    return TopRightCorner;
      default:               return AutoCorner;
    }
}

/** Snap an area to the outside of an area.
 * \param geometry geometry of the area to snap
 * \param snap_geometry geometry of snapping area
 * \param snap snap trigger in pixel
 * \return snapped geometry
 */
static area_t
mouse_snapclienttogeometry_outside(area_t geometry, area_t snap_geometry, int snap)
{
    if(geometry.x < snap + snap_geometry.x + snap_geometry.width
       && geometry.x > snap_geometry.x + snap_geometry.width)
        geometry.x = snap_geometry.x + snap_geometry.width;
    else if(geometry.x + geometry.width < snap_geometry.x
            && geometry.x + geometry.width > snap_geometry.x - snap)
        geometry.x = snap_geometry.x - geometry.width;

    if(geometry.y < snap + snap_geometry.y + snap_geometry.height
       && geometry.y > snap_geometry.y + snap_geometry.height)
        geometry.y = snap_geometry.y + snap_geometry.height;
    else if(geometry.y + geometry.height < snap_geometry.y
            && geometry.y + geometry.height > snap_geometry.y - snap)
        geometry.y = snap_geometry.y - geometry.height;

    return geometry;
}

/** Snap an area to the inside of an area.
 * \param geometry geometry of the area to snap
 * \param snap_geometry geometry of snapping area
 * \param snap snap trigger in pixel
 * \return snapped geometry
 */
static area_t
mouse_snapclienttogeometry_inside(area_t geometry, area_t snap_geometry, int snap)
{
    if(abs(geometry.x) < snap + snap_geometry.x && geometry.x > snap_geometry.x)
        geometry.x = snap_geometry.x;
    else if(abs((snap_geometry.x + snap_geometry.width) - (geometry.x + geometry.width))
            < snap)
        geometry.x = snap_geometry.x + snap_geometry.width - geometry.width;
    if(abs(geometry.y) < snap + snap_geometry.y && geometry.y > snap_geometry.y)
        geometry.y = snap_geometry.y;
    else if(abs((snap_geometry.y + snap_geometry.height) - (geometry.y + geometry.height))
            < snap)
        geometry.y = snap_geometry.y + snap_geometry.height - geometry.height;

    return geometry;
}

/** Snap a client with a future geometry to the screen and other clients.
 * \param c The client.
 * \param geometry Geometry the client will get.
 * \param snap The maximum distance in pixels to trigger a "snap".
 * \return Geometry to set to the client.
 */
static area_t
mouse_snapclient(client_t *c, area_t geometry, int snap)
{
    client_t *snapper;
    area_t snapper_geometry;
    area_t screen_geometry =
        screen_area_get(c->screen,
                        &globalconf.screens[c->screen].wiboxes,
                        &globalconf.screens[c->screen].padding,
                        false);

    area_t screen_geometry_barless =
        screen_area_get(c->screen,
                        NULL,
                        &globalconf.screens[c->screen].padding,
                        false);

    geometry = titlebar_geometry_add(c->titlebar, c->border, geometry);

    geometry =
        mouse_snapclienttogeometry_inside(geometry, screen_geometry, snap);

    geometry =
        mouse_snapclienttogeometry_inside(geometry, screen_geometry_barless, snap);

    for(snapper = globalconf.clients; snapper; snapper = snapper->next)
        if(snapper != c && client_isvisible(snapper, c->screen))
        {
            snapper_geometry = snapper->geometry;
            snapper_geometry = titlebar_geometry_add(snapper->titlebar, snapper->border, snapper_geometry);
            geometry =
                mouse_snapclienttogeometry_outside(geometry,
                                                   snapper_geometry,
                                                   snap);
        }

    return titlebar_geometry_remove(c->titlebar, c->border, geometry);
}

/** Set coordinates to a corner of an area.
 *
 * \param a The area.
 * \param[in,out] x The x coordinate.
 * \param[in,out] y The y coordinate.
 * \param corner The corner to snap to.
 * \return The corner the coordinates have been set to. If corner != AutoCorner
 * this is always equal to \em corner.
 *
 * \todo rename/move this is still awkward and might be useful somewhere else.
 */
static corner_t
mouse_snap_to_corner(area_t a, int *x, int *y, corner_t corner)
{
    int top, bottom, left, right;

    top = AREA_TOP(a);
    bottom = AREA_BOTTOM(a);
    left = AREA_LEFT(a);
    right = AREA_RIGHT(a);

    /* figure out the nearser corner */
    if(corner == AutoCorner)
    {
        if(abs(top - *y) < abs(bottom - *y))
        {
            if(abs(left - *x) < abs(right - *x))
                corner = TopLeftCorner;
            else
                corner = TopRightCorner;
        }
        else
        {
            if(abs(left - *x) < abs(right - *x))
                corner = BottomLeftCorner;
            else
                corner = BottomRightCorner;
        }
    }

    switch(corner)
    {
        case TopRightCorner:
            *x = right;
            *y = top;
            break;

        case TopLeftCorner:
            *x = left;
            *y = top;
            break;

        case BottomLeftCorner:
            *x = left;
            *y = bottom;
            break;

        case BottomRightCorner:
            *x = right;
            *y = bottom;
            break;

        default:
            break;
    }

    return corner;
}

/** Redraw the infobox.
 * \param sw The simple window.
 * \param geometry The geometry to use for the box.
 * \param border The client border size.
 */
static void
mouse_infobox_draw(simple_window_t *sw, area_t geometry, int border)
{
    area_t draw_geometry = { 0, 0, sw->ctx.width, sw->ctx.height };
    char size[64];
    size_t len;

    len = snprintf(size, sizeof(size), "<text align=\"center\"/>%dx%d+%d+%d",
                   geometry.width, geometry.height, geometry.x, geometry.y);
    draw_rectangle(&sw->ctx, draw_geometry, 1.0, true, &globalconf.colors.bg);
    draw_text(&sw->ctx, globalconf.font, draw_geometry, size, len, NULL);
    simplewindow_move(sw,
                      geometry.x + ((2 * border + geometry.width) - sw->geometry.width) / 2,
                      geometry.y + ((2 * border + geometry.height) - sw->geometry.height) / 2);
    simplewindow_refresh_pixmap(sw);
    xcb_flush(globalconf.connection);
}

#define MOUSE_INFOBOX_STRING_DEFAULT "0000x0000+0000+0000"

/** Initialize the infobox window.
 * \param sw The simple window to init.
 * \param phys_screen Physical screen number.
 * \param border Border size of the client.
 * \param geometry Client geometry.
 * \return The simple window.
 */
static void
mouse_infobox_new(simple_window_t *sw, int phys_screen, int border, area_t geometry)
{
    area_t geom;
    draw_parser_data_t pdata;

    draw_parser_data_init(&pdata);

    geom = draw_text_extents(globalconf.default_screen,
                             globalconf.font,
                             MOUSE_INFOBOX_STRING_DEFAULT,
                             sizeof(MOUSE_INFOBOX_STRING_DEFAULT)-1,
                             &pdata);
    geom.x = geometry.x + ((2 * border + geometry.width) - geom.width) / 2;
    geom.y = geometry.y + ((2 * border + geometry.height) - geom.height) / 2;

    p_clear(sw, 1);
    simplewindow_init(sw, phys_screen, geom, 0, East,
                      &globalconf.colors.fg, &globalconf.colors.bg);

    xcb_map_window(globalconf.connection, sw->window);
    mouse_infobox_draw(sw, geometry, border);

    draw_parser_data_wipe(&pdata);
}

/** Get the pointer position.
 * \param window The window to get position on.
 * \param x will be set to the Pointer-x-coordinate relative to window
 * \param y will be set to the Pointer-y-coordinate relative to window
 * \param mask will be set to the current buttons state
 * \return true on success, false if an error occured
 **/
static bool
mouse_query_pointer(xcb_window_t window, int *x, int *y, uint16_t *mask)
{
    xcb_query_pointer_cookie_t query_ptr_c;
    xcb_query_pointer_reply_t *query_ptr_r;

    query_ptr_c = xcb_query_pointer_unchecked(globalconf.connection, window);
    query_ptr_r = xcb_query_pointer_reply(globalconf.connection, query_ptr_c, NULL);

    if(!query_ptr_r || !query_ptr_r->same_screen)
        return false;

    *x = query_ptr_r->win_x;
    *y = query_ptr_r->win_y;
    if (mask)
        *mask = query_ptr_r->mask;

    p_delete(&query_ptr_r);

    return true;
}

/** Get the pointer position on the screen.
 * \param screen This will be set to the screen number the mouse is on.
 * \param x This will be set to the Pointer-x-coordinate relative to window.
 * \param y This will be set to the Pointer-y-coordinate relative to window.
 * \param mask This will be set to the current buttons state.
 * \return True on success, false if an error occured.
 */
static bool
mouse_query_pointer_root(int *s, int *x, int *y, uint16_t *mask)
{
    for(int screen = 0;
        screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen++)
    {
        xcb_window_t root = xutil_screen_get(globalconf.connection, screen)->root;

        if(mouse_query_pointer(root, x, y, mask))
        {
            *s = screen;
            return true;
        }
    }
    return false;
}

/** Grab the Pointer.
 * \param window The window grabbed.
 * \param cursor The cursor to display (see struct.h CurNormal, CurResize, etc).
 * \return True on success, false if an error occured.
 */
static bool
mouse_grab_pointer(xcb_window_t window, size_t cursor)
{
    xcb_grab_pointer_cookie_t grab_ptr_c;
    xcb_grab_pointer_reply_t *grab_ptr_r;

    if(cursor >= CurLast)
        cursor = CurNormal;

    grab_ptr_c = xcb_grab_pointer_unchecked(globalconf.connection, false, window,
                                            MOUSEMASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                            window, globalconf.cursor[cursor], XCB_CURRENT_TIME);
    grab_ptr_r = xcb_grab_pointer_reply(globalconf.connection, grab_ptr_c, NULL);

    if(!grab_ptr_r)
        return false;

    p_delete(&grab_ptr_r);

    return true;
}

/** Ungrab the Pointer
 */
static inline void
mouse_ungrab_pointer(void)
{
    xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);
}

/** Set the pointer position.
 * \param window The destination window.
 * \param x X-coordinate inside window.
 * \param y Y-coordinate inside window.
 */
static inline void
mouse_warp_pointer(xcb_window_t window, int x, int y)
{
    xcb_warp_pointer(globalconf.connection, XCB_NONE, window,
                     0, 0, 0, 0, x, y );
}

/** Utility function to help with mouse-dragging
 *
 * \param x set to x-coordinate of the last event on return
 * \param y set to y-coordinate of the last event on return
 * \return true if an motion event was received
 *         false if an button release event was received
 */
static bool
mouse_track_mouse_drag(int *x, int *y)
{
    xcb_generic_event_t *ev;
    xcb_motion_notify_event_t *ev_motion;
    xcb_button_release_event_t *ev_button;

    while(true)
        while((ev = xcb_wait_for_event(globalconf.connection)))
            switch((ev->response_type & 0x7F))
            {

                case XCB_MOTION_NOTIFY:
                    ev_motion = (xcb_motion_notify_event_t*) ev;
                    *x = ev_motion->event_x;
                    *y = ev_motion->event_y;
                    p_delete(&ev);
                    return true;

                case XCB_BUTTON_RELEASE:
                    ev_button = (xcb_button_release_event_t*) ev;
                    *x = ev_button->event_x;
                    *y = ev_button->event_y;
                    p_delete(&ev);
                    return false;

                default:
                    xcb_event_handle(&globalconf.evenths, ev);
                    p_delete(&ev);
                    break;
            }
}

/** Get the client that contains the pointer.
 *
 * \return The client that contains the pointer or NULL.
 */
static client_t *
mouse_get_client_under_pointer(int phys_screen)
{
    xcb_window_t root;
    xcb_query_pointer_cookie_t query_ptr_c;
    xcb_query_pointer_reply_t *query_ptr_r;
    client_t *c = NULL;

    root = xutil_screen_get(globalconf.connection, phys_screen)->root;

    query_ptr_c = xcb_query_pointer_unchecked(globalconf.connection, root);
    query_ptr_r = xcb_query_pointer_reply(globalconf.connection, query_ptr_c, NULL);

    if(query_ptr_r)
    {
        c = client_getbywin(query_ptr_r->child);
        p_delete(&query_ptr_r);
    }

    return c;
}

/** Move the focused window with the mouse.
 * \param c The client.
 * \param snap The maximum distance in pixels to trigger a "snap".
 * \param infobox Enable or disable the infobox.
 */
static void
mouse_client_move(client_t *c, int snap, bool infobox)
{
    /* current mouse postion */
    int mouse_x, mouse_y;
    /* last mouse position */
    int last_x = 0, last_y = 0;
    /* current layout */
    layout_t *layout;
    /* the infobox */
    simple_window_t sw;
    /* the root window */
    xcb_window_t root;

    layout = layout_get_current(c->screen);
    root = xutil_screen_get(globalconf.connection, c->phys_screen)->root;

    /* get current pointer position */
    mouse_query_pointer(root, &last_x, &last_y, NULL);

    /* grab pointer */
    if(c->isfullscreen
       || c->type == WINDOW_TYPE_DESKTOP
       || c->type == WINDOW_TYPE_SPLASH
       || c->type == WINDOW_TYPE_DOCK
       || !mouse_grab_pointer(root, CurMove))
        return;

    if(infobox && (client_isfloating(c) || layout == layout_floating))
        mouse_infobox_new(&sw, c->phys_screen, c->border, c->geometry);
    else
        infobox = false;

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
        if(client_isfloating(c) || layout == layout_floating)
        {
            area_t geometry;

            /* calc new geometry */
            geometry = c->geometry;
            geometry.x += (mouse_x - last_x);
            geometry.y += (mouse_y - last_y);

            /* snap and move */
            geometry = mouse_snapclient(c, geometry, snap);
            c->ismoving = true;
            client_resize(c, geometry, false);
            xcb_flush(globalconf.connection);
            c->ismoving = false;

            /* draw the infobox */
            if(infobox)
                mouse_infobox_draw(&sw, c->geometry, c->border);

            /* refresh live */
            wibox_refresh();
            xcb_flush(globalconf.connection);

            /* keep track */
            last_x = mouse_x;
            last_y = mouse_y;
        }
        else
        {
            int newscreen;
            client_t *target;

            /* client moved to another screen? */
            newscreen = screen_getbycoord(c->screen, mouse_x, mouse_y);
            if(newscreen != c->screen)
            {
                screen_client_moveto(c, newscreen, true, true);
                globalconf.screens[c->screen].need_arrange = true;
                globalconf.screens[newscreen].need_arrange = true;
                layout_refresh();
                wibox_refresh();
                xcb_flush(globalconf.connection);
            }

            /* find client to swap with */
            target = mouse_get_client_under_pointer(c->phys_screen);

            /* swap position */
            if(target && target != c && !target->isfloating)
            {
                client_list_swap(&globalconf.clients, c, target);
                globalconf.screens[c->screen].need_arrange = true;
                luaA_dofunction(globalconf.L, globalconf.hooks.clients, 0, 0);
                layout_refresh();
                wibox_refresh();
                xcb_flush(globalconf.connection);
            }
        }

    /* ungrab pointer */
    xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);

    /* free the infobox */
    if(infobox)
        simplewindow_wipe(&sw);
}


/** Resize a floating client with the mouse.
 * \param c The client to resize.
 * \param corner The corner to resize with.
 * \param infobox Enable or disable the infobox.
 */
static void
mouse_client_resize_floating(client_t *c, corner_t corner, bool infobox)
{
    xcb_screen_t *screen;
    /* one corner of the client has a fixed position */
    int fixed_x, fixed_y;
    /* the other is moved with the mouse */
    int mouse_x = 0, mouse_y = 0;
    /* the infobox */
    simple_window_t sw;
    size_t cursor = CurResize;
    int top, bottom, left, right;

    screen = xutil_screen_get(globalconf.connection, c->phys_screen);

    /* get current mouse position */
    mouse_query_pointer(screen->root, &mouse_x, &mouse_y, NULL);

    top = c->geometry.y;
    bottom = top + c->geometry.height;
    left = c->geometry.x;
    right = left + c->geometry.width;

    /* figure out which corner to move */
    corner = mouse_snap_to_corner(c->geometry, &mouse_x, &mouse_y, corner);

    /* the opposite corner is fixed */
    fixed_x = (mouse_x == left) ? right : left;
    fixed_y = (mouse_y == top) ? bottom : top;

    /* select cursor */
    switch(corner)
    {
      default:
          cursor = CurTopLeft;
          break;
        case TopRightCorner:
          cursor = CurTopRight;
          break;
        case BottomLeftCorner:
          cursor = CurBotLeft;
          break;
        case BottomRightCorner:
          cursor = CurBotRight;
          break;
    }

    /* grab the pointer */
    if(!mouse_grab_pointer(screen->root, cursor))
        return;

    /* set pointer to the moveable corner */
    mouse_warp_pointer(screen->root, mouse_x, mouse_y);

    /* create the infobox */
    if(infobox)
        mouse_infobox_new(&sw, c->phys_screen, c->border, c->geometry);

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        /* new client geometry */
        area_t geo = { .x = MIN(fixed_x, mouse_x),
                       .y = MIN(fixed_y, mouse_y),
                       .width = (MAX(fixed_x, mouse_x) - MIN(fixed_x, mouse_x)),
                       .height = (MAX(fixed_y, mouse_y) - MIN(fixed_y, mouse_y)) };
        /* new moveable corner */
        corner_t new_corner;

        if(mouse_x == AREA_LEFT(geo))
            new_corner = (mouse_y == AREA_TOP(geo)) ? TopLeftCorner : BottomLeftCorner;
        else
            new_corner = (mouse_y == AREA_TOP(geo)) ? TopRightCorner : BottomRightCorner;

        /* update cursor */
        if(corner != new_corner)
        {
            corner = new_corner;

            switch(corner)
            {
                default: cursor = CurTopLeft; break;
                case TopRightCorner: cursor = CurTopRight; break;
                case BottomLeftCorner: cursor = CurBotLeft; break;
                case BottomRightCorner: cursor = CurBotRight; break;
            }

            xcb_change_active_pointer_grab(globalconf.connection, globalconf.cursor[cursor],
                                           XCB_CURRENT_TIME, MOUSEMASK);
        }

        if(c->hassizehints && c->honorsizehints)
        {
            int dx, dy;

            /* apply size hints */
            geo = client_geometry_hints(c, geo);

            /* get the nonmoveable corner back onto fixed_x,fixed_y */
            switch(corner)
            {
                default /* TopLeftCorner */:
                    dx = fixed_x - AREA_RIGHT(geo);
                    dy = fixed_y - AREA_BOTTOM(geo);
                    break;
                case TopRightCorner:
                    dx = fixed_x - AREA_LEFT(geo);
                    dy = fixed_y - AREA_BOTTOM(geo);
                    break;
                case BottomRightCorner:
                    dx = fixed_x - AREA_LEFT(geo);
                    dy = fixed_y - AREA_TOP(geo);
                    break;
                case BottomLeftCorner:
                    dx = fixed_x - AREA_RIGHT(geo);
                    dy = fixed_y - AREA_TOP(geo);
                    break;
            }

            geo.x += dx;
            geo.y += dy;
        }

        /* resize the client */
        client_resize(c, geo, false);

        /* refresh live */
        wibox_refresh();
        xcb_flush(globalconf.connection);

        /* draw the infobox */
        if(infobox)
            mouse_infobox_draw(&sw, c->geometry, c->border);
    }

    /* relase pointer */
    mouse_ungrab_pointer();

    /* free the infobox */
    if(infobox)
        simplewindow_wipe(&sw);
}

/** Resize the master column/row of a tiled layout
 * \param c A client on the tag/layout to resize.
 */
static void
mouse_client_resize_tiled(client_t *c)
{
    xcb_screen_t *screen;
    /* screen area modulo wibox */
    area_t area;
    /* current tag */
    tag_t *tag;
    /* current layout */
    layout_t *layout;

    int mouse_x = 0, mouse_y = 0;
    size_t cursor = CurResize;

    screen = xutil_screen_get(globalconf.connection, c->phys_screen);
    tag = tags_get_current(c->screen)[0];
    layout = tag->layout;

    area = screen_area_get(tag->screen,
                           &globalconf.screens[tag->screen].wiboxes,
                           &globalconf.screens[tag->screen].padding,
                           true);

    mouse_query_pointer(screen->root, &mouse_x, &mouse_y, NULL);

    /* select initial pointer position */
    if(layout == layout_tile)
    {
        mouse_x = area.x + area.width * tag->mwfact;
        cursor = CurResizeH;
    }
    else if(layout == layout_tileleft)
    {
        mouse_x = area.x + area.width * (1. - tag->mwfact);
        cursor = CurResizeH;
    }
    else if(layout == layout_tilebottom)
    {
        mouse_y = area.y + area.height * tag->mwfact;
        cursor = CurResizeV;
    }
    else if(layout == layout_tiletop)
    {
        mouse_y = area.y + area.height * (1. - tag->mwfact);
        cursor = CurResizeV;
    }
    else
        return;

    /* grab the pointer */
    if(!mouse_grab_pointer(screen->root, cursor))
        return;

    /* set pointer to the moveable border */
    mouse_warp_pointer(screen->root, mouse_x, mouse_y);

    xcb_flush(globalconf.connection);

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        double mwfact = 0, fact_x, fact_y;

        /* calculate new master / rest ratio */
        fact_x = (double) (mouse_x - area.x) / area.width;
        fact_y = (double) (mouse_y - area.y) / area.height;

        if(layout == layout_tile)
            mwfact = fact_x;
        else if(layout == layout_tileleft)
            mwfact = 1. - fact_x;
        else if(layout == layout_tilebottom)
            mwfact = fact_y;
        else if(layout == layout_tiletop)
            mwfact = 1. - fact_y;

        /* keep mwfact within reasonable bounds */
        mwfact = MIN(MAX( 0.01, mwfact), 0.99);

        /* refresh layout */
        if(fabs(tag->mwfact - mwfact) >= 0.01)
        {
            tag->mwfact = mwfact;
            globalconf.screens[tag->screen].need_arrange = true;
            layout_refresh();
            wibox_refresh();
            xcb_flush(globalconf.connection);
        }
    }

    /* relase pointer */
    mouse_ungrab_pointer();
}

/** Resize the master client in mangifier layout
 * \param c The client to resize.
 * \param infobox Enable or disable the infobox.
 */
static void
mouse_client_resize_magnified(client_t *c, bool infobox)
{
    /* screen area modulo wibox */
    area_t area;
    /* center of area */
    int center_x, center_y;
    /* max. distance from the center */
    double maxdist;
    /* mouse position */
    int mouse_x = 0, mouse_y = 0;
    /* cursor while grabbing */
    size_t cursor = CurResize;
    corner_t corner = AutoCorner;
    /* current tag */
    tag_t *tag;
    /* the infobox */
    simple_window_t sw;
    xcb_window_t root;

    tag = tags_get_current(c->screen)[0];

    root = xutil_screen_get(globalconf.connection, c->phys_screen)->root;

    area = screen_area_get(tag->screen,
                           &globalconf.screens[tag->screen].wiboxes,
                           &globalconf.screens[tag->screen].padding,
                           true);

    center_x = area.x + (round(area.width / 2.));
    center_y = area.y + (round(area.height / 2.));

    maxdist = round(sqrt((area.width*area.width) + (area.height*area.height)) / 2.);

    root = xutil_screen_get(globalconf.connection, c->phys_screen)->root;

    if(!mouse_query_pointer(root, &mouse_x, &mouse_y, NULL))
        return;

    /* select corner */
    corner = mouse_snap_to_corner(c->geometry, &mouse_x, &mouse_y, corner);

    /* select cursor */
    switch(corner)
    {
      default:
          cursor = CurTopLeft;
          break;
        case TopRightCorner:
          cursor = CurTopRight;
          break;
        case BottomLeftCorner:
          cursor = CurBotLeft;
          break;
        case BottomRightCorner:
          cursor = CurBotRight;
          break;
    }

    /* grab pointer */
    if(!mouse_grab_pointer(root, cursor))
        return;

    /* move pointer to corner */
    mouse_warp_pointer(root, mouse_x, mouse_y);

    /* create the infobox */
    if(infobox)
        mouse_infobox_new(&sw, c->phys_screen, c->border, c->geometry);

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        /* \todo keep pointer on screen diagonals */
        double mwfact, dist, dx, dy;

        /* calc distance from the center */
        dx = center_x - mouse_x;
        dy = center_y - mouse_y;
        dist = sqrt((dx * dx) + (dy * dy));

        /* new master/rest ratio */
        mwfact = (dist * dist) / (maxdist * maxdist);

        /* keep mwfact within reasonable bounds */
        mwfact = MIN(MAX( 0.01, mwfact), 0.99);

        /* refresh the layout */
        if(fabs(tag->mwfact - mwfact) >= 0.01)
        {
            tag->mwfact = mwfact;
            globalconf.screens[tag->screen].need_arrange = true;
            layout_refresh();
            wibox_refresh();
            xcb_flush(globalconf.connection);
        }

        /* draw the infobox */
        if(infobox)
            mouse_infobox_draw(&sw, c->geometry, c->border);
    }

    /* ungrab pointer */
    mouse_ungrab_pointer();

    /* free the infobox */
    if(infobox)
        simplewindow_wipe(&sw);
}

/** Resize a client with the mouse.
 * \param c The client to resize.
 * \param corner The corner to use.
 * \param infobox Enable or disable the info box.
 */
static void
mouse_client_resize(client_t *c, corner_t corner, bool infobox)
{
    int n, screen;
    tag_t **curtags;
    layout_t *layout;
    xcb_screen_t *s;

    if(c->isfullscreen
       || c->type == WINDOW_TYPE_DESKTOP
       || c->type == WINDOW_TYPE_SPLASH
       || c->type == WINDOW_TYPE_DOCK)
        return;

    curtags = tags_get_current(c->screen);
    layout = curtags[0]->layout;
    s = xutil_screen_get(globalconf.connection, c->phys_screen);

    /* only handle floating, tiled and magnifier layouts */
    if(layout == layout_floating || client_isfloating(c))
        mouse_client_resize_floating(c, corner, infobox);
    else if(layout == layout_tile || layout == layout_tileleft
            || layout == layout_tilebottom || layout == layout_tiletop)
    {
        screen = c->screen;
        for(n = 0, c = globalconf.clients; c; c = c->next)
            if(IS_TILED(c, screen))
                n++;

        /* only masters on this screen? */
        if(n <= curtags[0]->nmaster)
            goto bailout;

        /* no tiled clients on this screen? */
        for(c = globalconf.clients; c && !IS_TILED(c, screen); c = c->next);
        if(!c)
            goto bailout;

        mouse_client_resize_tiled(c);
    }
    else if(layout == layout_magnifier)
        mouse_client_resize_magnified(c, infobox);

bailout:
    p_delete(&curtags);
}

/** Resize a client with mouse.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 * \lparam An optional table with keys: `corner', such as bottomleft,
 * topright, etc, to specify which corner to grab (default to auto) and
 * `infobox' to enable or disable the coordinates and dimensions box (default to
 * enabled).
 */
int
luaA_client_mouse_resize(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    corner_t corner = AutoCorner;
    bool infobox = true;
    size_t len;
    const char *buf;

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        luaA_checktable(L, 2);
        buf = luaA_getopt_lstring(L, 2, "corner", "auto", &len);
        corner = a_strtocorner(buf, len);
        infobox = luaA_getopt_boolean(L, 2, "infobox", true);
    }

    mouse_client_resize(*c, corner, infobox);

    return 0;
}

/** Move a client with mouse.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 * \lparam An optional table with keys: `snap' for pixel to snap (default to 8), and
 * `infobox' to enable or disable the coordinates and dimensions box (default to
 * enabled).
 */
int
luaA_client_mouse_move(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    int snap = 8;
    bool infobox = true;

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        luaA_checktable(L, 2);
        snap = luaA_getopt_number(L, 2, "snap", 8);
        infobox = luaA_getopt_boolean(L, 2, "infobox", true);
    }

    mouse_client_move(*c, snap, infobox);

    return 0;
}

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with modifiers keys, or a button to clone.
 * \lparam A mouse button number.
 * \lparam A function to execute on click events.
 * \lparam A function to execute on release events.
 * \lreturn A mouse button binding.
 */
static int
luaA_button_new(lua_State *L)
{
    int i, len;
    button_t *button, **orig;

    if((orig = luaA_toudata(L, 2, "button")))
    {
        button_t *copy = p_new(button_t, 1);
        copy->mod = (*orig)->mod;
        copy->button = (*orig)->button;
        if((*orig)->press != LUA_REFNIL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, (*orig)->press);
            luaA_registerfct(L, -1, &copy->press);
        }
        else
            copy->press = LUA_REFNIL;
        if((*orig)->release != LUA_REFNIL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, (*orig)->release);
            luaA_registerfct(L, -1, &copy->release);
        }
        else
            copy->release = LUA_REFNIL;
        return luaA_button_userdata_new(L, copy);
    }

    luaA_checktable(L, 2);
    /* arg 3 is mouse button */
    i = luaL_checknumber(L, 3);
    /* arg 4 and 5 are callback functions */
    if(!lua_isnil(L, 4))
        luaA_checkfunction(L, 4);
    if(lua_gettop(L) == 5 && !lua_isnil(L, 5))
        luaA_checkfunction(L, 5);

    button = p_new(button_t, 1);
    button->button = xutil_button_fromint(i);

    if(lua_isnil(L, 4))
        button->press = LUA_REFNIL;
    else
        luaA_registerfct(L, 4, &button->press);

    if(lua_gettop(L) == 5 && !lua_isnil(L, 5))
        luaA_registerfct(L, 5, &button->release);
    else
        button->release = LUA_REFNIL;

    len = lua_objlen(L, 2);
    for(i = 1; i <= len; i++)
    {
        size_t blen;
        const char *buf;
        lua_rawgeti(L, 2, i);
        buf = luaL_checklstring(L, -1, &blen);
        button->mod |= xutil_key_mask_fromstr(buf, blen);
    }

    return luaA_button_userdata_new(L, button);
}

/** Return a formated string for a button.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue  A button.
 * \lreturn A string.
 */
static int
luaA_button_tostring(lua_State *L)
{
    button_t **p = luaA_checkudata(L, 1, "button");
    lua_pushfstring(L, "[button udata(%p)]", *p);
    return 1;
}

/** Set a button array with a Lua table.
 * \param L The Lua VM state.
 * \param idx The index of the Lua table.
 * \param buttons The array button to fill.
 */
void
luaA_button_array_set(lua_State *L, int idx, button_array_t *buttons)
{
    button_t **b;

    luaA_checktable(L, idx);
    button_array_wipe(buttons);
    button_array_init(buttons);
    lua_pushnil(L);
    while(lua_next(L, idx))
    {
        b = luaA_checkudata(L, -1, "button");
        button_array_append(buttons, *b);
        button_ref(b);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/** Push an array of button as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param buttons The button array to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_button_array_get(lua_State *L, button_array_t *buttons)
{
    luaA_otable_new(L);
    for(int i = 0; i < buttons->len; i++)
    {
        luaA_button_userdata_new(L, buttons->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/** Button object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield press The function called when button press event is received.
 * \lfield release The function called when button release event is received.
 */
static int
luaA_button_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    button_t **button = luaA_checkudata(L, 1, "button");
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_PRESS:
        if((*button)->press != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, (*button)->press);
        else
            lua_pushnil(L);
        break;
      case A_TK_RELEASE:
        if((*button)->release != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, (*button)->release);
        else
            lua_pushnil(L);
        break;
      case A_TK_BUTTON:
        /* works fine, but not *really* neat */
        lua_pushnumber(L, (*button)->button);
        break;
      default:
        break;
    }

    return 1;
}

/** Button object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 */
static int
luaA_button_newindex(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    button_t **button = luaA_checkudata(L, 1, "button");
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_PRESS:
        luaA_registerfct(L, 3, &(*button)->press);
        break;
      case A_TK_RELEASE:
        luaA_registerfct(L, 3, &(*button)->release);
        break;
      case A_TK_BUTTON:
        (*button)->button = xutil_button_fromint(luaL_checknumber(L, 3));
        break;
      default:
        break;
    }

    return 0;
}

const struct luaL_reg awesome_button_methods[] =
{
    { "__call", luaA_button_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_button_meta[] =
{
    { "__index", luaA_button_index },
    { "__newindex", luaA_button_newindex },
    { "__gc", luaA_button_gc },
    { "__eq", luaA_button_eq },
    { "__tostring", luaA_button_tostring },
    { NULL, NULL }
};

/** Mouse library.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield coords Mouse coordinates.
 * \lfield screen Mouse screen number.
 */
static int
luaA_mouse_index(lua_State *L)
{
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);
    int mouse_x, mouse_y, i;
    int screen;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, NULL))
            return 0;

        i = screen_getbycoord(screen, mouse_x, mouse_y);

        lua_pushnumber(L, i + 1);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Newindex for mouse.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_mouse_newindex(lua_State *L)
{
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);
    int x, y = 0;
    xcb_window_t root;
    int screen, phys_screen;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        screen = luaL_checknumber(L, 3) - 1;
        luaA_checkscreen(screen);

        /* we need the physical one to get the root window */
        phys_screen = screen_virttophys(screen);
        root = xutil_screen_get(globalconf.connection, phys_screen)->root;

        x = globalconf.screens[screen].geometry.x;
        y = globalconf.screens[screen].geometry.y;

        mouse_warp_pointer(root, x, y);
        break;
      default:
        return 0;
    }

    return 0;
}

/** Get or set the mouse coords.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None or a table with x and y keys as mouse coordinates.
 * \lreturn A table with mouse coordinates.
 */
static int
luaA_mouse_coords(lua_State *L)
{
    uint16_t mask, maski;
    int screen, x, y, mouse_x, mouse_y, i = 1;

    if(lua_gettop(L) == 1)
    {
        xcb_window_t root;

        luaA_checktable(L, 1);

        if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, &mask))
            return 0;

        x = luaA_getopt_number(L, 1, "x", mouse_x);
        y = luaA_getopt_number(L, 1, "y", mouse_y);

        root = xutil_screen_get(globalconf.connection, screen)->root;
        mouse_warp_pointer(root, x, y);
        lua_pop(L, 1);
    }

    if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, &mask))
        return 0;

    lua_newtable(L);
    lua_pushnumber(L, mouse_x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, mouse_y);
    lua_setfield(L, -2, "y");
    lua_newtable(L);
    for(maski = XCB_BUTTON_MASK_1; i <= XCB_BUTTON_MASK_5; maski <<= 1, i++)
        if(mask & maski)
        {
            lua_pushboolean(L, true);
            lua_rawseti(L, -2, i);
        }
    lua_setfield(L, -2, "buttons");

    return 1;
}

const struct luaL_reg awesome_mouse_methods[] =
{
    { "__index", luaA_mouse_index },
    { "__newindex", luaA_mouse_newindex },
    { "coords", luaA_mouse_coords },
    { NULL, NULL }
};
const struct luaL_reg awesome_mouse_meta[] =
{
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
