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
#include "common/xscreen.h"

#define MOUSEMASK      (XCB_EVENT_MASK_BUTTON_PRESS \
                        | XCB_EVENT_MASK_BUTTON_RELEASE \
                        | XCB_EVENT_MASK_POINTER_MOTION)

extern AwesomeConf globalconf;

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
 * \param c the client
 * \param geometry geometry the client will get
 * \return geometry to set to the client
 */
static area_t
mouse_snapclient(Client *c, area_t geometry)
{
    Client *snapper;
    int snap = globalconf.screens[c->screen].snap;
    area_t snapper_geometry;
    area_t screen_geometry =
        screen_get_area(c->screen,
                        globalconf.screens[c->screen].statusbar,
                        &globalconf.screens[c->screen].padding);

    geometry = titlebar_geometry_add(&c->titlebar, geometry);
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
            snapper_geometry = titlebar_geometry_add(&snapper->titlebar,
                                                     snapper_geometry);
            geometry =
                mouse_snapclienttogeometry_outside(geometry,
                                                   snapper_geometry,
                                                   snap);
        }

    geometry.width -= 2 * c->border;
    geometry.height -= 2 * c->border;
    return titlebar_geometry_remove(&c->titlebar, geometry);
}

/** Redraw the resizebar
 * \param ctx draw context
 * \param style the style to use for drawing
 * \param geometry the geometry to use for the box
 * \param border the client border size
 */
static void
mouse_resizebar_draw(DrawCtx *ctx, style_t style, SimpleWindow *sw, area_t geometry, int border)
{
    area_t draw_geometry = { 0, 0, ctx->width, ctx->height, NULL, NULL };
    char size[32];

    snprintf(size, sizeof(size), "%dx%d+%d+%d",
             geometry.x, geometry.y, geometry.width, geometry.height);
    draw_text(ctx, draw_geometry, AlignCenter, style.font->height / 2, size, style);
    simplewindow_move(sw,
                      geometry.x + ((2 * border + geometry.width) - sw->geometry.width) / 2,
                      geometry.y + ((2 * border + geometry.height) - sw->geometry.height) / 2);
    simplewindow_refresh_drawable(sw, sw->phys_screen);
}

/** Initialize the resizebar window.
 * \param phys_screen physical screen id
 * \param border border size of the client
 * \param geometry client geometry
 * \param style style used to draw
 * \param ctx drawctx to create
 */
static SimpleWindow *
mouse_resizebar_new(int phys_screen, int border, area_t geometry, style_t style, DrawCtx **ctx)
{
    SimpleWindow *sw;
    area_t geom;

    geom.width = draw_textwidth(globalconf.connection,
                                globalconf.default_screen,
                                style.font,
                                "0000x0000+0000+0000") + style.font->height;
    geom.height = 1.5 * style.font->height;
    geom.x = geometry.x + ((2 * border + geometry.width) - geom.width) / 2;
    geom.y = geometry.y + ((2 * border + geometry.height) - geom.height) / 2;

    sw = simplewindow_new(globalconf.connection, phys_screen,
                          geom.x, geom.y,
                          geom.width, geom.height, 0);

    *ctx = draw_context_new(globalconf.connection, sw->phys_screen,
                            sw->geometry.width, sw->geometry.height,
                            sw->drawable);

    xcb_map_window(globalconf.connection, sw->window);
    mouse_resizebar_draw(*ctx, style, sw, geometry, border);

    return sw;
}

/** Move the focused window with the mouse.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_movemouse(int screen, char *arg __attribute__ ((unused)))
{
    int ocx, ocy, newscreen;
    area_t geometry;
    Client *c = globalconf.focus->client, *target;
    Layout *layout = layout_get_current(screen);
    SimpleWindow *sw = NULL;
    DrawCtx *ctx;
    style_t style;
    xcb_generic_event_t *ev = NULL;
    xcb_motion_notify_event_t *ev_motion = NULL;
    xcb_grab_pointer_reply_t *grab_pointer_r = NULL;
    xcb_grab_pointer_cookie_t grab_pointer_c;
    xcb_query_pointer_reply_t *query_pointer_r = NULL, *mquery_pointer_r = NULL;
    xcb_query_pointer_cookie_t query_pointer_c;
    xcb_screen_t *s = xcb_aux_get_screen(globalconf.connection, c->phys_screen);

    if(!c)
        return;

    /* Send pointer requests */
    grab_pointer_c = xcb_grab_pointer(globalconf.connection, false, s->root,
                                      MOUSEMASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                      s->root, globalconf.cursor[CurMove], XCB_CURRENT_TIME);

    query_pointer_c = xcb_query_pointer_unchecked(globalconf.connection, s->root);

    geometry = c->geometry;
    ocx = geometry.x;
    ocy = geometry.y;
    style = globalconf.screens[c->screen].styles.focus;

    /* Get responses */
    if(!(grab_pointer_r = xcb_grab_pointer_reply(globalconf.connection, grab_pointer_c, NULL)))
        return;

    c->ismax = false;

    p_delete(&grab_pointer_r);

    query_pointer_r = xcb_query_pointer_reply(globalconf.connection, query_pointer_c, NULL);

    if(c->isfloating || layout->arrange == layout_floating)
    {
        sw = mouse_resizebar_new(c->phys_screen, c->border, c->geometry, style, &ctx);
        xcb_aux_sync(globalconf.connection);
    }

    for(;;)
    {
        /* XMaskEvent allows to retrieve only specified events from
         * the queue and requeue the other events... */
        while((ev = xcb_wait_for_event(globalconf.connection)))
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
                if(c->isfloating || layout->arrange == layout_floating)
                {
                    ev_motion = (xcb_motion_notify_event_t *) ev;

                    geometry.x = ocx + (ev_motion->event_x - query_pointer_r->root_x);
                    geometry.y = ocy + (ev_motion->event_y - query_pointer_r->root_y);

                    geometry = mouse_snapclient(c, geometry);
                    c->ismoving = true;
                    client_resize(c, geometry, false);
                    c->ismoving = false;

                    if(sw)
                        mouse_resizebar_draw(ctx, style, sw, c->geometry, c->border);

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
                        move_client_to_screen(c, newscreen, true);
                        globalconf.screens[c->screen].need_arrange = true;
                        globalconf.screens[newscreen].need_arrange = true;
                        layout_refresh();
                    }
                    if((target = client_get_bywin(globalconf.clients, mquery_pointer_r->child))
                       && target != c && !target->isfloating)
                    {
                        client_list_swap(&globalconf.clients, c, target);
                        globalconf.screens[c->screen].need_arrange = true;
                        layout_refresh();
                    }
                    p_delete(&mquery_pointer_r);
                }
                break;
            default:
                xcb_handle_event(globalconf.evenths, ev);
                break;
            }
            p_delete(&ev);
        }
    }
}

/** Resize the focused window with the mouse.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_resizemouse(int screen, char *arg __attribute__ ((unused)))
{
    int ocx = 0, ocy = 0, n;
    xcb_generic_event_t *ev = NULL;
    xcb_motion_notify_event_t *ev_motion = NULL;
    Client *c = globalconf.focus->client;
    Tag **curtags = tags_get_current(screen);
    Layout *layout = curtags[0]->layout;
    area_t area = { 0, 0, 0, 0, NULL, NULL }, geometry = { 0, 0, 0, 0, NULL, NULL };
    double mwfact;
    SimpleWindow *sw = NULL;
    DrawCtx *ctx = NULL;
    style_t style;
    xcb_grab_pointer_cookie_t grab_pointer_c;
    xcb_grab_pointer_reply_t *grab_pointer_r = NULL;
    xcb_screen_t *s = xcb_aux_get_screen(globalconf.connection, c->phys_screen);

    /* only handle floating and tiled layouts */
    if(!c || c->isfixed)
        return;

    style = globalconf.screens[c->screen].styles.focus;

    if(layout->arrange == layout_floating || c->isfloating)
    {
        ocx = c->geometry.x;
        ocy = c->geometry.y;
        c->ismax = false;

        sw = mouse_resizebar_new(c->phys_screen, c->border, c->geometry, style, &ctx);
    }
    else if (layout->arrange == layout_tile || layout->arrange == layout_tileleft
             || layout->arrange == layout_tilebottom || layout->arrange == layout_tiletop)
    {
        for(n = 0, c = globalconf.clients; c; c = c->next)
            if(IS_TILED(c, screen))
                n++;

        if(n <= curtags[0]->nmaster) return;

        for(c = globalconf.clients; c && !IS_TILED(c, screen); c = c->next);
        if(!c) return;

        area = screen_get_area(screen,
                               globalconf.screens[c->screen].statusbar,
                               &globalconf.screens[c->screen].padding);
    }
    else
        return;

    grab_pointer_c = xcb_grab_pointer(globalconf.connection, false, s->root,
                                      MOUSEMASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                      s->root, globalconf.cursor[CurResize], XCB_CURRENT_TIME);

    if(!(grab_pointer_r = xcb_grab_pointer_reply(globalconf.connection,
                                                 grab_pointer_c, NULL)))
        return;

    p_delete(&grab_pointer_r);

    if(curtags[0]->layout->arrange == layout_tileleft)
        xcb_warp_pointer(globalconf.connection, XCB_NONE, c->win, 0, 0, 0, 0,
                         0, c->geometry.height + c->border - 1);
    else if(curtags[0]->layout->arrange == layout_tiletop)
        xcb_warp_pointer(globalconf.connection, XCB_NONE, c->win, 0, 0, 0, 0,
                         c->geometry.width + c->border - 1, 0);
    else
        xcb_warp_pointer(globalconf.connection, XCB_NONE, c->win, 0, 0, 0, 0,
                         c->geometry.width + c->border - 1,
                         c->geometry.height + c->border - 1);

    xcb_aux_sync(globalconf.connection);

    for(;;)
    {
        /* XMaskEvent allows to retrieve only specified events from
         * the queue and requeue the other events... */
        while((ev = xcb_wait_for_event(globalconf.connection)))
        {
            switch((ev->response_type & 0x7f))
            {
            case XCB_BUTTON_RELEASE:
                if(sw)
                {
                    draw_context_delete(&ctx);
                    simplewindow_delete(&sw);
                }
                xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);
                p_delete(&ev);
                p_delete(&curtags);
                return;
            case XCB_MOTION_NOTIFY:
                ev_motion = (xcb_motion_notify_event_t *) ev;

                if(layout->arrange == layout_floating || c->isfloating)
                {
                    if((geometry.width = ev_motion->event_x - ocx - 2 * c->border + 1) <= 0)
                        geometry.width = 1;
                    if((geometry.height = ev_motion->event_y - ocy - 2 * c->border + 1) <= 0)
                        geometry.height = 1;
                    geometry.x = c->geometry.x;
                    geometry.y = c->geometry.y;
                    client_resize(c, geometry, true);
                    if(sw)
                        mouse_resizebar_draw(ctx, style, sw, c->geometry, c->border);
                    xcb_aux_sync(globalconf.connection);
                }
                else if(layout->arrange == layout_tile || layout->arrange == layout_tileleft
                        || layout->arrange == layout_tiletop || layout->arrange == layout_tilebottom)
                {
                    if(layout->arrange == layout_tile)
                        mwfact = (double) (ev_motion->event_x - area.x) / area.width;
                    else if(curtags[0]->layout->arrange == layout_tileleft)
                        mwfact = 1 - (double) (ev_motion->event_x - area.x) / area.width;
                    else if(curtags[0]->layout->arrange == layout_tilebottom)
                        mwfact = (double) (ev_motion->event_y - area.y) / area.height;
                    else
                        mwfact = 1 - (double) (ev_motion->event_y - area.y) / area.height;
                    mwfact = MAX(globalconf.screens[screen].mwfact_lower_limit,
                                 MIN(globalconf.screens[screen].mwfact_upper_limit, mwfact));
                    if(fabs(curtags[0]->mwfact - mwfact) >= 0.01)
                    {
                        curtags[0]->mwfact = mwfact;
                        globalconf.screens[screen].need_arrange = true;
                        layout_refresh();
                    }
                }

                break;
            default:
                xcb_handle_event(globalconf.evenths, ev);
                break;
            }
            p_delete(&ev);
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
