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

#define MOUSEMASK      (XCB_EVENT_MASK_BUTTON_PRESS \
                        | XCB_EVENT_MASK_BUTTON_RELEASE \
                        | XCB_EVENT_MASK_POINTER_MOTION)

extern awesome_t globalconf;

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

/** Snap a client with a futur geometry to the screen and other clients.
 * \param c The client.
 * \param geometry Geometry the client will get.
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

    geometry = titlebar_geometry_add(c->titlebar, geometry);
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
            snapper_geometry = titlebar_geometry_add(c->titlebar, snapper_geometry);
            geometry =
                mouse_snapclienttogeometry_outside(geometry,
                                                   snapper_geometry,
                                                   snap);
        }

    geometry.width -= 2 * c->border;
    geometry.height -= 2 * c->border;
    return titlebar_geometry_remove(c->titlebar, geometry);
}

/** Redraw the resizebar.
 * \param ctx Draw context.
 * \param style The style pointer to use for drawing.
 * \param geometry The geometry to use for the box.
 * \param border The client border size.
 */
static void
mouse_resizebar_draw(draw_context_t *ctx,
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

/** Initialize the resizebar window.
 * \param phys_screen Physical screen number.
 * \param border Border size of the client.
 * \param geometry Client geometry.
 * \param style Style used to draw.
 * \param ctx Draw context to create.
 * \return The simple window.
 */
static simple_window_t *
mouse_resizebar_new(int phys_screen, int border, area_t geometry,
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
    mouse_resizebar_draw(*ctx, sw, geometry, border);

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

    query_ptr_c = xcb_query_pointer(globalconf.connection, window);
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

    grab_ptr_c = xcb_grab_pointer(globalconf.connection, false, window,
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
mouse_ungrab_pointer()
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

/** Move the focused window with the mouse.
 */
static void
mouse_client_move(client_t *c, int snap)
{
    int ocx, ocy, newscreen;
    area_t geometry;
    client_t *target;
    layout_t *layout;
    simple_window_t *sw = NULL;
    draw_context_t *ctx;
    xcb_generic_event_t *ev = NULL;
    xcb_motion_notify_event_t *ev_motion = NULL;
    xcb_grab_pointer_reply_t *grab_pointer_r = NULL;
    xcb_grab_pointer_cookie_t grab_pointer_c;
    xcb_query_pointer_reply_t *query_pointer_r = NULL, *mquery_pointer_r = NULL;
    xcb_query_pointer_cookie_t query_pointer_c;
    xcb_screen_t *s;

    layout = layout_get_current(c->screen);
    s = xcb_aux_get_screen(globalconf.connection, c->phys_screen);

    /* Send pointer requests */
    grab_pointer_c = xcb_grab_pointer(globalconf.connection, false, s->root,
                                      MOUSEMASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                      s->root, globalconf.cursor[CurMove], XCB_CURRENT_TIME);

    query_pointer_c = xcb_query_pointer_unchecked(globalconf.connection, s->root);

    geometry = c->geometry;
    ocx = geometry.x;
    ocy = geometry.y;

    /* Get responses */
    if(!(grab_pointer_r = xcb_grab_pointer_reply(globalconf.connection, grab_pointer_c, NULL)))
        return;

    c->ismax = false;

    p_delete(&grab_pointer_r);

    query_pointer_r = xcb_query_pointer_reply(globalconf.connection, query_pointer_c, NULL);

    if(c->isfloating || layout == layout_floating)
    {
        sw = mouse_resizebar_new(c->phys_screen, c->border, c->geometry, &ctx);
        xcb_aux_sync(globalconf.connection);
    }

    while(true)
    {
        /* XMaskEvent allows to retrieve only specified events from
         * the queue and requeue the other events... */
        while(ev || (ev = xcb_wait_for_event(globalconf.connection)))
        {
            switch((ev->response_type & 0x7f))
            {
              case XCB_BUTTON_RELEASE:
                xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);
                if(sw)
                {
                    draw_context_delete(&ctx);
                    simplewindow_delete(&sw);
                }
                p_delete(&query_pointer_r);
                p_delete(&ev);
                return;
              case XCB_MOTION_NOTIFY:
                if(c->isfloating || layout == layout_floating)
                {
                    ev_motion = (xcb_motion_notify_event_t *) ev;

                    geometry.x = ocx + (ev_motion->event_x - query_pointer_r->root_x);
                    geometry.y = ocy + (ev_motion->event_y - query_pointer_r->root_y);

                    geometry = mouse_snapclient(c, geometry, snap);
                    c->ismoving = true;
                    client_resize(c, geometry, false);
                    c->ismoving = false;

                    if(sw)
                        mouse_resizebar_draw(ctx, sw, c->geometry, c->border);

                    xcb_aux_sync(globalconf.connection);
                }
                else
                {
                    query_pointer_c = xcb_query_pointer_unchecked(globalconf.connection, s->root);
                    mquery_pointer_r = xcb_query_pointer_reply(globalconf.connection, query_pointer_c, NULL);
                    if((newscreen = screen_get_bycoord(globalconf.screens_info, c->screen,
                                                       mquery_pointer_r->root_x,
                                                       mquery_pointer_r->root_y)) != c->screen)
                    {
                        screen_client_moveto(c, newscreen, true);
                        globalconf.screens[c->screen].need_arrange = true;
                        globalconf.screens[newscreen].need_arrange = true;
                        layout_refresh();
                    }
                    if((target = client_getbywin(mquery_pointer_r->child))
                       && target != c && !target->isfloating)
                    {
                        client_list_swap(&globalconf.clients, c, target);
                        globalconf.screens[c->screen].need_arrange = true;
                        layout_refresh();
                    }
                    p_delete(&mquery_pointer_r);
                }
                p_delete(&ev);
                while((ev = xcb_poll_for_event(globalconf.connection))
                      && (ev->response_type & 0x7f) == XCB_MOTION_NOTIFY)
                    p_delete(&ev);
                break;
              default:
                xcb_handle_event(globalconf.evenths, ev);
                p_delete(&ev);
                break;
            }
        }
    }
}


/** Utility function to help with mouse-dragging
 *
 * \param x set to x-coordinate of the last event on return
 * \param y set to y-coordinate of the last event on return
 * \return true if an motion event was recieved
 *         false if an button release event was recieved
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

/** Resize a floating client with the mouse.
 * \param c The client to resize.
 */
static void
mouse_client_resize_floating(client_t *c)
{
    xcb_screen_t *screen;
    /* one corner of the client has a fixed position */
    int fixed_x, fixed_y;
    /* the other is moved with the mouse */
    int mouse_x = 0, mouse_y = 0;
    /* the resize bar */
    simple_window_t *sw;
    draw_context_t  *ctx;
    size_t cursor = CurResize;

    screen = xcb_aux_get_screen(globalconf.connection, c->phys_screen);

    /* get current mouse poistion */
    mouse_query_pointer(screen->root, &mouse_x, &mouse_y);

    /* figure out which corner to move */
    {
        int top, bottom, left, right;

        top = c->geometry.y;
        bottom = top + c->geometry.height;
        left = c->geometry.x;
        right = left + c->geometry.width;

        if(abs(top - mouse_y) < abs(bottom - mouse_y))
        {
            mouse_y = top;
            fixed_y = bottom;
            if(abs(left - mouse_x) < abs(right - mouse_x))
            {
                mouse_x = left;
                fixed_x = right;
                cursor = CurTopLeft;
            }
            else
            {
                mouse_x = right;
                fixed_x = left;
                cursor = CurTopRight;
            }
        }
        else
        {
            mouse_y = bottom;
            fixed_y = top;
            if(abs(left - mouse_x) < abs(right - mouse_x))
            {
                mouse_x = left;
                fixed_x = right;
                cursor = CurBotLeft;
            }
            else
            {
                mouse_x = right;
                fixed_x = left;
                cursor = CurBotRight;
            }
        }
    }

    /* grab the pointer */
    if(!mouse_grab_pointer(screen->root, cursor))
        return;

    /* set pointer to the moveable corner */
    mouse_warp_pointer(screen->root, mouse_x, mouse_y);

    /* create the resizebar */
    sw = mouse_resizebar_new(c->phys_screen, c->border, c->geometry, &ctx);
    xcb_aux_sync(globalconf.connection);

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

        /* draw the resizebar */
        mouse_resizebar_draw(ctx, sw, c->geometry, c->border);

        xcb_aux_sync(globalconf.connection);
    }

    /* relase pointer */
    mouse_ungrab_pointer();

    /* free the resize bar */
    draw_context_delete(&ctx);
    simplewindow_delete(&sw);

    xcb_aux_sync(globalconf.connection);
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

    screen = xcb_aux_get_screen(globalconf.connection, c->phys_screen);
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


/** Resize a client with the mouse.
 * \param c The client to resize.
 */
static void
mouse_client_resize(client_t *c)
{
    int n, screen;
    tag_t **curtags;
    layout_t *layout;
    xcb_screen_t *s;

    curtags = tags_get_current(c->screen);
    layout = curtags[0]->layout;
    s = xcb_aux_get_screen(globalconf.connection, c->phys_screen);

    /* only handle floating and tiled layouts */
    if(layout == layout_floating || c->isfloating)
    {
        if(c->isfixed)
            return;

        c->ismax = false;

        mouse_client_resize_floating(c);
    }
    else if (layout == layout_tile || layout == layout_tileleft
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
}

/** Set mouse coordinates.
 * \param The x coordinates.
 * \param The y coordinates.
 */
static int
luaA_mouse_coords_set(lua_State *L)
{
    int x, y;
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);
    xcb_warp_pointer(globalconf.connection, XCB_NONE,
                     xcb_aux_get_screen(globalconf.connection, globalconf.default_screen)->root,
                     0, 0, 0, 0, x, y);
    return 0;
}

/** Resize a client with mouse.
 */
int
luaA_client_mouse_resize(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    mouse_client_resize(*c);
    return 0;
}

/** Move a client with mouse.
 * \param The pixel to snap.
 */
int
luaA_client_mouse_move(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    int snap = luaL_optnumber(L, 2, 8);
    mouse_client_move(*c, snap);
    return 0;
}

/** Get the screen number where the mouse ic.
 * \return The screen number.
 */
static int
luaA_mouse_screen_get(lua_State *L)
{
    int screen;
    xcb_query_pointer_cookie_t qc; 
    xcb_query_pointer_reply_t *qr; 

    qc = xcb_query_pointer(globalconf.connection,
                           xcb_aux_get_screen(globalconf.connection,
                                              globalconf.default_screen)->root);

    if(!(qr = xcb_query_pointer_reply(globalconf.connection, qc, NULL)))
        return 0;

    screen = screen_get_bycoord(globalconf.screens_info,
                                globalconf.default_screen,
                                qr->root_x, qr->root_y);
    p_delete(&qr);
    lua_pushnumber(L, screen + 1);

    return 1;
}

const struct luaL_reg awesome_mouse_lib[] =
{
    { "screen_get", luaA_mouse_screen_get },
    { "coords_set", luaA_mouse_coords_set },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
