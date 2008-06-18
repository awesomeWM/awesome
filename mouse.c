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
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "mouse.h"
#include "screen.h"
#include "tag.h"
#include "event.h"
#include "client.h"
#include "titlebar.h"
#include "layouts/floating.h"
#include "layouts/tile.h"
#include "layouts/magnifier.h"

#define MOUSEMASK      (XCB_EVENT_MASK_BUTTON_PRESS \
                        | XCB_EVENT_MASK_BUTTON_RELEASE \
                        | XCB_EVENT_MASK_POINTER_MOTION)

extern awesome_t globalconf;

DO_LUA_NEW(static, button_t, mouse, "mouse", button_ref)
DO_LUA_GC(button_t, mouse, "mouse", button_unref)
DO_LUA_EQ(button_t, mouse, "mouse")

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
 * \return A corner type.
 */
static corner_t
a_strtocorner(const char *str)
{
    if(!a_strcmp(str, "bottomright"))
        return BottomRightCorner;
    else if(!a_strcmp(str, "bottomleft"))
        return BottomLeftCorner;
    else if(!a_strcmp(str, "topleft"))
        return TopLeftCorner;
    else if(!a_strcmp(str, "topright"))
        return TopRightCorner;
    return AutoCorner;
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
                        globalconf.screens[c->screen].statusbar,
                        &globalconf.screens[c->screen].padding);

    geometry = titlebar_geometry_add(c->titlebar, c->border, geometry);
    geometry.width += 2 * c->border;
    geometry.height += 2 * c->border;

    geometry =
        mouse_snapclienttogeometry_inside(geometry, screen_geometry, snap);

    for(snapper = globalconf.clients; snapper; snapper = snapper->next)
        if(snapper != c && client_isvisible(c, c->screen))
        {
            snapper_geometry = snapper->geometry;
            snapper_geometry.width += 2 * c->border;
            snapper_geometry.height += 2 * c->border;
            snapper_geometry = titlebar_geometry_add(c->titlebar, c->border, snapper_geometry);
            geometry =
                mouse_snapclienttogeometry_outside(geometry,
                                                   snapper_geometry,
                                                   snap);
        }

    geometry.width -= 2 * c->border;
    geometry.height -= 2 * c->border;
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
 * \param ctx Draw context.
 * \param sw The simple window.
 * \param geometry The geometry to use for the box.
 * \param border The client border size.
 */
static void
mouse_infobox_draw(draw_context_t *ctx,
                   simple_window_t *sw,
                   area_t geometry, int border)
{
    area_t draw_geometry = { 0, 0, ctx->width, ctx->height, NULL, NULL };
    char size[64];

    snprintf(size, sizeof(size), "<text align=\"center\"/>%dx%d+%d+%d",
             geometry.width, geometry.height, geometry.x, geometry.y);
    draw_rectangle(ctx, draw_geometry, 1.0, true, globalconf.colors.bg);
    draw_text(ctx, globalconf.font, draw_geometry, size);
    simplewindow_move(sw,
                      geometry.x + ((2 * border + geometry.width) - sw->geometry.width) / 2,
                      geometry.y + ((2 * border + geometry.height) - sw->geometry.height) / 2);
    simplewindow_refresh_pixmap(sw);
}

/** Initialize the infobox window.
 * \param phys_screen Physical screen number.
 * \param border Border size of the client.
 * \param geometry Client geometry.
 * \param ctx Draw context to create.
 * \return The simple window.
 */
static simple_window_t *
mouse_infobox_new(int phys_screen, int border, area_t geometry,
                  draw_context_t **ctx)
{
    simple_window_t *sw;
    area_t geom;

    geom = draw_text_extents(globalconf.connection,
                             globalconf.default_screen,
                             globalconf.font,
                             "0000x0000+0000+0000");
    geom.x = geometry.x + ((2 * border + geometry.width) - geom.width) / 2;
    geom.y = geometry.y + ((2 * border + geometry.height) - geom.height) / 2;

    sw = simplewindow_new(globalconf.connection, phys_screen,
                          geom.x, geom.y,
                          geom.width, geom.height, 0);

    *ctx = draw_context_new(globalconf.connection, sw->phys_screen,
                            sw->geometry.width, sw->geometry.height,
                            sw->pixmap,
                            globalconf.colors.fg,
                            globalconf.colors.bg);

    xcb_map_window(globalconf.connection, sw->window);
    mouse_infobox_draw(*ctx, sw, geometry, border);

    return sw;
}

/** Get the Pointer position
 * \param window
 * \param x will be set to the Pointer-x-coordinate relative to window
 * \param y will be set to the Pointer-y-coordinate relative to window
 * \return true on success, false if an error occured
 **/
static bool
mouse_query_pointer(xcb_window_t window, int *x, int *y)
{
    xcb_query_pointer_cookie_t query_ptr_c;
    xcb_query_pointer_reply_t *query_ptr_r;

    query_ptr_c = xcb_query_pointer_unchecked(globalconf.connection, window);
    query_ptr_r = xcb_query_pointer_reply(globalconf.connection, query_ptr_c, NULL);

    if(!query_ptr_r)
        return false;

    *x = query_ptr_r->win_x;
    *y = query_ptr_r->win_y;

    p_delete(&query_ptr_r);

    return true;
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

/** Set the Pointer position
 * \param window the destination window
 * \param x x-coordinate inside window
 * \param y y-coordinate inside window
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
    {
        while((ev = xcb_wait_for_event(globalconf.connection)))
        {
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
                    xcb_handle_event(globalconf.evenths, ev);
                    p_delete(&ev);
                    break;
            }

        }
    }
}

/** Get the client that contains the Pointer.
 *
 * \return The client that contains the Pointer or NULL.
 */
static client_t *
mouse_get_client_under_pointer(void)
{
    xcb_window_t root;
    xcb_query_pointer_cookie_t query_ptr_c;
    xcb_query_pointer_reply_t *query_ptr_r;
    client_t *c = NULL;

    root = xutil_screen_get(globalconf.connection, globalconf.default_screen)->root;

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
    int last_x, last_y;
    /* current layout */
    layout_t *layout;
    /* the infobox */
    simple_window_t *sw = NULL;
    draw_context_t *ctx;
    /* the root window */
    xcb_window_t root;

    layout = layout_get_current(c->screen);
    root = xutil_screen_get(globalconf.connection, c->phys_screen)->root;

    /* get current pointer position */
    mouse_query_pointer(root, &last_x, &last_y);

    /* grab pointer */
    mouse_grab_pointer(root, CurMove);

    c->ismax = false;

    if(infobox && (c->isfloating || layout == layout_floating))
    {
        sw = mouse_infobox_new(c->phys_screen, c->border, c->geometry, &ctx);
        xcb_aux_sync(globalconf.connection);
    }

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        if(c->isfloating || layout == layout_floating)
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
            c->ismoving = false;

            /* draw the infobox */
            if(sw)
            {
                mouse_infobox_draw(ctx, sw, c->geometry, c->border);
                xcb_aux_sync(globalconf.connection);
            }

            /* keep track */
            last_x = mouse_x;
            last_y = mouse_y;
        }
        else
        {
            int newscreen;
            client_t *target;

            /* client moved to another screen? */
            newscreen = screen_get_bycoord(globalconf.screens_info, c->screen,
                                           mouse_x, mouse_y);
            if(newscreen != c->screen)
            {
                screen_client_moveto(c, newscreen, true);
                globalconf.screens[c->screen].need_arrange = true;
                globalconf.screens[newscreen].need_arrange = true;
                layout_refresh();
            }

            /* find client to swap with */
            target = mouse_get_client_under_pointer();

            /* swap position */
            if(target && target != c && !target->isfloating)
            {
                client_list_swap(&globalconf.clients, c, target);
                globalconf.screens[c->screen].need_arrange = true;
                layout_refresh();
            }
        }
    }

    /* ungrab pointer */
    xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);

    /* free the infobox */
    if(sw)
    {
        draw_context_delete(&ctx);
        simplewindow_delete(&sw);
    }

    xcb_aux_sync(globalconf.connection);
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
    simple_window_t *sw = NULL;
    draw_context_t  *ctx;
    size_t cursor = CurResize;
    int top, bottom, left, right;

    screen = xutil_screen_get(globalconf.connection, c->phys_screen);

    /* get current mouse position */
    mouse_query_pointer(screen->root, &mouse_x, &mouse_y);

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
    {
        sw = mouse_infobox_new(c->phys_screen, c->border, c->geometry, &ctx);
        xcb_aux_sync(globalconf.connection);
    }

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        /* new client geometry */
        area_t geo = { .x = MIN(fixed_x, mouse_x),
                       .y = MIN(fixed_y, mouse_y),
                       .width = (MAX(fixed_x, mouse_x) - MIN(fixed_x, mouse_x)),
                       .height = (MAX(fixed_y, mouse_y) - MIN(fixed_y, mouse_y)) };

        /* resize the client */
        client_resize(c, geo, true);

        /* draw the infobox */
        if(sw)
            mouse_infobox_draw(ctx, sw, c->geometry, c->border);

        xcb_aux_sync(globalconf.connection);
    }

    /* relase pointer */
    mouse_ungrab_pointer();

    /* free the infobox */
    if(sw)
    {
        draw_context_delete(&ctx);
        simplewindow_delete(&sw);
    }
}

/** Resize the master column/row of a tiled layout
 * \param c A client on the tag/layout to resize.
 */
static void
mouse_client_resize_tiled(client_t *c)
{
    xcb_screen_t *screen;
    /* screen area modulo statusbar */
    area_t area;
    /* current tag */
    tag_t *tag;
    /* current layout */
    layout_t *layout;

    int mouse_x, mouse_y;
    size_t cursor = CurResize;

    screen = xutil_screen_get(globalconf.connection, c->phys_screen);
    tag = tags_get_current(c->screen)[0];
    layout = tag->layout;

    area = screen_area_get(tag->screen,
                           globalconf.screens[tag->screen].statusbar,
                           &globalconf.screens[tag->screen].padding);

    mouse_query_pointer(screen->root, &mouse_x, &mouse_y);

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

    xcb_aux_sync(globalconf.connection);

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        double mwfact, fact_x, fact_y;

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
        mwfact = MIN( MAX( 0.01, mwfact), 0.99 );

        /* refresh layout */
        if(fabs(tag->mwfact - mwfact) >= 0.01)
        {
            tag->mwfact = mwfact;
            globalconf.screens[tag->screen].need_arrange = true;
            layout_refresh();
            xcb_aux_sync(globalconf.connection);
        }
    }

    /* relase pointer */
    mouse_ungrab_pointer();

    xcb_aux_sync(globalconf.connection);
}

/** Resize the master client in mangifier layout
 * \param c The client to resize.
 * \param infobox Enable or disable the infobox.
 */
static void
mouse_client_resize_magnified(client_t *c, bool infobox)
{
    /* screen area modulo statusbar */
    area_t area;
    /* center of area */
    int center_x, center_y;
    /* max. distance from the center */
    double maxdist;
    /* mouse position */
    int mouse_x, mouse_y;
    /* cursor while grabbing */
    size_t cursor = CurResize;
    corner_t corner = AutoCorner;
    /* current tag */
    tag_t *tag;
    /* the infobox */
    simple_window_t *sw = NULL;
    draw_context_t  *ctx;
    xcb_window_t root;

    tag = tags_get_current(c->screen)[0];

    root = xutil_screen_get(globalconf.connection, c->phys_screen)->root;

    area = screen_area_get(tag->screen,
                           globalconf.screens[tag->screen].statusbar,
                           &globalconf.screens[tag->screen].padding);

    center_x = area.x + (round(area.width / 2.));
    center_y = area.y + (round(area.height / 2.));

    maxdist = round(sqrt((area.width*area.width) + (area.height*area.height)) / 2.);

    /* get current mouse position */
    mouse_query_pointer(root, &mouse_x, &mouse_y);

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
    mouse_grab_pointer(root, cursor);

    /* move pointer to corner */
    mouse_warp_pointer(root, mouse_x, mouse_y);

    /* create the infobox */
    if(infobox)
        sw = mouse_infobox_new(c->phys_screen, c->border, c->geometry, &ctx);

    xcb_aux_sync(globalconf.connection);

    /* for each motion event */
    while(mouse_track_mouse_drag(&mouse_x, &mouse_y))
    {
        /* \todo keep pointer on screen diagonals */
        double mwfact, dist, dx, dy;

        /* calc distance from the center */
        dx = center_x - mouse_x;
        dy = center_y - mouse_y;
        dist = sqrt((dx*dx) + (dy*dy));

        /* new master/rest ratio */
        mwfact = dist / maxdist;

        /* keep mwfact within reasonable bounds */
        mwfact = MIN(MAX( 0.01, mwfact), 0.99);

        /* refresh the layout */
        if(fabs(tag->mwfact - mwfact) >= 0.01)
        {
            tag->mwfact = mwfact;
            globalconf.screens[tag->screen].need_arrange = true;
            layout_refresh();
        }

        /* draw the infobox */
        if(sw)
        {
            mouse_infobox_draw(ctx, sw, c->geometry, c->border);
            xcb_aux_sync(globalconf.connection);
        }
    }

    /* ungrab pointer */
    mouse_ungrab_pointer();

    /* free the infobox */
    if(sw)
    {
        draw_context_delete(&ctx);
        simplewindow_delete(&sw);
    }
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

    curtags = tags_get_current(c->screen);
    layout = curtags[0]->layout;
    s = xutil_screen_get(globalconf.connection, c->phys_screen);

    /* only handle floating, tiled and magnifier layouts */
    if(layout == layout_floating || c->isfloating)
    {
        if(c->isfixed)
            return;

        c->ismax = false;

        mouse_client_resize_floating(c, corner, infobox);
    }
    else if(layout == layout_tile || layout == layout_tileleft
            || layout == layout_tilebottom || layout == layout_tiletop)
    {
        screen = c->screen;
        for(n = 0, c = globalconf.clients; c; c = c->next)
            if(IS_TILED(c, screen))
                n++;

        /* only masters on this screen? */
        if(n <= curtags[0]->nmaster) return;

        /* no tiled clients on this screen? */
        for(c = globalconf.clients; c && !IS_TILED(c, screen); c = c->next);
        if(!c) return;

        mouse_client_resize_tiled(c);
    }
    else if(layout == layout_magnifier)
    {
        mouse_client_resize_magnified(c, infobox);
    }
}

/** Set mouse coordinates.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam The x coordinate.
 * \lparam The y coordinate.
 */
static int
luaA_mouse_coords_set(lua_State *L)
{
    int x, y;
    xcb_window_t root;

    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);

    root = xutil_screen_get(globalconf.connection, globalconf.default_screen)->root;
    mouse_warp_pointer(root, x, y);

    return 0;
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

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        luaA_checktable(L, 2);
        corner = a_strtocorner(luaA_getopt_string(L, 2, "corner", "auto"));
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

/** Get the screen number where the mouse ic.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lreturn The screen number.
 */
static int
luaA_mouse_screen_get(lua_State *L)
{
    int screen, mouse_x, mouse_y;
    xcb_window_t root;

    root = xutil_screen_get(globalconf.connection, globalconf.default_screen)->root;

    if(!mouse_query_pointer(root, &mouse_x, &mouse_y))
        return 0;

    screen = screen_get_bycoord(globalconf.screens_info,
                                globalconf.default_screen,
                                mouse_x, mouse_y);

    lua_pushnumber(L, screen + 1);

    return 1;
}

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \luastack
 * \lparam A table with modifiers keys.
 * \lparam A mouse button number.
 * \lparam A function to execute on click events.
 * \lreturn A mouse button binding.
 */
static int
luaA_mouse_new(lua_State *L)
{
    int i, len;
    button_t *button;

    luaA_checktable(L, 1);
    /* arg 3 is mouse button */
    i = luaL_checknumber(L, 2);
    /* arg 4 is cmd to run */
    luaA_checkfunction(L, 3);

    button = p_new(button_t, 1);
    button->button = xutil_button_fromint(i);
    button->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 1);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 1, i);
        button->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    return luaA_mouse_userdata_new(L, button);
}

const struct luaL_reg awesome_mouse_methods[] =
{
    { "new", luaA_mouse_new },
    { "screen_get", luaA_mouse_screen_get },
    { "coords_set", luaA_mouse_coords_set },
    { NULL, NULL }
};
const struct luaL_reg awesome_mouse_meta[] =
{
    { "__gc", luaA_mouse_gc },
    { "__eq", luaA_mouse_eq },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
