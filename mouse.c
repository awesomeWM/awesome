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

#include "mouse.h"
#include "screen.h"
#include "tag.h"
#include "event.h"
#include "client.h"
#include "titlebar.h"
#include "layouts/floating.h"
#include "layouts/tile.h"
#include "common/xscreen.h"

#define MOUSEMASK      (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)

extern AwesomeConf globalconf;

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

static void
mouse_resizebar_draw(DrawCtx *ctx, style_t style, SimpleWindow *sw, area_t geometry, int border)
{
    area_t draw_geometry = { 0, 0, ctx->width, ctx->height, NULL, NULL };
    char size[64];

    snprintf(size, sizeof(size), "%dx%d+%d+%d",
             geometry.x, geometry.y, geometry.width, geometry.height);
    draw_text(ctx, draw_geometry, AlignCenter, style.font->height / 2, size, style);
    simplewindow_move(sw,
                      geometry.x + ((2 * border + geometry.width) - sw->geometry.width) / 2,
                      geometry.y + ((2 * border + geometry.height) - sw->geometry.height) / 2);
    simplewindow_refresh_drawable(sw, sw->phys_screen);
}

/** Move the focused window with the mouse.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_movemouse(int screen, char *arg __attribute__ ((unused)))
{
    int x, y, ocx, ocy, di, newscreen;
    unsigned int dui;
    Window dummy, child;
    XEvent ev;
    area_t geometry;
    Client *c = globalconf.focus->client, *target;
    Layout *layout = layout_get_current(screen);
    SimpleWindow *sw = NULL;
    DrawCtx *ctx;
    style_t style;

    if(!c
       || XGrabPointer(globalconf.display,
                       RootWindow(globalconf.display, c->phys_screen),
                       False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                       RootWindow(globalconf.display, c->phys_screen),
                       globalconf.cursor[CurMove], CurrentTime) != GrabSuccess)
        return;

    XQueryPointer(globalconf.display,
                  RootWindow(globalconf.display, c->phys_screen),
                  &dummy, &dummy, &x, &y, &di, &di, &dui);

    geometry = c->geometry;
    ocx = geometry.x;
    ocy = geometry.y;
    c->ismax = False;

    style = globalconf.screens[c->screen].styles.focus;

    if(c->isfloating || layout->arrange == layout_floating)
    {
        sw = simplewindow_new(globalconf.display, c->phys_screen, 0, 0,
                              draw_textwidth(globalconf.display,
                                             globalconf.screens[c->screen].styles.focus.font,
                                             "0000x0000+0000+0000") + style.font->height,
                              1.5 * style.font->height, 0);

        ctx = draw_context_new(globalconf.display, sw->phys_screen,
                               sw->geometry.width, sw->geometry.height,
                               sw->drawable);
        XMapRaised(globalconf.display, sw->window);
        mouse_resizebar_draw(ctx, style, sw, geometry, c->border);
    }

    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
          case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
            if(sw)
            {
                draw_context_delete(&ctx);
                simplewindow_delete(&sw);
            }
            return;
          case ConfigureRequest:
            event_handle_configurerequest(&ev);
            break;
          case Expose:
            event_handle_expose(&ev);
            break;
          case MapRequest:
            event_handle_maprequest(&ev);
            break;
          case EnterNotify:
            event_handle_enternotify(&ev);
            break;
          case MotionNotify:
            if(c->isfloating || layout->arrange == layout_floating)
            {
                geometry.x = ocx + (ev.xmotion.x - x);
                geometry.y = ocy + (ev.xmotion.y - y);

                geometry = mouse_snapclient(c, geometry);

                c->ismoving = True;
                client_resize(c, geometry, False);
                c->ismoving = False;
                if(sw)
                    mouse_resizebar_draw(ctx, style, sw, c->geometry, c->border);
            }
            else
            {
                XQueryPointer(globalconf.display,
                              RootWindow(globalconf.display, c->phys_screen),
                              &dummy, &child, &x, &y, &di, &di, &dui);
                if((newscreen = screen_get_bycoord(globalconf.screens_info, c->screen, x, y)) != c->screen)
                {
                    move_client_to_screen(c, newscreen, True);
                    globalconf.screens[c->screen].need_arrange = True;
                    if(layout_get_current(newscreen)->arrange != layout_floating)
                        globalconf.screens[newscreen].need_arrange = True;
                    layout_refresh();
                }
                if((target = client_get_bywin(globalconf.clients, child))
                   && target != c && !target->isfloating)
                {
                    client_list_swap(&globalconf.clients, c, target);
                    globalconf.screens[c->screen].need_arrange = True;
                    layout_refresh();
                }
            }
            while(XCheckMaskEvent(globalconf.display, PointerMotionMask, &ev));
            break;
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
    XEvent ev;
    Client *c = globalconf.focus->client;
    Tag **curtags = tags_get_current(screen);
    Layout *layout = curtags[0]->layout;
    area_t area = { 0, 0, 0, 0, NULL, NULL }, geometry = { 0, 0, 0, 0, NULL, NULL };
    double mwfact;
    SimpleWindow *sw = NULL;
    DrawCtx *ctx = NULL;
    style_t style;

    /* only handle floating and tiled layouts */
    if(!c || c->isfixed)
        return;

    style = globalconf.screens[c->screen].styles.focus;

    if(layout->arrange == layout_floating || c->isfloating)
    {
        ocx = c->geometry.x;
        ocy = c->geometry.y;
        c->ismax = False;

        sw = simplewindow_new(globalconf.display, c->phys_screen, 0, 0,
                              draw_textwidth(globalconf.display,
                                             globalconf.screens[c->screen].styles.focus.font,
                                             "0000x0000+0000+0000") + style.font->height,
                              1.5 * style.font->height, 0);

        ctx = draw_context_new(globalconf.display, sw->phys_screen,
                               sw->geometry.width, sw->geometry.height,
                               sw->drawable);
        XMapRaised(globalconf.display, sw->window);
        mouse_resizebar_draw(ctx, style, sw, c->geometry, c->border);
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

    if(XGrabPointer(globalconf.display,
                    RootWindow(globalconf.display, c->phys_screen),
                    False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                    RootWindow(globalconf.display, c->phys_screen),
                    globalconf.cursor[CurResize], CurrentTime) != GrabSuccess)
        return;

    if(curtags[0]->layout->arrange == layout_tileleft)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0, 0,
                     c->geometry.height + c->border - 1);
    else if(curtags[0]->layout->arrange == layout_tilebottom)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0,
                     c->geometry.width + c->border - 1, c->geometry.height + c->border - 1);
    else if(curtags[0]->layout->arrange == layout_tiletop)
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0,
                     c->geometry.width + c->border - 1, 0);
    else
        XWarpPointer(globalconf.display, None, c->win, 0, 0, 0, 0,
                     c->geometry.width + c->border - 1, c->geometry.height + c->border - 1);

    for(;;)
    {
        XMaskEvent(globalconf.display, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
        switch (ev.type)
        {
          case ButtonRelease:
            XUngrabPointer(globalconf.display, CurrentTime);
            if(sw)
            {
                draw_context_delete(&ctx);
                simplewindow_delete(&sw);
            }
            return;
          case ConfigureRequest:
            event_handle_configurerequest(&ev);
            break;
          case Expose:
            event_handle_expose(&ev);
            break;
          case MapRequest:
            event_handle_maprequest(&ev);
            break;
          case MotionNotify:
            if(layout->arrange == layout_floating || c->isfloating)
            {
                if((geometry.width = ev.xmotion.x - ocx - 2 * c->border + 1) <= 0)
                    geometry.width = 1;
                if((geometry.height = ev.xmotion.y - ocy - 2 * c->border + 1) <= 0)
                    geometry.height = 1;
                geometry.x = c->geometry.x;
                geometry.y = c->geometry.y;
                client_resize(c, geometry, True);
                if(sw)
                    mouse_resizebar_draw(ctx, style, sw, c->geometry, c->border);
            }
            else if(layout->arrange == layout_tile || layout->arrange == layout_tileleft
                    || layout->arrange == layout_tiletop || layout->arrange == layout_tilebottom)
            {
                if(layout->arrange == layout_tile)
                    mwfact = (double) (ev.xmotion.x - area.x) / area.width;
                else if(curtags[0]->layout->arrange == layout_tileleft)
                    mwfact = 1 - (double) (ev.xmotion.x - area.x) / area.width;
                else if(curtags[0]->layout->arrange == layout_tilebottom)
                    mwfact = (double) (ev.xmotion.y - area.y) / area.height;
                else
                    mwfact = 1 - (double) (ev.xmotion.y - area.y) / area.height;
                mwfact = MAX(globalconf.screens[screen].mwfact_lower_limit,
                             MIN(globalconf.screens[screen].mwfact_upper_limit, mwfact));
                if(fabs(curtags[0]->mwfact - mwfact) >= 0.01)
                {
                    curtags[0]->mwfact = mwfact;
                    globalconf.screens[screen].need_arrange = True;
                    layout_refresh();
                    while(XCheckMaskEvent(globalconf.display, PointerMotionMask, &ev));
                }
            }

            break;
        }
    }
    p_delete(&curtags);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
