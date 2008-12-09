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

#include <xcb/xcb_atom.h>

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

extern awesome_t globalconf;

DO_LUA_NEW(extern, client_t, client, "client", client_ref)
DO_LUA_EQ(client_t, client, "client")
DO_LUA_GC(client_t, client, "client", client_unref)

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any.
 * \param c A client pointer.
 * \param screen A virtual screen.
 * \return True if client had property, false otherwise.
 */
static bool
client_loadprops(client_t * c, screen_t *screen)
{
    ssize_t len;
    tag_array_t *tags = &screen->tags;
    char *prop = NULL;
    xcb_get_property_cookie_t fullscreen_q;
    xcb_get_property_reply_t *reply;
    void *data;

    if(!xutil_text_prop_get(globalconf.connection, c->win, _AWESOME_TAGS,
                            &prop, &len))
        return false;

    /* Send the GetProperty requests which will be processed later */
    fullscreen_q = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                             _AWESOME_FULLSCREEN, CARDINAL, 0, 1);

    /* ignore property if the tag count isn't matching */
    if(len == tags->len)
        for(int i = 0; i < tags->len; i++)
            if(prop[i] == '1')
                tag_client(c, tags->tab[i]);
            else
                untag_client(c, tags->tab[i]);

    p_delete(&prop);

    /* check for fullscreen */
    reply = xcb_get_property_reply(globalconf.connection, fullscreen_q, NULL);

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        client_setfullscreen(c, *(bool *) data);
    p_delete(&reply);

    return true;
}

/** Check if client supports protocol a protocole in WM_PROTOCOL.
 * \param win The window.
 * \return True if client has the atom in protocol, false otherwise.
 */
static bool
window_hasproto(xcb_window_t win, xcb_atom_t atom)
{
    uint32_t i;
    xcb_get_wm_protocols_reply_t protocols;
    bool ret = false;

    if(xcb_get_wm_protocols_reply(globalconf.connection,
                                  xcb_get_wm_protocols_unchecked(globalconf.connection,
                                                                 win, WM_PROTOCOLS),
                                  &protocols, NULL))
    {
        for(i = 0; !ret && i < protocols.atoms_len; i++)
            if(protocols.atoms[i] == atom)
                ret = true;
        xcb_get_wm_protocols_reply_wipe(&protocols);
    }
    return ret;
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

/** Unfocus a client.
 * \param c The client.
 */
static void
client_unfocus(client_t *c)
{
    xcb_window_t root_win = xutil_screen_get(globalconf.connection, c->phys_screen)->root;
    globalconf.screens[c->phys_screen].client_focus = NULL;

    /* Set focus on root window, so no events leak to the current window. */
    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
        root_win, XCB_CURRENT_TIME);

    /* Call hook */
    if(globalconf.hooks.unfocus != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.unfocus, 1, 0);
    }

    ewmh_update_net_active_window(c->phys_screen);
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
        uint32_t request[2] = { - (c->geometry.width + 2 * c->border),
                                - (c->geometry.height + 2 * c->border) };

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             request);

        /* Do it manually because client geometry remains unchanged. */
        if (c->titlebar)
        {
            simple_window_t *sw = &c->titlebar->sw;

            if (sw->window)
            {
                request[0] = - (sw->geometry.width);
                request[1] = - (sw->geometry.height);
                /* Move the titlebar to the same place as the window. */
                xcb_configure_window(globalconf.connection, sw->window,
                                     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                     request);
            }
        }

        c->isbanned = true;

        /* All the wiboxes (may) need to be repositioned. */
        if(client_hasstrut(c))
            wibox_update_positions();
    }

    /* Wait until the last moment to take away the focus from the window. */
    if (globalconf.screens[c->phys_screen].client_focus == c)
        client_unfocus(c);
}

/** Give focus to client, or to first client if client is NULL.
 * \param c The client or NULL.
 * \return True if a window (even root) has received focus, false otherwise.
 */
static void
client_focus(client_t *c)
{
    if(!client_maybevisible(c, c->screen) || c->nofocus)
        return;

    /* unfocus current selected client */
    if(globalconf.screen_focus->client_focus
       && c != globalconf.screen_focus->client_focus)
        client_unfocus(globalconf.screen_focus->client_focus);

    /* stop hiding c */
    c->ishidden = false;
    client_setminimized(c, false);

    /* unban the client before focusing or it will fail */
    client_unban(c);

    globalconf.screen_focus = &globalconf.screens[c->phys_screen];
    globalconf.screen_focus->client_focus = c;

    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                        c->win, XCB_CURRENT_TIME);

    /* Some layouts use focused client differently, so call them back.
     * And anyway, we have maybe unhidden */
    client_need_arrange(c);

    /* execute hook */
    if(globalconf.hooks.focus != LUA_REFNIL)
    {
        luaA_client_userdata_new(globalconf.L, globalconf.screen_focus->client_focus);
        luaA_dofunction(globalconf.L, globalconf.hooks.focus, 1, 0);
    }

    ewmh_update_net_active_window(c->phys_screen);
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
        {
            client_stack_above(node->client,
                               previous);
            previous = node->client->win;
        }

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
    LAYER_OUTOFSPACE
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
      case WINDOW_TYPE_DOCK:
        return LAYER_ABOVE;
      case WINDOW_TYPE_DESKTOP:
        return LAYER_DESKTOP;
      case WINDOW_TYPE_DIALOG:
      case WINDOW_TYPE_MENU:
      case WINDOW_TYPE_TOOLBAR:
      case WINDOW_TYPE_UTILITY:
        return LAYER_ABOVE;
      default:
        break;
    }

    return LAYER_NORMAL;
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
void
client_stack()
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

    /* stack bottom layers */
    for(layer = LAYER_BELOW; layer < LAYER_FULLSCREEN; layer++)
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

    /* finally stack ontop and fullscreen windows */
    for(layer = LAYER_FULLSCREEN; layer < LAYER_OUTOFSPACE; layer++)
        for(node = last; node; node = node->prev)
            if(client_layer_translator(node->client) == layer)
                config_win_vals[0] = client_stack_above(node->client,
                                                        config_win_vals[0]);
}

/** Copy tags from one client to another client on the same screen.
 * Be careful: this does not untag!
 * \param src_c The source client.
 * \param dst_c The destination client.
 */
static void
client_duplicate_tags(client_t *src_c, client_t *dst_c)
{
    int i;
    tag_array_t *tags = &globalconf.screens[src_c->screen].tags;

    /* Avoid doing dangerous things. */
    if (src_c->screen != dst_c->screen)
        return;

    for(i = 0; i < tags->len; i++)
        if(is_client_tagged(src_c, tags->tab[i]))
            tag_client(dst_c, tags->tab[i]);
}

/** Manage a new client.
 * \param w The window.
 * \param wgeom Window geometry.
 * \param phys_screen Physical screen number.
 * \param screen Virtual screen number where to manage client.
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, int phys_screen, int screen)
{
    xcb_get_property_cookie_t ewmh_icon_cookie;
    client_t *c, *tc = NULL, *group = NULL;
    image_t *icon;
    const uint32_t select_input_val[] =
    {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE
            | XCB_EVENT_MASK_ENTER_WINDOW
            | XCB_EVENT_MASK_LEAVE_WINDOW
    };


    if(systray_iskdedockapp(w))
    {
        systray_request_handle(w, phys_screen, NULL);
        return;
    }

    /* Send request to get NET_WM_ICON property as soon as possible... */
    ewmh_icon_cookie = ewmh_window_icon_get_unchecked(w);
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);
    c = p_new(client_t, 1);

    c->screen = screen_getbycoord(screen, wgeom->x, wgeom->y);

    c->phys_screen = phys_screen;

    /* Initial values */
    c->win = w;
    c->geometry.x = wgeom->x;
    c->geometry.y = wgeom->y;
    c->geometry.width = wgeom->width;
    c->geometry.height = wgeom->height;
    client_setborder(c, wgeom->border_width);
    if((icon = ewmh_window_icon_get_reply(ewmh_icon_cookie)))
        c->icon = image_ref(&icon);

    /* we honor size hints by default */
    c->honorsizehints = true;

    /* update hints */
    property_update_wm_normal_hints(c, NULL);
    property_update_wm_hints(c, NULL);
    property_update_wm_transient_for(c, NULL);
    property_update_wm_client_leader(c, NULL);

    /* Recursively find the parent. */
    for(tc = c; tc->transient_for; tc = tc->transient_for);
    if(tc != c)
        screen = tc->screen;

    /* Try to load props, if it fails check for group windows.
     * transient_for windows are excluded, because they inherit the parent tags. */
    if(!client_loadprops(c, &globalconf.screens[screen])
       && c->group_win
       && !c->transient_for)
        /* Try to find a group of windows for better initial placement. */
        for(group = globalconf.clients; group; group = group->next)
            if(group->group_win == c->group_win)
                break;

    /* Move to the right screen.
     * Assumption: Window groups do not span multiple logical screens. */
    if(group)
        screen = group->screen;

    /* Then check clients hints */
    ewmh_client_check_hints(c);

    /* move client to screen, but do not tag it for now */
    screen_client_moveto(c, screen, false, true);

    /* Duplicate the tags of either the parent or a group window.
     * Otherwise check for existing tags, if that fails tag it with the current tag.
     * This could be done lua side, but it's sane behaviour.  */
    if (!c->issticky)
    {
        if(c->transient_for)
            client_duplicate_tags(tc, c);
        else if (group)
            client_duplicate_tags(group, c);
        else
        {
            int i;
            tag_array_t *tags = &globalconf.screens[screen].tags;
            for(i = 0; i < tags->len; i++)
                if(is_client_tagged(c, tags->tab[i]))
                    break;

            /* if no tag, set current selected */
            if(i == tags->len)
                for(i = 0; i < tags->len; i++)
                    if(tags->tab[i]->selected)
                        tag_client(c, tags->tab[i]);
        }
    }

    /* Push client in client list */
    client_list_push(&globalconf.clients, client_ref(&c));

    /* Push client in stack */
    client_raise(c);

    /* update window title */
    property_update_wm_name(c);
    property_update_wm_icon_name(c);

    /* update strut */
    ewmh_client_strut_update(c, NULL);

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
        luaA_dofunction(globalconf.L, globalconf.hooks.manage, 1, 0);
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
    double dx, dy, max, min, ratio;

    if(c->minay > 0 && c->maxay > 0 && (geometry.height - c->baseh) > 0
       && (geometry.width - c->basew) > 0)
    {
        dx = (double) (geometry.width - c->basew);
        dy = (double) (geometry.height - c->baseh);
        min = (double) (c->minax) / (double) (c->minay);
        max = (double) (c->maxax) / (double) (c->maxay);
        ratio = dx / dy;
        if(max > 0 && min > 0 && ratio > 0)
        {
            if(ratio < min)
            {
                dy = (dx * min + dy) / (min * min + 1);
                dx = dy * min;
                geometry.width = (int) dx + c->basew;
                geometry.height = (int) dy + c->baseh;
            }
            else if(ratio > max)
            {
                dy = (dx * min + dy) / (max * max + 1);
                dx = dy * min;
                geometry.width = (int) dx + c->basew;
                geometry.height = (int) dy + c->baseh;
            }
        }
    }
    if(c->minw && geometry.width < c->minw)
        geometry.width = c->minw;
    if(c->minh && geometry.height < c->minh)
        geometry.height = c->minh;
    if(c->maxw && geometry.width > c->maxw)
        geometry.width = c->maxw;
    if(c->maxh && geometry.height > c->maxh)
        geometry.height = c->maxh;
    if(c->incw)
        geometry.width -= (geometry.width - c->basew) % c->incw;
    if(c->inch)
        geometry.height -= (geometry.height - c->baseh) % c->inch;

    return geometry;
}

/** Resize client window.
 * \param c Client to resize.
 * \param geometry New window geometry.
 * \param hints Use size hints.
 */
void
client_resize(client_t *c, area_t geometry, bool hints)
{
    int new_screen;
    area_t area;

    if(hints)
        geometry = client_geometry_hints(c, geometry);

    if(geometry.width <= 0 || geometry.height <= 0)
        return;

    /* offscreen appearance fixes */
    area = display_area_get(c->phys_screen, NULL,
                            &globalconf.screens[c->screen].padding);

    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width - 2 * c->border;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height - 2 * c->border;
    if(geometry.x + geometry.width + 2 * c->border < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height + 2 * c->border < 0)
        geometry.y = 0;

    if(c->geometry.x != geometry.x
       || c->geometry.y != geometry.y
       || c->geometry.width != geometry.width
       || c->geometry.height != geometry.height)
    {
        new_screen = screen_getbycoord(c->screen, geometry.x, geometry.y);

        /* Values to configure a window is an array where values are
         * stored according to 'value_mask' */
        uint32_t values[4];

        c->geometry.x = values[0] = geometry.x;
        c->geometry.y = values[1] = geometry.y;
        c->geometry.width = values[2] = geometry.width;
        c->geometry.height = values[3] = geometry.height;

        titlebar_update_geometry(c);

        /* The idea is to give a client a resize even when banned. */
        /* We just have to move the (x,y) to keep it out of the viewport. */
        /* This at least doesn't break expectations about events. */
        if (c->isbanned)
        {
            geometry.x = values[0] = - (geometry.width + 2 * c->border);
            geometry.y = values[1] = - (geometry.height + 2 * c->border);
        }

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                             | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             values);
        window_configure(c->win, geometry, c->border);

        screen_client_moveto(c, new_screen, true, false);

        /* execute hook */
        hooks_property(c, "geometry");
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

        /* become fullscreen! */
        if((c->isfullscreen = s))
        {
            /* remove any max state */
            client_setmaxhoriz(c, false);
            client_setmaxvert(c, false);

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
        xcb_change_property(globalconf.connection,
                            XCB_PROP_MODE_REPLACE,
                            c->win, _AWESOME_FULLSCREEN, CARDINAL, 8, 1,
                            &c->isfullscreen);
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
            /* Remove space needed for titlebar and border. */
            geometry = titlebar_geometry_remove(c->titlebar,
                                        c->border,
                                        geometry);
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

        client_resize(c, geometry, c->honorsizehints);
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
            /* Remove space needed for titlebar and border. */
            geometry = titlebar_geometry_remove(c->titlebar,
                                        c->border,
                                        geometry);
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

        client_resize(c, geometry, c->honorsizehints);
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
        c->isontop = s;
        client_stack();
        /* execute hook */
        hooks_property(c, "ontop");
    }
}

/** Save client properties as an X property.
 * \param c The client.
 */
void
client_saveprops_tags(client_t *c)
{
    tag_array_t *tags = &globalconf.screens[c->screen].tags;
    unsigned char *prop = p_alloca(unsigned char, tags->len + 1);
    int i;

    for(i = 0; i < tags->len; i++)
        prop[i] = is_client_tagged(c, tags->tab[i]) ? '1' : '0';

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, c->win, _AWESOME_TAGS, STRING, 8, i, prop);
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
        uint32_t request[2] = { c->geometry.x, c->geometry.y };

        xcb_configure_window(globalconf.connection, c->win,
                              XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                              request);
        window_configure(c->win, c->geometry, c->border);

        /* Do this manually because the system doesn't know we moved the toolbar.
         * Note that !isvisible titlebars are unmapped and for fullscreen it'll
         * end up offscreen anyway. */
        if(c->titlebar)
        {
            simple_window_t *sw = &c->titlebar->sw;
            /* All resizing is done, so only move now. */
            request[0] = sw->geometry.x;
            request[1] = sw->geometry.y;

            xcb_configure_window(globalconf.connection, sw->window,
                                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                                request);
        }

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

    /* delete properties */
    xcb_delete_property(globalconf.connection, c->win, _AWESOME_TAGS);
    xcb_delete_property(globalconf.connection, c->win, _AWESOME_FLOATING);

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

    c->border = width;
    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH, &w);

    client_need_arrange(c);

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
    luaA_otable_new(L);
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
        xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                            (*c)->win, XCB_CURRENT_TIME);

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
 * \param full Use titlebar also.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_handlegeom(lua_State *L, bool full)
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

        if(full)
            geometry = titlebar_geometry_remove((*c)->titlebar,
                                                (*c)->border,
                                                geometry);

        client_resize(*c, geometry, (*c)->honorsizehints);
    }

    if(full)
        return luaA_pusharea(L, titlebar_geometry_add((*c)->titlebar,
                                                      (*c)->border,
                                                      (*c)->geometry));

    return luaA_pusharea(L, (*c)->geometry);
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
    return luaA_client_handlegeom(L, false);
}

/** Return client geometry, using also titlebar and border width.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new coordinates, or none.
 * \lreturn A table with client coordinates.
 */
static int
luaA_client_fullgeometry(lua_State *L)
{
    return luaA_client_handlegeom(L, true);
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
      case A_TK_MINIMIZE:
        luaA_deprecate(L, "client.minimized");
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
      case A_TK_HONORSIZEHINTS:
        (*c)->honorsizehints = luaA_checkboolean(L, 3);
        client_need_arrange(*c);
        break;
      case A_TK_BORDER_WIDTH:
        client_setborder(*c, luaL_checknumber(L, 3));
        break;
      case A_TK_ONTOP:
        client_setontop(*c, luaA_checkboolean(L, 3));
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
 * \lfield minimize Define it the client must be iconify, i.e. only visible in
 * taskbar.
 * \lfield icon_path Path to the icon used to identify.
 * \lfield honorsizehints Honor size hints, i.e. respect size ratio.
 * \lfield border_width The client border width.
 * \lfield border_color The client border color.
 * \lfield titlebar The client titlebar.
 * \lfield urgent The client urgent state.
 * \lfield focus The focused client.
 * \lfield opacity The client opacity between 0 and 1.
 * \lfield ontop The client is on top of every other windows.
 * \lfield fullscreen The client is fullscreen or not.
 * \lfield maximized_horizontal The client is maximized horizontally or not.
 * \lfield maximized_vertical The client is maximized vertically or not.
 * \lfield transient_for Return the client the window is transient for.
 * \lfield group_id Identification unique to a group of windows.
 * \lfield leader_id Identification unique to windows spawned by the same command.
 * \lfield size_hints A table with size hints of the client: user_position,
 *         user_size, program_position and program_size.
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
    xcb_get_wm_class_reply_t hint;
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r = NULL;
    double d;

    if((*c)->invalid)
        luaL_error(L, "client is invalid\n");

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(buf, len))
    {
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
          default:
            lua_pushliteral(L, "normal");
            break;
        }
        break;
      case A_TK_CLASS:
        if(!xcb_get_wm_class_reply(globalconf.connection,
                                   xcb_get_wm_class_unchecked(globalconf.connection, (*c)->win),
                                   &hint, NULL))
             return 0;
        lua_pushstring(L, hint.class);
        xcb_get_wm_class_reply_wipe(&hint);
        break;
      case A_TK_INSTANCE:
        if(!xcb_get_wm_class_reply(globalconf.connection,
                                   xcb_get_wm_class_unchecked(globalconf.connection, (*c)->win),
                                   &hint, NULL))
            return 0;
        lua_pushstring(L, hint.name);
        xcb_get_wm_class_reply_wipe(&hint);
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
      case A_TK_MINIMIZE:
        luaA_deprecate(L, "client.minimized");
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
      case A_TK_STICKY:
        lua_pushboolean(L, (*c)->issticky);
        break;
      case A_TK_HONORSIZEHINTS:
        lua_pushboolean(L, (*c)->honorsizehints);
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
        lua_newtable(L);
        lua_pushboolean(L, (*c)->size_hints.flags & XCB_SIZE_HINT_US_POSITION);
        lua_setfield(L, -2, "user_position");
        lua_pushboolean(L, (*c)->size_hints.flags & XCB_SIZE_HINT_US_SIZE);
        lua_setfield(L, -2, "user_size");
        lua_pushboolean(L, (*c)->size_hints.flags & XCB_SIZE_HINT_P_POSITION);
        lua_setfield(L, -2, "program_position");
        lua_pushboolean(L, (*c)->size_hints.flags & XCB_SIZE_HINT_P_SIZE);
        lua_setfield(L, -2, "program_size");
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

    return luaA_button_array_get(L, buttons);
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

/** Move a client with mouse (DEPRECATED).
 * \param L The Lua VM state.
 */
static int
luaA_client_mouse_move(lua_State *L)
{
    luaA_deprecate(L, "awful.mouse.client.move()");
    return 0;
}

/** Resize a client with mouse (DEPRECATED).
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_mouse_resize(lua_State *L)
{
    luaA_deprecate(L, "awful.mouse.client.resize()");
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
    { "fullgeometry", luaA_client_fullgeometry },
    { "buttons", luaA_client_buttons },
    { "tags", luaA_client_tags },
    { "kill", luaA_client_kill },
    { "swap", luaA_client_swap },
    { "raise", luaA_client_raise },
    { "lower", luaA_client_lower },
    { "redraw", luaA_client_redraw },
    { "mouse_resize", luaA_client_mouse_resize },
    { "mouse_move", luaA_client_mouse_move },
    { "unmanage", luaA_client_unmanage },
    { "__index", luaA_client_index },
    { "__newindex", luaA_client_newindex },
    { "__eq", luaA_client_eq },
    { "__gc", luaA_client_gc },
    { "__tostring", luaA_client_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
