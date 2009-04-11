/*
 * client.c - client management
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

#include <xcb/xtest.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_image.h>

#include "cnode.h"
#include "tag.h"
#include "window.h"
#include "ewmh.h"
#include "screen.h"
#include "titlebar.h"
#include "systray.h"
#include "property.h"
#include "wibox.h"
#include "common/atoms.h"

DO_LUA_NEW(extern, client_t, client, "client", client_ref)
DO_LUA_EQ(client_t, client, "client")
DO_LUA_GC(client_t, client, "client", client_unref)

/** Change the clients urgency flag.
 * \param c The client
 * \param urgent The new flag state
 */
void
client_seturgent(client_t *c, bool urgent)
{
    if(c->isurgent != urgent)
    {
        xcb_get_property_cookie_t hints =
            xcb_get_wm_hints_unchecked(globalconf.connection, c->win);

        c->isurgent = urgent;
        ewmh_client_update_hints(c);

        /* update ICCCM hints */
        xcb_wm_hints_t wmh;
        xcb_get_wm_hints_reply(globalconf.connection, hints, &wmh, NULL);

        if(urgent)
            wmh.flags |= XCB_WM_HINT_X_URGENCY;
        else
            wmh.flags &= ~XCB_WM_HINT_X_URGENCY;

        xcb_set_wm_hints(globalconf.connection, c->win, &wmh);

        hooks_property(c, "urgent");
    }
}

/** Returns true if a client is tagged
 * with one of the tags of the specified screen.
 * \param c The client to check.
 * \param screen Virtual screen number.
 * \return true if the client is visible, false otherwise.
 */
bool
client_maybevisible(client_t *c, int screen)
{
    if(c->screen == screen)
    {
        if(c->issticky || c->type == WINDOW_TYPE_DESKTOP)
            return true;

        tag_array_t *tags = &globalconf.screens[screen].tags;

        for(int i = 0; i < tags->len; i++)
            if(tags->tab[i]->selected && is_client_tagged(c, tags->tab[i]))
                return true;
    }
    return false;
}

/** Return the content of a client as an image.
 * That's just take a screenshot.
 * \param c The client.
 * \return An image.
 */
static image_t *
client_getcontent(client_t *c)
{
    xcb_image_t *ximage = xcb_image_get(globalconf.connection,
                                        c->win,
                                        0, 0,
                                        c->geometries.internal.width,
                                        c->geometries.internal.height,
                                        ~0, XCB_IMAGE_FORMAT_Z_PIXMAP);

    if(!ximage || ximage->bpp < 24)
        return NULL;

    uint32_t *data = p_alloca(uint32_t, ximage->width * ximage->height);

    for(int y = 0; y < ximage->height; y++)
        for(int x = 0; x < ximage->width; x++)
        {
            data[y * ximage->width + x] = xcb_image_get_pixel(ximage, x, y);
            data[y * ximage->width + x] |= 0xff000000; /* set alpha to 0xff */
        }

    image_t *image = image_new_from_argb32(ximage->width, ximage->height, data);

    xcb_image_destroy(ximage);

    return image;
}

/** Get a client by its window.
 * \param w The client window to find.
 * \return A client pointer if found, NULL otherwise.
 */
client_t *
client_getbywin(xcb_window_t w)
{
    client_t *c;
    for(c = globalconf.clients; c && c->win != w; c = c->next);
    return c;
}

/** Record that a client lost focus.
 * \param c Client being unfocused
 */
void
client_unfocus_update(client_t *c)
{
    globalconf.screens[c->phys_screen].client_focus = NULL;
    ewmh_update_net_active_window(c->phys_screen);

    /* Call hook */
    if(globalconf.hooks.unfocus != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.unfocus, 1, 0);
    }

}

/** Unfocus a client.
 * \param c The client.
 */
void
client_unfocus(client_t *c)
{
    xcb_window_t root_win = xutil_screen_get(globalconf.connection, c->phys_screen)->root;
    globalconf.screens[c->phys_screen].client_focus = NULL;

    /* Set focus on root window, so no events leak to the current window. */
    window_setfocus(root_win, true);
}

/** Ban client and move it out of the viewport.
 * \param c The client.
 */
void
client_ban(client_t *c)
{
    if(!c->isbanned)
    {
        /* Move all clients out of the physical viewport into negative coordinate space. */
        /* They will all be put on top of each other. */
        uint32_t request[2] = { - (c->geometries.internal.width + 2 * c->border),
                                - (c->geometries.internal.height + 2 * c->border) };

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             request);

        c->isbanned = true;

        /* All the wiboxes (may) need to be repositioned. */
        if(client_hasstrut(c))
            wibox_update_positions();

        /* Wait until the last moment to take away the focus from the window. */
        if(globalconf.screens[c->phys_screen].client_focus == c)
            client_unfocus(c);
    }
}

/** Record that a client got focus.
 * \param c Client being focused.
 */
void
client_focus_update(client_t *c)
{
    if(!client_maybevisible(c, c->screen))
        return;

    /* stop hiding client */
    c->ishidden = false;
    client_setminimized(c, false);

    /* unban the client before focusing for consistency */
    client_unban(c);

    globalconf.screen_focus = &globalconf.screens[c->phys_screen];
    globalconf.screen_focus->client_focus = c;

    /* Some layouts use focused client differently, so call them back.
     * And anyway, we have maybe unhidden */
    client_need_arrange(c);

    /* according to EWMH, we have to remove the urgent state from a client */
    client_seturgent(c, false);

    ewmh_update_net_active_window(c->phys_screen);

    /* execute hook */
    if(globalconf.hooks.focus != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.focus, 1, 0);
    }
}


/** Give focus to client, or to first client if client is NULL.
 * \param c The client or NULL.
 */
void
client_focus(client_t *c)
{
    /* We have to set focus on first client */
    if(!c && !(c = globalconf.clients))
        return;

    if(!client_maybevisible(c, c->screen))
        return;

    globalconf.screen_focus = &globalconf.screens[c->phys_screen];
    globalconf.screen_focus->client_focus = c;

    window_setfocus(c->win, !c->nofocus);
}

/** Stack a window below.
 * \param c The client.
 * \param previous The previous window on the stack.
 * \param return The next-previous!
 */
static xcb_window_t
client_stack_above(client_t *c, xcb_window_t previous)
{
    uint32_t config_win_vals[2];

    config_win_vals[0] = previous;
    config_win_vals[1] = XCB_STACK_MODE_ABOVE;

    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                         config_win_vals);

    config_win_vals[0] = c->win;

    if(c->titlebar)
    {
        xcb_configure_window(globalconf.connection,
                             c->titlebar->sw.window,
                             XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                             config_win_vals);
        previous = c->titlebar->sw.window;
    }
    else
        previous = c->win;

    /* stack transient window on top of their parents */
    for(client_node_t *node = *client_node_list_last(&globalconf.stack);
        node; node = node->prev)
        if(node->client->transient_for == c)
            previous = client_stack_above(node->client, previous);

    return previous;
}

/** Stacking layout layers */
typedef enum
{
    /** This one is a special layer */
    LAYER_IGNORE,
    LAYER_DESKTOP,
    LAYER_BELOW,
    LAYER_NORMAL,
    LAYER_ABOVE,
    LAYER_FULLSCREEN,
    LAYER_ONTOP,
    /** This one only used for counting and is not a real layer */
    LAYER_COUNT
} layer_t;

/** Get the real layer of a client according to its attribute (fullscreen, …)
 * \param c The client.
 * \return The real layer.
 */
static layer_t
client_layer_translator(client_t *c)
{
    /* first deal with user set attributes */
    if(c->isontop)
        return LAYER_ONTOP;
    else if(c->isfullscreen)
        return LAYER_FULLSCREEN;
    else if(c->isabove)
        return LAYER_ABOVE;
    else if(c->isbelow)
        return LAYER_BELOW;

    /* check for transient attr */
    if(c->transient_for)
        return LAYER_IGNORE;

    /* then deal with windows type */
    switch(c->type)
    {
      case WINDOW_TYPE_DESKTOP:
        return LAYER_DESKTOP;
      default:
        break;
    }

    return LAYER_NORMAL;
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
static void
client_real_stack(void)
{
    uint32_t config_win_vals[2];
    client_node_t *node, *last = *client_node_list_last(&globalconf.stack);
    layer_t layer;
    int screen;

    config_win_vals[0] = XCB_NONE;
    config_win_vals[1] = XCB_STACK_MODE_ABOVE;

    /* stack desktop windows */
    for(layer = LAYER_DESKTOP; layer < LAYER_BELOW; layer++)
        for(node = last; node; node = node->prev)
            if(client_layer_translator(node->client) == layer)
                config_win_vals[0] = client_stack_above(node->client,
                                                        config_win_vals[0]);

    /* first stack not ontop wibox window */
    for(screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
        {
            wibox_t *sb = globalconf.screens[screen].wiboxes.tab[i];
            if(!sb->ontop)
            {
                xcb_configure_window(globalconf.connection,
                                     sb->sw.window,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = sb->sw.window;
            }
        }

    /* then stack clients */
    for(layer = LAYER_BELOW; layer < LAYER_COUNT; layer++)
        for(node = last; node; node = node->prev)
            if(client_layer_translator(node->client) == layer)
                config_win_vals[0] = client_stack_above(node->client,
                                                        config_win_vals[0]);

    /* then stack ontop wibox window */
    for(screen = 0; screen < globalconf.nscreen; screen++)
        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
        {
            wibox_t *sb = globalconf.screens[screen].wiboxes.tab[i];
            if(sb->ontop)
            {
                xcb_configure_window(globalconf.connection,
                                     sb->sw.window,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = sb->sw.window;
            }
        }
}

void
client_stack_refresh()
{
    if (!globalconf.client_need_stack_refresh)
        return;
    globalconf.client_need_stack_refresh = false;
    client_real_stack();
}

/** Manage a new client.
 * \param w The window.
 * \param wgeom Window geometry.
 * \param phys_screen Physical screen number.
 * \param startup True if we are managing at startup time.
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, int phys_screen, bool startup)
{
    xcb_get_property_cookie_t ewmh_icon_cookie;
    client_t *c, *tc = NULL;
    image_t *icon;
    int screen;
    const uint32_t select_input_val[] = { CLIENT_SELECT_INPUT_EVENT_MASK };

    if(systray_iskdedockapp(w))
    {
        systray_request_handle(w, phys_screen, NULL);
        return;
    }

    /* Send request to get NET_WM_ICON property as soon as possible... */
    ewmh_icon_cookie = ewmh_window_icon_get_unchecked(w);
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);
    c = p_new(client_t, 1);

    screen = c->screen = screen_getbycoord(phys_screen, wgeom->x, wgeom->y);

    c->phys_screen = phys_screen;

    /* Initial values */
    c->win = w;
    c->geometry.x = wgeom->x;
    c->geometry.y = wgeom->y;
    /* Border will be added later. */
    c->geometry.width = wgeom->width;
    c->geometry.height = wgeom->height;
    /* Also set internal geometry (client_ban() needs it). */
    c->geometries.internal.x = wgeom->x;
    c->geometries.internal.y = wgeom->y;
    c->geometries.internal.width = wgeom->width;
    c->geometries.internal.height = wgeom->height;
    client_setborder(c, wgeom->border_width);
    if((icon = ewmh_window_icon_get_reply(ewmh_icon_cookie)))
        c->icon = image_ref(&icon);

    /* we honor size hints by default */
    c->size_hints_honor = true;

    /* update hints */
    property_update_wm_normal_hints(c, NULL);
    property_update_wm_hints(c, NULL);
    property_update_wm_transient_for(c, NULL);
    property_update_wm_client_leader(c, NULL);

    /* Recursively find the parent. */
    for(tc = c; tc->transient_for; tc = tc->transient_for);
    if(tc != c && tc->phys_screen == c->phys_screen)
        screen = tc->screen;

    /* Then check clients hints */
    ewmh_client_check_hints(c);

    /* move client to screen, but do not tag it */
    screen_client_moveto(c, screen, false, true);

    /* Push client in client list */
    client_list_push(&globalconf.clients, client_ref(&c));

    /* Push client in stack */
    client_raise(c);

    /* update window title */
    property_update_wm_name(c);
    property_update_wm_icon_name(c);
    property_update_wm_class(c);

    /* update strut */
    ewmh_process_client_strut(c, NULL);

    ewmh_update_net_client_list(c->phys_screen);

    /* Always stay in NORMAL_STATE. Even though iconified seems more
     * appropriate sometimes. The only possible loss is that clients not using
     * visibility events may continue to proces data (when banned).
     * Without any exposes or other events the cost should be fairly limited though.
     *
     * Some clients may expect the window to be unmapped when STATE_ICONIFIED.
     * Two conflicting parts of the ICCCM v2.0 (section 4.1.4):
     *
     * "Normal -> Iconic - The client should send a ClientMessage event as described later in this section."
     * (note no explicit mention of unmapping, while Normal->Widthdrawn does mention that)
     *
     * "Once a client's window has left the Withdrawn state, the window will be mapped
     * if it is in the Normal state and the window will be unmapped if it is in the Iconic state."
     *
     * At this stage it's just safer to keep it in normal state and avoid confusion.
     */
    window_state_set(c->win, XCB_WM_STATE_NORMAL);

    /* Move window outside the viewport before mapping it. */
    client_ban(c);
    xcb_map_window(globalconf.connection, c->win);

    /* Call hook to notify list change */
    if(globalconf.hooks.clients != LUA_REFNIL)
        luaA_dofunction(globalconf.L, globalconf.hooks.clients, 0, 0);

    /* call hook */
    if(globalconf.hooks.manage != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, c);
        lua_pushboolean(globalconf.L, startup);
        luaA_dofunction(globalconf.L, globalconf.hooks.manage, 2, 0);
    }
}

/** Compute client geometry with respect to its geometry hints.
 * \param c The client.
 * \param geometry The geometry that the client might receive.
 * \return The geometry the client must take respecting its hints.
 */
area_t
client_geometry_hints(client_t *c, area_t geometry)
{
    int32_t basew, baseh, minw, minh;

    /* base size is substituted with min size if not specified */
    if(c->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
    {
        basew = c->size_hints.base_width;
        baseh = c->size_hints.base_height;
    }
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
    {
        basew = c->size_hints.min_width;
        baseh = c->size_hints.min_height;
    }
    else
        basew = baseh = 0;

    /* min size is substituted with base size if not specified */
    if(c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
    {
        minw = c->size_hints.min_width;
        minh = c->size_hints.min_height;
    }
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
    {
        minw = c->size_hints.base_width;
        minh = c->size_hints.base_height;
    }
    else
        minw = minh = 0;

    if(c->size_hints.flags & XCB_SIZE_HINT_P_ASPECT
       && c->size_hints.min_aspect_num > 0
       && c->size_hints.min_aspect_den > 0
       && geometry.height - baseh > 0
       && geometry.width - basew > 0)
    {
        double dx = (double) (geometry.width - basew);
        double dy = (double) (geometry.height - baseh);
        double min = (double) c->size_hints.min_aspect_num / (double) c->size_hints.min_aspect_den;
        double max = (double) c->size_hints.max_aspect_num / (double) c->size_hints.min_aspect_den;
        double ratio = dx / dy;
        if(max > 0 && min > 0 && ratio > 0)
        {
            if(ratio < min)
            {
                dy = (dx * min + dy) / (min * min + 1);
                dx = dy * min;
                geometry.width = (int) dx + basew;
                geometry.height = (int) dy + baseh;
            }
            else if(ratio > max)
            {
                dy = (dx * min + dy) / (max * max + 1);
                dx = dy * min;
                geometry.width = (int) dx + basew;
                geometry.height = (int) dy + baseh;
            }
        }
    }

    if(minw)
        geometry.width = MAX(geometry.width, minw);
    if(minh)
        geometry.height = MAX(geometry.height, minh);

    if(c->size_hints.flags & XCB_SIZE_HINT_P_MAX_SIZE)
    {
        if(c->size_hints.max_width)
            geometry.width = MIN(geometry.width, c->size_hints.max_width);
        if(c->size_hints.max_height)
            geometry.height = MIN(geometry.height, c->size_hints.max_height);
    }

    if(c->size_hints.flags & (XCB_SIZE_HINT_P_RESIZE_INC | XCB_SIZE_HINT_BASE_SIZE)
       && c->size_hints.width_inc && c->size_hints.height_inc)
    {
        uint16_t t1 = geometry.width, t2 = geometry.height;
        unsigned_subtract(t1, basew);
        unsigned_subtract(t2, baseh);
        geometry.width -= t1 % c->size_hints.width_inc;
        geometry.height -= t2 % c->size_hints.height_inc;
    }

    return geometry;
}

/** Resize client window.
 * The sizse given as parameters are with titlebar and borders!
 * \param c Client to resize.
 * \param geometry New window geometry.
 * \param hints Use size hints.
 * \return true if an actual resize occurred.
 */
bool
client_resize(client_t *c, area_t geometry, bool hints)
{
    int new_screen;
    area_t geometry_internal;
    area_t area;

    /* offscreen appearance fixes */
    area = display_area_get(c->phys_screen, NULL,
                            &globalconf.screens[c->screen].padding);

    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height;
    if(geometry.x + geometry.width < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height < 0)
        geometry.y = 0;

    /* Real client geometry, please keep it contained to C code at the very least. */
    geometry_internal = titlebar_geometry_remove(c->titlebar, c->border, geometry);

    if(hints)
        geometry_internal = client_geometry_hints(c, geometry_internal);

    if(geometry_internal.width == 0 || geometry_internal.height == 0)
        return false;

    /* Also let client hints propegate to the "official" geometry. */
    geometry = titlebar_geometry_add(c->titlebar, c->border, geometry_internal);

    if(c->geometries.internal.x != geometry_internal.x
       || c->geometries.internal.y != geometry_internal.y
       || c->geometries.internal.width != geometry_internal.width
       || c->geometries.internal.height != geometry_internal.height)
    {
        new_screen = screen_getbycoord(c->screen, geometry_internal.x, geometry_internal.y);

        /* Values to configure a window is an array where values are
         * stored according to 'value_mask' */
        uint32_t values[4];

        c->geometries.internal.x = values[0] = geometry_internal.x;
        c->geometries.internal.y = values[1] = geometry_internal.y;
        c->geometries.internal.width = values[2] = geometry_internal.width;
        c->geometries.internal.height = values[3] = geometry_internal.height;

        /* Also store geometry including border and titlebar. */
        c->geometry = geometry;

        titlebar_update_geometry(c);

        /* The idea is to give a client a resize even when banned. */
        /* We just have to move the (x,y) to keep it out of the viewport. */
        /* This at least doesn't break expectations about events. */
        if(c->isbanned)
        {
            geometry_internal.x = values[0] = - (geometry_internal.width + 2 * c->border);
            geometry_internal.y = values[1] = - (geometry_internal.height + 2 * c->border);
        }

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                             | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             values);
        window_configure(c->win, geometry_internal, c->border);

        screen_client_moveto(c, new_screen, true, false);

        /* execute hook */
        hooks_property(c, "geometry");

        return true;
    }

    return false;
}

/** Update the position of all window with struts on a specific screen.
 * \param screen The screen that should be processed.
 */
void
client_update_strut_positions(int screen)
{
    client_t *c;
    area_t allowed_area, geom;

    /* Ignore all struts for starters. */
    for(c = globalconf.clients; c; c = c->next)
        if(c->screen == screen && client_hasstrut(c))
            c->ignore_strut = true;

    /* Rationale:
     * Top and bottom panels are common, so they take precendence.
     * WINDOW_TYPE_DOCK really wants to be at the side, so choose them first.
     */

    /* WINDOW_TYPE_DOCK: top + bottom. */
    for(c = globalconf.clients; c; c = c->next)
    {
        if(c->screen != screen || !client_hasstrut(c) || c->type != WINDOW_TYPE_DOCK)
            continue;

        /* Screen area, minus padding, wibox'es and already processed struts. */
        allowed_area = screen_area_get(c->screen,
                        &globalconf.screens[c->screen].wiboxes,
                        &globalconf.screens[c->screen].padding,
                        true);

        geom = c->geometry;

        if(c->strut.top || c->strut.top_start_x || c->strut.top_end_x)
        {
            geom.y = allowed_area.y;
            if(geom.x < allowed_area.x
               || geom.x + geom.width > allowed_area.x + allowed_area.width)
            {
                geom.x = allowed_area.x;
                if(geom.width > allowed_area.width)
                    geom.width = allowed_area.width;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
        else if(c->strut.bottom || c->strut.bottom_start_x || c->strut.bottom_end_x)
        {
            geom.y = allowed_area.y + allowed_area.height - geom.height;
            if(geom.x < allowed_area.x
               || geom.x + geom.width > allowed_area.x + allowed_area.width)
            {
                geom.x = allowed_area.x;
                if(geom.width > allowed_area.width)
                    geom.width = allowed_area.width;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
    }

    /* WINDOW_TYPE_DOCK: left + right. */
    for(c = globalconf.clients; c; c = c->next)
    {
        if(c->screen != screen || !client_hasstrut(c) || c->type != WINDOW_TYPE_DOCK)
            continue;

        /* Screen area, minus padding, wibox'es and already processed struts. */
        allowed_area = screen_area_get(c->screen,
                        &globalconf.screens[c->screen].wiboxes,
                        &globalconf.screens[c->screen].padding,
                        true);

        geom = c->geometry;

        if(c->strut.left || c->strut.left_start_y || c->strut.left_end_y)
        {
            geom.x = allowed_area.x;
            if(geom.y < allowed_area.y
               || geom.y + geom.height > allowed_area.y + allowed_area.height)
            {
                geom.y = allowed_area.y;
                if (geom.height > allowed_area.height)
                    geom.height = allowed_area.height;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
        else if(c->strut.right || c->strut.right_start_y || c->strut.right_end_y)
        {
            geom.x = allowed_area.x + allowed_area.width - geom.width;
            if(geom.y < allowed_area.y
               || geom.y + geom.height > allowed_area.y + allowed_area.height)
            {
                geom.y = allowed_area.y;
                if (geom.height > allowed_area.height)
                    geom.height = allowed_area.height;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
    }

    /* not WINDOW_TYPE_DOCK: top + bottom. */
    for(c = globalconf.clients; c; c = c->next)
    {
        if(c->screen != screen || !client_hasstrut(c) || c->type == WINDOW_TYPE_DOCK)
            continue;

        /* Screen area, minus padding, wibox'es and already processed struts. */
        allowed_area = screen_area_get(c->screen,
                        &globalconf.screens[c->screen].wiboxes,
                        &globalconf.screens[c->screen].padding,
                        true);

        geom = c->geometry;

        if(c->strut.top || c->strut.top_start_x || c->strut.top_end_x)
        {
            geom.y = allowed_area.y;
            if(geom.x < allowed_area.x
               || geom.x + geom.width > allowed_area.x + allowed_area.width)
            {
                geom.x = allowed_area.x;
                if(geom.width > allowed_area.width)
                    geom.width = allowed_area.width;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
        else if(c->strut.bottom || c->strut.bottom_start_x || c->strut.bottom_end_x)
        {
            geom.y = allowed_area.y + allowed_area.height - geom.height;
            if(geom.x < allowed_area.x
               || geom.x + geom.width > allowed_area.x + allowed_area.width)
            {
                geom.x = allowed_area.x;
                if(geom.width > allowed_area.width)
                    geom.width = allowed_area.width;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
    }

    /* not WINDOW_TYPE_DOCK: left + right. */
    for(c = globalconf.clients; c; c = c->next)
    {
        if(c->screen != screen || !client_hasstrut(c) || c->type == WINDOW_TYPE_DOCK)
            continue;

        /* Screen area, minus padding, wibox'es and already processed struts. */
        allowed_area = screen_area_get(c->screen,
                        &globalconf.screens[c->screen].wiboxes,
                        &globalconf.screens[c->screen].padding,
                        true);

        geom = c->geometry;

        if(c->strut.left || c->strut.left_start_y || c->strut.left_end_y)
        {
            geom.x = allowed_area.x;
            if(geom.y < allowed_area.y
               || geom.y + geom.height > allowed_area.y + allowed_area.height)
            {
                geom.y = allowed_area.y;
                if (geom.height > allowed_area.height)
                    geom.height = allowed_area.height;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
        else if(c->strut.right || c->strut.right_start_y || c->strut.right_end_y)
        {
            geom.x = allowed_area.x + allowed_area.width - geom.width;
            if(geom.y < allowed_area.y
               || geom.y + geom.height > allowed_area.y + allowed_area.height)
            {
                geom.y = allowed_area.y;
                if (geom.height > allowed_area.height)
                    geom.height = allowed_area.height;
            }
            c->ignore_strut = false;
            client_resize(c, geom, false);
        }
    }
}


/** Set a client minimized, or not.
 * \param c The client.
 * \param s Set or not the client minimized.
 */
void
client_setminimized(client_t *c, bool s)
{
    if(c->isminimized != s)
    {
        client_need_arrange(c);
        c->isminimized = s;
        client_need_arrange(c);
        ewmh_client_update_hints(c);
        /* execute hook */
        hooks_property(c, "minimized");
    }
}

/** Set a client sticky, or not.
 * \param c The client.
 * \param s Set or not the client sticky.
 */
void
client_setsticky(client_t *c, bool s)
{
    if(c->issticky != s)
    {
        client_need_arrange(c);
        c->issticky = s;
        client_need_arrange(c);
        ewmh_client_update_hints(c);
        hooks_property(c, "sticky");
    }
}

/** Set a client fullscreen, or not.
 * \param c The client.
 * \param s Set or not the client fullscreen.
 */
void
client_setfullscreen(client_t *c, bool s)
{
    if(c->isfullscreen != s)
    {
        area_t geometry;

        /* Make sure the current geometry is stored without titlebar. */
        if(s)
            titlebar_ban(c->titlebar);

        /* become fullscreen! */
        if((c->isfullscreen = s))
        {
            /* remove any max state */
            client_setmaxhoriz(c, false);
            client_setmaxvert(c, false);
            /* You can only be part of one of the special layers. */
            client_setbelow(c, false);
            client_setabove(c, false);
            client_setontop(c, false);

            geometry = screen_area_get(c->screen, NULL, NULL, false);
            c->geometries.fullscreen = c->geometry;
            c->border_fs = c->border;
            client_setborder(c, 0);
        }
        else
        {
            geometry = c->geometries.fullscreen;
            client_setborder(c, c->border_fs);
        }
        client_resize(c, geometry, false);
        client_need_arrange(c);
        client_stack();
        ewmh_client_update_hints(c);
        hooks_property(c, "fullscreen");
    }
}

/** Set a client horizontally maximized.
 * \param c The client.
 * \param s The maximized status.
 */
void
client_setmaxhoriz(client_t *c, bool s)
{
    if(c->ismaxhoriz != s)
    {
        area_t geometry;

        if((c->ismaxhoriz = s))
        {
            /* remove fullscreen mode */
            client_setfullscreen(c, false);

            geometry = screen_area_get(c->screen,
                                       &globalconf.screens[c->screen].wiboxes,
                                       &globalconf.screens[c->screen].padding,
                                       true);
            geometry.y = c->geometry.y;
            geometry.height = c->geometry.height;
            c->geometries.max.x = c->geometry.x;
            c->geometries.max.width = c->geometry.width;
        }
        else
        {
            geometry = c->geometry;
            geometry.x = c->geometries.max.x;
            geometry.width = c->geometries.max.width;
        }

        client_resize(c, geometry, c->size_hints_honor);
        client_need_arrange(c);
        client_stack();
        ewmh_client_update_hints(c);
        hooks_property(c, "maximized_horizontal");
    }
}

/** Set a client vertically maximized.
 * \param c The client.
 * \param s The maximized status.
 */
void
client_setmaxvert(client_t *c, bool s)
{
    if(c->ismaxvert != s)
    {
        area_t geometry;

        if((c->ismaxvert = s))
        {
            /* remove fullscreen mode */
            client_setfullscreen(c, false);

            geometry = screen_area_get(c->screen,
                                       &globalconf.screens[c->screen].wiboxes,
                                       &globalconf.screens[c->screen].padding,
                                       true);
            geometry.x = c->geometry.x;
            geometry.width = c->geometry.width;
            c->geometries.max.y = c->geometry.y;
            c->geometries.max.height = c->geometry.height;
        }
        else
        {
            geometry = c->geometry;
            geometry.y = c->geometries.max.y;
            geometry.height = c->geometries.max.height;
        }

        client_resize(c, geometry, c->size_hints_honor);
        client_need_arrange(c);
        client_stack();
        ewmh_client_update_hints(c);
        hooks_property(c, "maximized_vertical");
    }
}

/** Set a client above, or not.
 * \param c The client.
 * \param s Set or not the client above.
 */
void
client_setabove(client_t *c, bool s)
{
    if(c->isabove != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_setbelow(c, false);
            client_setontop(c, false);
            client_setfullscreen(c, false);
        }
        c->isabove = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hooks_property(c, "above");
    }
}

/** Set a client below, or not.
 * \param c The client.
 * \param s Set or not the client below.
 */
void
client_setbelow(client_t *c, bool s)
{
    if(c->isbelow != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_setabove(c, false);
            client_setontop(c, false);
            client_setfullscreen(c, false);
        }
        c->isbelow = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hooks_property(c, "below");
    }
}

/** Set a client modal, or not.
 * \param c The client.
 * \param s Set or not the client moda.
 */
void
client_setmodal(client_t *c, bool s)
{
    if(c->ismodal != s)
    {
        c->ismodal = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hooks_property(c, "modal");
    }
}

/** Set a client ontop, or not.
 * \param c The client.
 * \param s Set or not the client moda.
 */
void
client_setontop(client_t *c, bool s)
{
    if(c->isontop != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_setabove(c, false);
            client_setbelow(c, false);
            client_setfullscreen(c, false);
        }
        c->isontop = s;
        client_stack();
        /* execute hook */
        hooks_property(c, "ontop");
    }
}

/** Unban a client and move it back into the viewport.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    if(c->isbanned)
    {
        /* Move the client back where it belongs. */
        uint32_t request[] = { c->geometries.internal.x,
                               c->geometries.internal.y,
                               c->geometries.internal.width,
                               c->geometries.internal.height };

        xcb_configure_window(globalconf.connection, c->win,
                              XCB_CONFIG_WINDOW_X
                              | XCB_CONFIG_WINDOW_Y
                              | XCB_CONFIG_WINDOW_WIDTH
                              | XCB_CONFIG_WINDOW_HEIGHT,
                              request);
        window_configure(c->win, c->geometries.internal, c->border);

        c->isbanned = false;

        /* All the wiboxes (may) need to be repositioned. */
        if(client_hasstrut(c))
            wibox_update_positions();
    }
}

/** Unmanage a client.
 * \param c The client.
 */
void
client_unmanage(client_t *c)
{
    tag_array_t *tags = &globalconf.screens[c->screen].tags;

    /* Reset transient_for attributes of widows that maybe refering to us */
    for(client_t *tc = globalconf.clients; tc; tc = tc->next)
        if(tc->transient_for == c)
            tc->transient_for = NULL;

    if(globalconf.screens[c->phys_screen].client_focus == c)
        client_unfocus(c);

    /* remove client everywhere */
    client_list_detach(&globalconf.clients, c);
    stack_client_delete(c);
    for(int i = 0; i < tags->len; i++)
        untag_client(c, tags->tab[i]);

    /* call hook */
    if(globalconf.hooks.unmanage != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.unmanage, 1, 0);
    }

    /* Call hook to notify list change */
    if(globalconf.hooks.clients != LUA_REFNIL)
        luaA_dofunction(globalconf.L, globalconf.hooks.clients, 0, 0);

    /* The server grab construct avoids race conditions. */
    xcb_grab_server(globalconf.connection);

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, c->win,
                      XCB_BUTTON_MASK_ANY);
    window_state_set(c->win, XCB_WM_STATE_WITHDRAWN);

    xcb_flush(globalconf.connection);
    xcb_ungrab_server(globalconf.connection);

    titlebar_client_detach(c);

    ewmh_update_net_client_list(c->phys_screen);

    /* All the wiboxes (may) need to be repositioned. */
    if(client_hasstrut(c))
        wibox_update_positions();

    /* set client as invalid */
    c->invalid = true;

    client_unref(&c);
}

/** Kill a client via a WM_DELETE_WINDOW request or KillClient if not
 * supported.
 * \param c The client to kill.
 */
void
client_kill(client_t *c)
{
    if(window_hasproto(c->win, WM_DELETE_WINDOW))
    {
        xcb_client_message_event_t ev;

        /* Initialize all of event's fields first */
        p_clear(&ev, 1);

        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = c->win;
        ev.format = 32;
        ev.data.data32[1] = XCB_CURRENT_TIME;
        ev.type = WM_PROTOCOLS;
        ev.data.data32[0] = WM_DELETE_WINDOW;

        xcb_send_event(globalconf.connection, false, c->win,
                       XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    }
    else
        xcb_kill_client(globalconf.connection, c->win);
}

/** Get all clients into a table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam An optional screen nunmber.
 * \lreturn A table with all clients.
 */
static int
luaA_client_get(lua_State *L)
{
    int i = 1, screen;
    client_t *c;

    screen = luaL_optnumber(L, 1, 0) - 1;

    lua_newtable(L);

    if(screen == SCREEN_UNDEF)
        for(c = globalconf.clients; c; c = c->next)
        {
            luaA_client_userdata_new(globalconf.L, c);
            lua_rawseti(L, -2, i++);
        }
    else
    {
        luaA_checkscreen(screen);
        for(c = globalconf.clients; c; c = c->next)
            if(c->screen == screen)
            {
                luaA_client_userdata_new(globalconf.L, c);
                lua_rawseti(L, -2, i++);
            }
    }

    return 1;
}

/** Check if a client is visible on its screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lreturn A boolean value, true if the client is visible, false otherwise.
 */
static int
luaA_client_isvisible(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_pushboolean(L, client_isvisible(*c, (*c)->screen));
    return 1;
}

/** Set client border width.
 * \param c The client.
 * \param width The border width.
 */
void
client_setborder(client_t *c, int width)
{
    uint32_t w = width;

    if(width > 0 && (c->type == WINDOW_TYPE_DOCK
                     || c->type == WINDOW_TYPE_SPLASH
                     || c->type == WINDOW_TYPE_DESKTOP
                     || c->isfullscreen))
        return;

    if(width == c->border || width < 0)
        return;

    /* Update geometry with the new border. */
    c->geometry.width -= 2 * c->border;
    c->geometry.height -= 2 * c->border;

    c->border = width;
    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH, &w);

    c->geometry.width += 2 * c->border;
    c->geometry.height += 2 * c->border;
    /* Tiled clients will be resized by the layout functions. */
    client_need_arrange(c);

    /* Changing border size also affects the size of the titlebar. */
    titlebar_update_geometry(c);

    hooks_property(c, "border_width");
}

/** Kill a client.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_kill(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_kill(*c);
    return 0;
}

/** Swap a client with another one.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A client to swap with.
 */
static int
luaA_client_swap(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_t **swap = luaA_checkudata(L, 2, "client");

    if(*c != *swap)
    {
        client_list_swap(&globalconf.clients, *swap, *c);
        client_need_arrange(*c);
        client_need_arrange(*swap);

        /* Call hook to notify list change */
        if(globalconf.hooks.clients != LUA_REFNIL)
            luaA_dofunction(L, globalconf.hooks.clients, 0, 0);
    }

    return 0;
}

/** Access or set the client tags.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \lparam A table with tags to set, or none to get the current tags table.
 * \return The clients tag.
 */
static int
luaA_client_tags(lua_State *L)
{
    tag_array_t *tags;
    tag_t **tag;
    client_t **c = luaA_checkudata(L, 1, "client");
    int j = 0;

    if(lua_gettop(L) == 2)
    {
        luaA_checktable(L, 2);
        tags = &globalconf.screens[(*c)->screen].tags;
        for(int i = 0; i < tags->len; i++)
            untag_client(*c, tags->tab[i]);
        lua_pushnil(L);
        while(lua_next(L, 2))
        {
            tag = luaA_checkudata(L, -1, "tag");
            tag_client(*c, *tag);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    tags = &globalconf.screens[(*c)->screen].tags;
    lua_newtable(L);
    for(int i = 0; i < tags->len; i++)
        if(is_client_tagged(*c, tags->tab[i]))
        {
            luaA_tag_userdata_new(L, tags->tab[i]);
            lua_rawseti(L, -2, ++j);
        }

    return 1;
}

/** Raise a client on top of others which are on the same layer.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_raise(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_raise(*c);
    return 0;
}

/** Lower a client on bottom of others which are on the same layer.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_lower(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_lower(*c);
    return 0;
}

/** Redraw a client by unmapping and mapping it quickly.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_redraw(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");

    xcb_unmap_window(globalconf.connection, (*c)->win);
    xcb_map_window(globalconf.connection, (*c)->win);

    /* Set the focus on the current window if the redraw has been
       performed on the window where the pointer is currently on
       because after the unmapping/mapping, the focus is lost */
    if(globalconf.screen_focus->client_focus == *c)
    {
        client_unfocus(*c);
        client_focus(*c);
    }

    return 0;
}

/** Stop managing a client.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_unmanage(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_unmanage(*c);
    return 0;
}

/** Return client geometry.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new coordinates, or none.
 * \lreturn A table with client coordinates.
 */
static int
luaA_client_geometry(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");

    if(lua_gettop(L) == 2)
    {
        area_t geometry;

        luaA_checktable(L, 2);
        geometry.x = luaA_getopt_number(L, 2, "x", (*c)->geometry.x);
        geometry.y = luaA_getopt_number(L, 2, "y", (*c)->geometry.y);
        if(client_isfixed(*c))
        {
            geometry.width = (*c)->geometry.width;
            geometry.height = (*c)->geometry.height;
        }
        else
        {
            geometry.width = luaA_getopt_number(L, 2, "width", (*c)->geometry.width);
            geometry.height = luaA_getopt_number(L, 2, "height", (*c)->geometry.height);
        }

        client_resize(*c, geometry, (*c)->size_hints_honor);
    }

    return luaA_pusharea(L, (*c)->geometry);
}

/** Push a strut type to a table on stack.
 * \param L The Lua VM state.
 * \param struts The struts to push.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_pushstruts(lua_State *L, strut_t struts)
{
    lua_newtable(L);
    lua_pushnumber(L, struts.left);
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, struts.right);
    lua_setfield(L, -2, "right");
    lua_pushnumber(L, struts.top);
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, struts.bottom);
    lua_setfield(L, -2, "bottom");
    return 1;
}

/** Return client struts (reserved space at the edge of the screen).
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new strut values, or none.
 * \lreturn A table with strut values.
 */
static int
luaA_client_struts(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");

    if(lua_gettop(L) == 2)
    {
        strut_t struts;
        area_t screen_area = display_area_get((*c)->phys_screen, NULL, NULL);

        struts.left = luaA_getopt_number(L, 2, "left", (*c)->strut.left);
        struts.right = luaA_getopt_number(L, 2, "right", (*c)->strut.right);
        struts.top = luaA_getopt_number(L, 2, "top", (*c)->strut.top);
        struts.bottom = luaA_getopt_number(L, 2, "bottom", (*c)->strut.bottom);

        if(struts.left != (*c)->strut.left || struts.right != (*c)->strut.right ||
                struts.top != (*c)->strut.top || struts.bottom != (*c)->strut.bottom) {
            /* Struts are not so well defined in the context of xinerama. So we just
             * give the entire root window and let the window manager decide. */
            struts.left_start_y = 0;
            struts.left_end_y = !struts.left ? 0 : screen_area.height;
            struts.right_start_y = 0;
            struts.right_end_y = !struts.right ? 0 : screen_area.height;
            struts.top_start_x = 0;
            struts.top_end_x = !struts.top ? 0 : screen_area.width;
            struts.bottom_start_x = 0;
            struts.bottom_end_x = !struts.bottom ? 0 : screen_area.width;

            (*c)->strut = struts;

            ewmh_update_client_strut((*c));

            client_need_arrange((*c));
            /* All the wiboxes (may) need to be repositioned. */
            wibox_update_positions();

            hooks_property(*c, "struts");
        }
    }

    return luaA_pushstruts(L, (*c)->strut);
}

/** Client newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_client_newindex(lua_State *L)
{
    size_t len;
    client_t **c = luaA_checkudata(L, 1, "client");
    const char *buf = luaL_checklstring(L, 2, &len);
    bool b;
    double d;
    int i;
    wibox_t **t = NULL;
    image_t **image;

    if((*c)->invalid)
        luaL_error(L, "client is invalid\n");

    switch(a_tokenize(buf, len))
    {
      case A_TK_SCREEN:
        if(globalconf.xinerama_is_active)
        {
            i = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(i);
            screen_client_moveto(*c, i, true, true);
        }
        break;
      case A_TK_HIDE:
        b = luaA_checkboolean(L, 3);
        if(b != (*c)->ishidden)
        {
            client_need_arrange(*c);
            (*c)->ishidden = b;
            client_need_arrange(*c);
        }
        break;
      case A_TK_MINIMIZED:
        client_setminimized(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_FULLSCREEN:
        client_setfullscreen(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_MAXIMIZED_HORIZONTAL:
        client_setmaxhoriz(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_MAXIMIZED_VERTICAL:
        client_setmaxvert(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_ICON:
        image = luaA_checkudata(L, 3, "image");
        image_unref(&(*c)->icon);
        image_ref(image);
        (*c)->icon = *image;
        /* execute hook */
        hooks_property(*c, "icon");
        break;
      case A_TK_OPACITY:
        if(lua_isnil(L, 3))
            window_opacity_set((*c)->win, -1);
        else
        {
            d = luaL_checknumber(L, 3);
            if(d >= 0 && d <= 1)
                window_opacity_set((*c)->win, d);
        }
        break;
      case A_TK_STICKY:
        client_setsticky(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_SIZE_HINTS_HONOR:
        (*c)->size_hints_honor = luaA_checkboolean(L, 3);
        client_need_arrange(*c);
        break;
      case A_TK_BORDER_WIDTH:
        client_setborder(*c, luaL_checknumber(L, 3));
        break;
      case A_TK_ONTOP:
        client_setontop(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_ABOVE:
        client_setabove(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_BELOW:
        client_setbelow(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_URGENT:
        client_seturgent(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len))
           && xcolor_init_reply(xcolor_init_unchecked(&(*c)->border_color, buf, len)))
            xcb_change_window_attributes(globalconf.connection, (*c)->win,
                                         XCB_CW_BORDER_PIXEL, &(*c)->border_color.pixel);
        break;
      case A_TK_TITLEBAR:
        if(lua_isnil(L, 3))
            titlebar_client_detach(*c);
        else
        {
            t = luaA_checkudata(L, 3, "wibox");
            titlebar_client_attach(*c, *t);
        }
        break;
      default:
        return 0;
    }

    return 0;
}

/** Client object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield id The window X id.
 * \lfield name The client title.
 * \lfield skip_taskbar True if the client does not want to be in taskbar.
 * \lfield type The window type (desktop, normal, dock, …).
 * \lfield class The client class.
 * \lfield instance The client instance.
 * \lfield pid The client PID, if available.
 * \lfield role The window role, if available.
 * \lfield machine The machine client is running on.
 * \lfield icon_name The client name when iconified.
 * \lfield screen Client screen number.
 * \lfield hide Define if the client must be hidden, i.e. never mapped,
 * invisible in taskbar.
 * \lfield minimized Define it the client must be iconify, i.e. only visible in
 * taskbar.
 * \lfield size_hints_honor Honor size hints, i.e. respect size ratio.
 * \lfield border_width The client border width.
 * \lfield border_color The client border color.
 * \lfield titlebar The client titlebar.
 * \lfield urgent The client urgent state.
 * \lfield content An image representing the client window content (screenshot).
 * \lfield focus The focused client.
 * \lfield opacity The client opacity between 0 and 1.
 * \lfield ontop The client is on top of every other windows.
 * \lfield above The client is above normal windows.
 * \lfield below The client is below normal windows.
 * \lfield fullscreen The client is fullscreen or not.
 * \lfield maximized_horizontal The client is maximized horizontally or not.
 * \lfield maximized_vertical The client is maximized vertically or not.
 * \lfield transient_for Return the client the window is transient for.
 * \lfield group_id Identification unique to a group of windows.
 * \lfield leader_id Identification unique to windows spawned by the same command.
 * \lfield size_hints A table with size hints of the client: user_position,
 *         user_size, program_position, program_size, etc.
 */
static int
luaA_client_index(lua_State *L)
{
    size_t len;
    ssize_t slen;
    client_t **c = luaA_checkudata(L, 1, "client");
    const char *buf = luaL_checklstring(L, 2, &len);
    char *value;
    void *data;
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r = NULL;
    double d;

    if((*c)->invalid)
        luaL_error(L, "client is invalid\n");

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(buf, len))
    {
        image_t *image;

      case A_TK_NAME:
        lua_pushstring(L, (*c)->name);
        break;
      case A_TK_TRANSIENT_FOR:
        if((*c)->transient_for)
            return luaA_client_userdata_new(L, (*c)->transient_for);
        return 0;
      case A_TK_SKIP_TASKBAR:
        lua_pushboolean(L, (*c)->skiptb);
        break;
      case A_TK_CONTENT:
        if((image = client_getcontent(*c)))
            return luaA_image_userdata_new(L, image);
        return 0;
      case A_TK_TYPE:
        switch((*c)->type)
        {
          case WINDOW_TYPE_DESKTOP:
            lua_pushliteral(L, "desktop");
            break;
          case WINDOW_TYPE_DOCK:
            lua_pushliteral(L, "dock");
            break;
          case WINDOW_TYPE_SPLASH:
            lua_pushliteral(L, "splash");
            break;
          case WINDOW_TYPE_DIALOG:
            lua_pushliteral(L, "dialog");
            break;
          case WINDOW_TYPE_MENU:
            lua_pushliteral(L, "menu");
            break;
          case WINDOW_TYPE_TOOLBAR:
            lua_pushliteral(L, "toolbar");
            break;
          case WINDOW_TYPE_UTILITY:
            lua_pushliteral(L, "utility");
            break;
          case WINDOW_TYPE_DROPDOWN_MENU:
            lua_pushliteral(L, "dropdown_menu");
            break;
          case WINDOW_TYPE_POPUP_MENU:
            lua_pushliteral(L, "popup_menu");
            break;
          case WINDOW_TYPE_TOOLTIP:
            lua_pushliteral(L, "tooltip");
            break;
          case WINDOW_TYPE_NOTIFICATION:
            lua_pushliteral(L, "notification");
            break;
          case WINDOW_TYPE_COMBO:
            lua_pushliteral(L, "combo");
            break;
          case WINDOW_TYPE_DND:
            lua_pushliteral(L, "dnd");
            break;
          case WINDOW_TYPE_NORMAL:
            lua_pushliteral(L, "normal");
            break;
        }
        break;
      case A_TK_CLASS:
        lua_pushstring(L, (*c)->class);
        break;
      case A_TK_INSTANCE:
        lua_pushstring(L, (*c)->instance);
        break;
      case A_TK_ROLE:
        if(!xutil_text_prop_get(globalconf.connection, (*c)->win,
                                WM_WINDOW_ROLE, &value, &slen))
            return 0;
        lua_pushlstring(L, value, slen);
        p_delete(&value);
        break;
      case A_TK_PID:
        prop_c = xcb_get_property_unchecked(globalconf.connection, false, (*c)->win, _NET_WM_PID, CARDINAL, 0L, 1L);
        prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL);

        if(prop_r && prop_r->value_len && (data = xcb_get_property_value(prop_r)))
            lua_pushnumber(L, *(uint32_t *)data);
        else
        {
            p_delete(&prop_r);
            return 0;
        }
        break;
      case A_TK_ID:
        lua_pushnumber(L, (*c)->win);
        break;
      case A_TK_LEADER_ID:
        lua_pushnumber(L, (*c)->leader_win);
        break;
      case A_TK_MACHINE:
        if(!xutil_text_prop_get(globalconf.connection, (*c)->win,
                                WM_CLIENT_MACHINE, &value, &slen))
            return 0;
        lua_pushlstring(L, value, slen);
        p_delete(&value);
        break;
      case A_TK_ICON_NAME:
        lua_pushstring(L, (*c)->icon_name);
        break;
      case A_TK_SCREEN:
        lua_pushnumber(L, 1 + (*c)->screen);
        break;
      case A_TK_HIDE:
        lua_pushboolean(L, (*c)->ishidden);
        break;
      case A_TK_MINIMIZED:
        lua_pushboolean(L, (*c)->isminimized);
        break;
      case A_TK_FULLSCREEN:
        lua_pushboolean(L, (*c)->isfullscreen);
        break;
      case A_TK_GROUP_ID:
        if((*c)->group_win)
            lua_pushnumber(L, (*c)->group_win);
        else
            return 0;
        break;
      case A_TK_MAXIMIZED_HORIZONTAL:
        lua_pushboolean(L, (*c)->ismaxhoriz);
        break;
      case A_TK_MAXIMIZED_VERTICAL:
        lua_pushboolean(L, (*c)->ismaxvert);
        break;
      case A_TK_ICON:
        if((*c)->icon)
            luaA_image_userdata_new(L, (*c)->icon);
        else
            return 0;
        break;
      case A_TK_OPACITY:
        if((d = window_opacity_get((*c)->win)) >= 0)
            lua_pushnumber(L, d);
        else
            return 0;
        break;
      case A_TK_ONTOP:
        lua_pushboolean(L, (*c)->isontop);
        break;
      case A_TK_ABOVE:
        lua_pushboolean(L, (*c)->isabove);
        break;
      case A_TK_BELOW:
        lua_pushboolean(L, (*c)->isbelow);
        break;
      case A_TK_STICKY:
        lua_pushboolean(L, (*c)->issticky);
        break;
      case A_TK_SIZE_HINTS_HONOR:
        lua_pushboolean(L, (*c)->size_hints_honor);
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, (*c)->border);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &(*c)->border_color);
        break;
      case A_TK_TITLEBAR:
        if((*c)->titlebar)
            return luaA_wibox_userdata_new(L, (*c)->titlebar);
        return 0;
      case A_TK_URGENT:
        lua_pushboolean(L, (*c)->isurgent);
        break;
      case A_TK_SIZE_HINTS:
        {
            const char *u_or_p = NULL;

            lua_newtable(L);

            if((*c)->size_hints.flags & XCB_SIZE_HINT_US_POSITION)
                u_or_p = "user_position";
            else if((*c)->size_hints.flags & XCB_SIZE_HINT_P_POSITION)
                u_or_p = "program_position";

            if(u_or_p)
            {
                lua_newtable(L);
                lua_pushnumber(L, (*c)->size_hints.x);
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, (*c)->size_hints.y);
                lua_setfield(L, -2, "y");
                lua_setfield(L, -2, u_or_p);
                u_or_p = NULL;
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_US_SIZE)
                u_or_p = "user_size";
            else if((*c)->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
                u_or_p = "program_size";

            if(u_or_p)
            {
                lua_newtable(L);
                lua_pushnumber(L, (*c)->size_hints.width);
                lua_setfield(L, -2, "width");
                lua_pushnumber(L, (*c)->size_hints.height);
                lua_setfield(L, -2, "height");
                lua_setfield(L, -2, u_or_p);
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
            {
                lua_pushnumber(L, (*c)->size_hints.min_width);
                lua_setfield(L, -2, "min_width");
                lua_pushnumber(L, (*c)->size_hints.min_height);
                lua_setfield(L, -2, "min_height");
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_P_MAX_SIZE)
            {
                lua_pushnumber(L, (*c)->size_hints.max_width);
                lua_setfield(L, -2, "max_width");
                lua_pushnumber(L, (*c)->size_hints.max_height);
                lua_setfield(L, -2, "max_height");
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_P_RESIZE_INC)
            {
                lua_pushnumber(L, (*c)->size_hints.width_inc);
                lua_setfield(L, -2, "width_inc");
                lua_pushnumber(L, (*c)->size_hints.height_inc);
                lua_setfield(L, -2, "height_inc");
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_P_ASPECT)
            {
                lua_pushnumber(L, (*c)->size_hints.min_aspect_num);
                lua_setfield(L, -2, "min_aspect_num");
                lua_pushnumber(L, (*c)->size_hints.min_aspect_den);
                lua_setfield(L, -2, "min_aspect_den");
                lua_pushnumber(L, (*c)->size_hints.max_aspect_num);
                lua_setfield(L, -2, "max_aspect_num");
                lua_pushnumber(L, (*c)->size_hints.max_aspect_den);
                lua_setfield(L, -2, "max_aspect_den");
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_BASE_SIZE)
            {
                lua_pushnumber(L, (*c)->size_hints.base_width);
                lua_setfield(L, -2, "base_width");
                lua_pushnumber(L, (*c)->size_hints.base_height);
                lua_setfield(L, -2, "base_height");
            }

            if((*c)->size_hints.flags & XCB_SIZE_HINT_P_WIN_GRAVITY)
            {
                switch((*c)->size_hints.win_gravity)
                {
                  default:
                    lua_pushliteral(L, "north_west");
                    break;
                  case XCB_GRAVITY_NORTH:
                    lua_pushliteral(L, "north");
                    break;
                  case XCB_GRAVITY_NORTH_EAST:
                    lua_pushliteral(L, "north_east");
                    break;
                  case XCB_GRAVITY_WEST:
                    lua_pushliteral(L, "west");
                    break;
                  case XCB_GRAVITY_CENTER:
                    lua_pushliteral(L, "center");
                    break;
                  case XCB_GRAVITY_EAST:
                    lua_pushliteral(L, "east");
                    break;
                  case XCB_GRAVITY_SOUTH_WEST:
                    lua_pushliteral(L, "south_west");
                    break;
                  case XCB_GRAVITY_SOUTH:
                    lua_pushliteral(L, "south");
                    break;
                  case XCB_GRAVITY_SOUTH_EAST:
                    lua_pushliteral(L, "south_east");
                    break;
                  case XCB_GRAVITY_STATIC:
                    lua_pushliteral(L, "static");
                    break;
                }
                lua_setfield(L, -2, "win_gravity");
            }
        }
        break;
      default:
        return 0;
    }

    return 1;
}

/** Get or set mouse buttons bindings for a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects of this client.
 */
static int
luaA_client_buttons(lua_State *L)
{
    client_t **client = luaA_checkudata(L, 1, "client");
    button_array_t *buttons = &(*client)->buttons;

    if(lua_gettop(L) == 2)
        luaA_button_array_set(L, 2, buttons);

    window_buttons_grab((*client)->win, &(*client)->buttons);

    return luaA_button_array_get(L, buttons);
}

/** Get or set keys bindings for a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of key bindings objects, or nothing.
 * \return The array of key bindings objects of this client.
 */
static int
luaA_client_keys(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    keybindings_t *keys = &(*c)->keys;

    if(lua_gettop(L) == 2)
    {
        luaA_key_array_set(L, 2, keys);
        xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, (*c)->win, XCB_BUTTON_MASK_ANY);
        window_grabkeys((*c)->win, keys);
    }

    return luaA_key_array_get(L, keys);
}

/** Send fake events to a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \param The event type: key_press, key_release, button_press, button_release
 * or motion_notify.
 * \param The detail: in case of a key event, this is the keycode to send, in
 * case of a button event this is the number of the button. In case of a motion
 * event, this is a boolean value which if true make the coordinates relatives.
 * \param In case of a motion event, this is the X coordinate.
 * \param In case of a motion event, this is the Y coordinate.
 */
static int
luaA_client_fake_input(lua_State *L)
{
    if(!globalconf.have_xtest)
    {
        luaA_warn(L, "XTest extension is not available, cannot fake input.");
        return 0;
    }

    client_t **c = luaA_checkudata(L, 1, "client");
    size_t tlen;
    const char *stype = luaL_checklstring(L, 2, &tlen);
    uint8_t type, detail;
    int x = 0, y = 0;

    switch(a_tokenize(stype, tlen))
    {
      case A_TK_KEY_PRESS:
        type = XCB_KEY_PRESS;
        detail = luaL_checknumber(L, 3); /* keycode */
        break;
      case A_TK_KEY_RELEASE:
        type = XCB_KEY_RELEASE;
        detail = luaL_checknumber(L, 3); /* keycode */
        break;
      case A_TK_BUTTON_PRESS:
        type = XCB_BUTTON_PRESS;
        detail = luaL_checknumber(L, 3); /* button number */
        break;
      case A_TK_BUTTON_RELEASE:
        type = XCB_BUTTON_RELEASE;
        detail = luaL_checknumber(L, 3); /* button number */
        break;
      case A_TK_MOTION_NOTIFY:
        type = XCB_MOTION_NOTIFY;
        detail = luaA_checkboolean(L, 3); /* relative to the current position or not */
        x = luaL_checknumber(L, 4);
        y = luaL_checknumber(L, 5);
        break;
      default:
        return 0;
    }

    xcb_test_fake_input(globalconf.connection,
                        type,
                        detail,
                        XCB_CURRENT_TIME,
                        (*c)->win,
                        x, y,
                        0);
    return 0;
}

/* Client module.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_index(lua_State *L)
{
    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(buf, len))
    {
      case A_TK_FOCUS:
        if(globalconf.screen_focus->client_focus)
            luaA_client_userdata_new(L, globalconf.screen_focus->client_focus);
        else
            return 0;
        break;
      default:
        return 0;
    }

    return 1;
}

/* Client module new index.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_newindex(lua_State *L)
{
    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);
    client_t **c;

    switch(a_tokenize(buf, len))
    {
      case A_TK_FOCUS:
        c = luaA_checkudata(L, 3, "client");
        client_focus(*c);
        break;
      default:
        break;
    }

    return 0;
}

const struct luaL_reg awesome_client_methods[] =
{
    { "get", luaA_client_get },
    { "__index", luaA_client_module_index },
    { "__newindex", luaA_client_module_newindex },
    { NULL, NULL }
};
const struct luaL_reg awesome_client_meta[] =
{
    { "isvisible", luaA_client_isvisible },
    { "geometry", luaA_client_geometry },
    { "struts", luaA_client_struts },
    { "buttons", luaA_client_buttons },
    { "keys", luaA_client_keys },
    { "tags", luaA_client_tags },
    { "kill", luaA_client_kill },
    { "swap", luaA_client_swap },
    { "raise", luaA_client_raise },
    { "lower", luaA_client_lower },
    { "redraw", luaA_client_redraw },
    { "unmanage", luaA_client_unmanage },
    { "fake_input", luaA_client_fake_input },
    { "__index", luaA_client_index },
    { "__newindex", luaA_client_newindex },
    { "__eq", luaA_client_eq },
    { "__gc", luaA_client_gc },
    { "__tostring", luaA_client_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
