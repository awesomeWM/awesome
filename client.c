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

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

#include "client.h"
#include "tag.h"
#include "window.h"
#include "ewmh.h"
#include "widget.h"
#include "screen.h"
#include "titlebar.h"
#include "lua.h"
#include "stack.h"
#include "mouse.h"
#include "systray.h"
#include "layouts/floating.h"
#include "common/markup.h"
#include "common/atoms.h"

extern awesome_t globalconf;
extern const name_func_link_t FloatingPlacementList[];

/** Create a new client userdata.
 * \param L The Lua VM state.
 * \param p A client pointer.
 * \param The number of elements pushed on the stack.
 */
int
luaA_client_userdata_new(lua_State *L, client_t *p)
{
    client_t **pp = lua_newuserdata(L, sizeof(client_t *));
    *pp = p;
    client_ref(pp);
    return luaA_settype(L, "client");
}

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

    if(!xutil_text_prop_get(globalconf.connection, c->win, _AWESOME_PROPERTIES,
                            &prop, &len))
        return false;

    if(len != tags->len + 2)
    {
        /* ignore property if the tag count isn't matching */
        p_delete(&prop);
        return false;
    }

    for(int i = 0; i < tags->len; i++)
        if(prop[i] == '1')
            tag_client(c, tags->tab[i]);
        else
            untag_client(c, tags->tab[i]);

    client_setlayer(c, prop[tags->len + 1] - '0');
    client_setfloating(c, prop[tags->len] == '1');
    p_delete(&prop);
    return true;
}

/** Check if client supports protocol WM_DELETE_WINDOW.
 * \param win The window.
 * \return True if client has WM_DELETE_WINDOW, false otherwise.
 */
static bool
window_isprotodel(xcb_window_t win)
{
    uint32_t i, n;
    xcb_atom_t *protocols;
    bool ret = false;

    if(xcb_get_wm_protocols(globalconf.connection, win, &n, &protocols))
    {
        for(i = 0; !ret && i < n; i++)
            if(protocols[i] == WM_DELETE_WINDOW)
                ret = true;
        p_delete(&protocols);
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
        tag_array_t *tags = &globalconf.screens[screen].tags;
        for(int i = 0; i < tags->len; i++)
            if(tags->tab[i]->selected && is_client_tagged(c, tags->tab[i]))
                return true;
    }
    return false;
}

/** Returns true if a client is tagged
 * with one of the tags of the specified screen and is not hidden.
 * \param c The client to check.
 * \param screen Virtual screen number.
 * \return true if the client is visible, false otherwise.
 */
bool
client_isvisible(client_t *c, int screen)
{
    return (!c->ishidden && client_maybevisible(c, screen));
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

/** Update client name attribute with its new title.
 * \param c The client.
 * \param Return true if it has been updated.
 */
bool
client_updatetitle(client_t *c)
{
    char *name, *utf8;
    ssize_t len;

    if(!xutil_text_prop_get(globalconf.connection, c->win, _NET_WM_NAME, &name, &len))
        if(!xutil_text_prop_get(globalconf.connection, c->win, WM_NAME, &name, &len))
            return false;

    p_delete(&c->name);

    if((utf8 = draw_iso2utf8(name, len)))
        c->name = utf8;
    else
        c->name = name;

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.titleupdate, 1, 0);

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);

    return true;
}

/** Unfocus a client.
 * \param c The client.
 */
static void
client_unfocus(client_t *c)
{
    globalconf.screens[c->screen].client_focus = NULL;

    /* Call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.unfocus, 1, 0);

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    ewmh_update_net_active_window(c->phys_screen);
}

/** Ban client and unmap it.
 * \param c The client.
 */
void
client_ban(client_t *c)
{
    if(globalconf.screen_focus->client_focus == c)
        client_unfocus(c);
    xcb_unmap_window(globalconf.connection, c->win);
    if(c->ishidden)
        window_state_set(c->win, XCB_WM_ICONIC_STATE);
    else
        window_state_set(c->win, XCB_WM_WITHDRAWN_STATE);
    if(c->titlebar && c->titlebar->position && c->titlebar->sw)
        xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
}

/** Give focus to client, or to first client if client is NULL.
 * \param c The client or NULL.
 * \return True if a window (even root) has received focus, false otherwise.
 */
static void
client_focus(client_t *c)
{
    if(!client_maybevisible(c, c->screen))
        return;

    /* unfocus current selected client */
    if(globalconf.screen_focus->client_focus
       && c != globalconf.screen_focus->client_focus)
        client_unfocus(globalconf.screen_focus->client_focus);

    /* stop hiding c */
    c->ishidden = false;

    /* unban the client before focusing or it will fail */
    client_unban(c);

    globalconf.screen_focus = &globalconf.screens[c->screen];
    globalconf.screen_focus->client_focus = c;

    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                        c->win, XCB_CURRENT_TIME);

    /* Some layouts use focused client differently, so call them back.
     * And anyway, we have maybe unhidden */
    globalconf.screens[c->screen].need_arrange = true;

    /* execute hook */
    luaA_client_userdata_new(globalconf.L, globalconf.screen_focus->client_focus);
    luaA_dofunction(globalconf.L, globalconf.hooks.focus, 1, 0);

    ewmh_update_net_active_window(c->phys_screen);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
void
client_stack(void)
{
    uint32_t config_win_vals[2];
    client_node_t *node;
    layer_t layer;
    statusbar_t *sb;
    int screen;
    xembed_window_t *emwin;

    config_win_vals[0] = XCB_NONE;
    config_win_vals[1] = XCB_STACK_MODE_BELOW;

    /* first stack fullscreen and modal windows */
    for(layer = LAYER_OUTOFSPACE - 1; layer >= LAYER_FULLSCREEN; layer--)
        for(node = globalconf.stack; node; node = node->next)
            if(node->client->layer == layer)
            {
                if(node->client->titlebar
                   && node->client->titlebar->sw
                   && node->client->titlebar->position)
                {
                    xcb_configure_window(globalconf.connection,
                                         node->client->titlebar->sw->window,
                                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                         config_win_vals);
                    config_win_vals[0] = node->client->titlebar->sw->window;
                }
                xcb_configure_window(globalconf.connection, node->client->win,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = node->client->win;
            }

    /* then stack systray windows */
    for(emwin = globalconf.embedded; emwin; emwin = emwin->next)
    {
        xcb_configure_window(globalconf.connection,
                             emwin->win,
                             XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                             config_win_vals);
        config_win_vals[0] = emwin->win;
    }

    /* then stack statusbar window */
    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
            if(sb->sw)
            {
                xcb_configure_window(globalconf.connection,
                                     sb->sw->window,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = sb->sw->window;
            }

    /* finally stack everything else */
    for(layer = LAYER_FULLSCREEN - 1; layer >= LAYER_DESKTOP; layer--)
        for(node = globalconf.stack; node; node = node->next)
            if(node->client->layer == layer)
            {
                if(node->client->titlebar
                   && node->client->titlebar->sw
                   && node->client->titlebar->position)
                {
                    xcb_configure_window(globalconf.connection,
                                         node->client->titlebar->sw->window,
                                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                         config_win_vals);
                    config_win_vals[0] = node->client->titlebar->sw->window;
                }
                xcb_configure_window(globalconf.connection, node->client->win,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = node->client->win;
            }
}

/** Put client on top of the stack
 * \param c The client to raise.
 */
void
client_raise(client_t *c)
{
    /* Push c on top of the stack. */
    stack_client_push(c);
    client_stack();
}

/** Manage a new client.
 * \param w The window.
 * \param wgeom Window geometry.
 * \param screen Virtual screen number where to manage client.
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, int screen)
{
    xcb_get_property_cookie_t ewmh_icon_cookie;
    client_t *c, *t = NULL;
    xcb_window_t trans;
    bool rettrans, retloadprops;
    xcb_size_hints_t *u_size_hints;
    const uint32_t select_input_val[] =
    {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE
            | XCB_EVENT_MASK_ENTER_WINDOW
    };

    /* Send request to get NET_WM_ICON property as soon as possible... */
    ewmh_icon_cookie = ewmh_window_icon_get_unchecked(w);
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);

    if(systray_iskdedockapp(w))
    {
        systray_request_handle(w, screen_virttophys(screen), NULL);
        return;
    }

    c = p_new(client_t, 1);

    c->screen = screen_get_bycoord(globalconf.screens_info, screen, wgeom->x, wgeom->y);

    if(globalconf.screens_info->xinerama_is_active)
        c->phys_screen = globalconf.default_screen;
    else
        c->phys_screen = c->screen;

    /* Initial values */
    c->win = w;
    c->geometry.x = c->f_geometry.x = c->m_geometry.x = wgeom->x;
    c->geometry.y = c->f_geometry.y = c->m_geometry.y = wgeom->y;
    c->geometry.width = c->f_geometry.width = c->m_geometry.width = wgeom->width;
    c->geometry.height = c->f_geometry.height = c->m_geometry.height = wgeom->height;
    c->layer = c->oldlayer = LAYER_TILE;
    client_setborder(c, wgeom->border_width);
    c->icon = ewmh_window_icon_get_reply(ewmh_icon_cookie);

    /* update hints */
    u_size_hints = client_updatesizehints(c);
    client_updatewmhints(c);

    /* Try to load props if any */
    if(!(retloadprops = client_loadprops(c, &globalconf.screens[screen])))
        screen_client_moveto(c, screen, true);

    /* Then check clients hints */
    ewmh_check_client_hints(c);

    /* check for transient and set tags like its parent */
    if((rettrans = xcb_get_wm_transient_for(globalconf.connection, w, &trans))
       && (t = client_getbywin(trans)))
    {
        tag_array_t *tags = &globalconf.screens[c->screen].tags;
        for(int i = 0; i < tags->len; i++)
            if(is_client_tagged(t, tags->tab[i]))
                tag_client(c, tags->tab[i]);
    }

    /* should be floating if transsient or fixed */
    if(rettrans || c->isfixed)
        client_setfloating(c, true);

    /* Push client in client list */
    client_list_push(&globalconf.clients, c);
    client_ref(&c);
    /* Push client in stack */
    client_raise(c);

    /* update window title */
    client_updatetitle(c);

    ewmh_update_net_client_list(c->phys_screen);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.manage, 1, 0);

    if(c->floating_placement
       && !retloadprops
       && u_size_hints
       && !(xcb_size_hints_get_flags(u_size_hints) & (XCB_SIZE_US_POSITION_HINT
                                                      | XCB_SIZE_P_POSITION_HINT)))
    {
        if(c->isfloating)
            client_resize(c, c->floating_placement(c), false);
        else
            c->f_geometry = c->floating_placement(c);
    }

    if(u_size_hints)
        xcb_free_size_hints(u_size_hints);
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
 * \return True if the client has been resized.
 */
bool
client_resize(client_t *c, area_t geometry, bool hints)
{
    int new_screen;
    area_t area;
    layout_t *layout = layout_get_current(c->screen);
    bool resized = false;
    /* Values to configure a window is an array where values are
     * stored according to 'value_mask' */
    uint32_t values[5];

    if(c->titlebar && !c->ismoving && !c->isfloating && layout != layout_floating)
        geometry = titlebar_geometry_remove(c->titlebar, c->border, geometry);

    if(hints)
        geometry = client_geometry_hints(c, geometry);

    if(geometry.width <= 0 || geometry.height <= 0)
        return false;

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

    if(c->geometry.x != geometry.x || c->geometry.y != geometry.y
       || c->geometry.width != geometry.width || c->geometry.height != geometry.height)
    {
        new_screen =
            screen_get_bycoord(globalconf.screens_info, c->screen, geometry.x, geometry.y);

        c->geometry.x = values[0] = geometry.x;
        c->geometry.width = values[2] = geometry.width;
        c->geometry.y = values[1] = geometry.y;
        c->geometry.height = values[3] = geometry.height;
        values[4] = c->border;

        /* save the floating geometry if the window is floating but not
         * maximized */
        if(c->ismoving || c->isfloating
           || layout_get_current(new_screen) == layout_floating)
        {
            titlebar_update_geometry_floating(c);
            if(!c->ismax)
                c->f_geometry = geometry;
        }

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             values);
        window_configure(c->win, geometry, c->border);

        if(c->screen != new_screen)
            screen_client_moveto(c, new_screen, false);

        resized = true;
    }

    /* call it again like it was floating,
     * we want it to be sticked to the window */
    if(!c->ismoving && !c->isfloating && layout != layout_floating)
        titlebar_update_geometry_floating(c);

    return resized;
}

/* Set the client layer.
 * \param c The client.
 * \param layer The layer.
 */
void
client_setlayer(client_t *c, layer_t layer)
{
    c->layer = layer;
    client_raise(c);
    client_saveprops(c);
}

/** Set a clinet floating.
 * \param c The client.
 * \param floating Set floating, or not.
 * \param layer Layer to put the floating window onto.
 */
void
client_setfloating(client_t *c, bool floating)
{
    if(c->isfloating != floating)
    {
        if((c->isfloating = floating))
        {
            client_setlayer(c, MAX(c->layer, LAYER_FLOAT));
            client_resize(c, c->f_geometry, false);
            titlebar_update_geometry_floating(c);
        }
        else if(c->ismax)
        {
            c->ismax = false;
            client_setlayer(c, c->oldlayer);
            client_resize(c, c->m_geometry, false);
        }
        else
            client_setlayer(c, c->oldlayer);
        if(client_isvisible(c, c->screen))
            globalconf.screens[c->screen].need_arrange = true;
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_saveprops(c);
    }
}

/** Save client properties as an X property.
 * \param c The client.
 */
void
client_saveprops(client_t *c)
{
    tag_array_t *tags = &globalconf.screens[c->screen].tags;
    unsigned char *prop = p_alloca(unsigned char, tags->len + 3);
    int i;

    for(i = 0; i < tags->len; i++)
        prop[i] = is_client_tagged(c, tags->tab[i]) ? '1' : '0';

    prop[i++] = c->isfloating ? '1' : '0';
    prop[i++] = '0' + c->layer;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, c->win, _AWESOME_PROPERTIES, STRING, 8, i, prop);
}

/** Unban a client.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    xcb_map_window(globalconf.connection, c->win);
    window_state_set(c->win, XCB_WM_NORMAL_STATE);
    if(c->titlebar && c->titlebar->sw && c->titlebar->position)
        xcb_map_window(globalconf.connection, c->titlebar->sw->window);
}

/** Unmanage a client.
 * \param c The client.
 */
void
client_unmanage(client_t *c)
{
    tag_array_t *tags = &globalconf.screens[c->screen].tags;

    if(globalconf.screens[c->screen].client_focus == c)
        client_unfocus(c);

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.unmanage, 1, 0);

    /* The server grab construct avoids race conditions. */
    xcb_grab_server(globalconf.connection);

    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         (uint32_t *) &c->oldborder);

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, c->win,
                      XCB_BUTTON_MASK_ANY);
    window_state_set(c->win, XCB_WM_WITHDRAWN_STATE);

    xcb_aux_sync(globalconf.connection);
    xcb_ungrab_server(globalconf.connection);

    /* remove client everywhere */
    client_list_detach(&globalconf.clients, c);
    stack_client_delete(c);
    for(int i = 0; i < tags->len; i++)
        untag_client(c, tags->tab[i]);

    if(c->titlebar)
    {
        simplewindow_delete(&c->titlebar->sw);
        titlebar_unref(&c->titlebar);
    }

    ewmh_update_net_client_list(c->phys_screen);

    /* delete properties */
    xcb_delete_property(globalconf.connection, c->win, _AWESOME_PROPERTIES);

    /* set client as invalid */
    c->invalid = true;

    client_unref(&c);
}

/** Update the WM hints of a client.
 * \param c The client.
 */
void
client_updatewmhints(client_t *c)
{
    xcb_wm_hints_t *wmh;
    uint32_t wm_hints_flags;

    if((wmh = xcb_get_wm_hints(globalconf.connection, c->win)))
    {
        wm_hints_flags = xcb_wm_hints_get_flags(wmh);
        if((c->isurgent = xcb_wm_hints_get_urgency(wmh)))
        {
            /* execute hook */
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.urgent, 1, 0);

            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        }
        if((wm_hints_flags & XCB_WM_STATE_HINT) &&
           (xcb_wm_hints_get_initial_state(wmh) == XCB_WM_WITHDRAWN_STATE))
        {
            client_setborder(c, 0);
            c->skip = true;
        }
        xcb_free_wm_hints(wmh);
    }
}

/** Update the size hints of a client.
 * \param c The client.
 * \return A pointer to a xcb_size_hints_t.
 */
xcb_size_hints_t *
client_updatesizehints(client_t *c)
{
    long msize;
    xcb_size_hints_t *size;
    uint32_t size_flags;

    if(!(size = xcb_get_wm_normal_hints(globalconf.connection, c->win, &msize)))
        return NULL;

    size_flags = xcb_size_hints_get_flags(size);

    if((size_flags & XCB_SIZE_P_SIZE_HINT))
        xcb_size_hints_get_base_size(size, &c->basew, &c->baseh);
    else if((size_flags & XCB_SIZE_P_MIN_SIZE_HINT))
        xcb_size_hints_get_min_size(size, &c->basew, &c->baseh);
    else
        c->basew = c->baseh = 0;
    if((size_flags & XCB_SIZE_P_RESIZE_INC_HINT))
        xcb_size_hints_get_increase(size, &c->incw, &c->inch);
    else
        c->incw = c->inch = 0;

    if((size_flags & XCB_SIZE_P_MAX_SIZE_HINT))
        xcb_size_hints_get_max_size(size, &c->maxw, &c->maxh);
    else
        c->maxw = c->maxh = 0;

    if((size_flags & XCB_SIZE_P_MIN_SIZE_HINT))
        xcb_size_hints_get_min_size(size, &c->minw, &c->minh);
    else if((size_flags & XCB_SIZE_BASE_SIZE_HINT))
        xcb_size_hints_get_base_size(size, &c->minw, &c->minh);
    else
        c->minw = c->minh = 0;

    if((size_flags & XCB_SIZE_P_ASPECT_HINT))
    {
        xcb_size_hints_get_min_aspect(size, &c->minax, &c->minay);
        xcb_size_hints_get_max_aspect(size, &c->maxax, &c->maxay);
    }
    else
        c->minax = c->maxax = c->minay = c->maxay = 0;

    if(c->maxw && c->minw && c->maxh && c->minh
       && c->maxw == c->minw && c->maxh == c->minh)
        c->isfixed = true;

    c->hassizehints = !(!c->basew && !c->baseh && !c->incw && !c->inch
                        && !c->maxw && !c->maxh && !c->minw && !c->minh
                        && !c->minax && !c->maxax && !c->minax && !c->minay);
    return size;
}

/** Kill a client via a WM_DELETE_WINDOW request or XKillClient if not
 * supported.
 * \param c The client to kill.
 */
void
client_kill(client_t *c)
{
    xcb_client_message_event_t ev;

    if(window_isprotodel(c->win))
    {
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
 * \luastack
 * \lreturn A table with all clients.
 */
static int
luaA_client_get(lua_State *L)
{
    int i = 1;
    client_t *c;

    lua_newtable(L);

    for(c = globalconf.clients; c; c = c->next)
    {
        luaA_client_userdata_new(globalconf.L, c);
        lua_rawseti(L, -2, i++);
    }

    return 1;
}

/** Add mouse bindings over clients's window.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A button binding.
 */
static int
luaA_client_mouse_add(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    button_t **b = luaA_checkudata(L, 2, "mouse");
    button_list_push(&(*c)->buttons, *b);
    button_ref(b);
    return 0;
}

/** Remove mouse bindings over clients's window.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A button binding.
 */
static int
luaA_client_mouse_remove(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    button_t **b = luaA_checkudata(L, 1, "mouse");
    button_list_detach(&(*c)->buttons, *b);
    button_unref(b);
    return 0;
}

/** Get only visible clients for a screen.
 * \param L The Lua VM state.
 * \luastack
 * \lparam A screen number.
 * \lreturn A table with all visible clients for this screen.
 */
static int
luaA_client_visible_get(lua_State *L)
{
    int i = 1;
    client_t *c;
    int screen = luaL_checknumber(L, 1) - 1;

    luaA_checkscreen(screen);

    lua_newtable(L);

    for(c = globalconf.clients; c; c = c->next)
        if(!c->skip && !c->ishidden && client_isvisible(c, screen))
        {
            luaA_client_userdata_new(globalconf.L, c);
            lua_rawseti(L, -2, i++);
        }

    return 1;
}

/** Get the currently focused client (DEPRECATED).
 * \param L The Lua VM state.
 * \luastack
 * \lreturn The currently focused client.
 */
static int
luaA_client_focus_get(lua_State *L)
{
    if(globalconf.screen_focus->client_focus)
        return luaA_client_userdata_new(L, globalconf.screen_focus->client_focus);
    deprecate();
    return 0;
}

/** Set client border width.
 * \param c The client.
 * \param width The border width.
 */
void
client_setborder(client_t *c, int width)
{
    uint32_t w = width;

    if((c->noborder && width > 0) || width == c->border || width < 0)
        return;

    c->border = width;
    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH, &w);

    if(c->isfloating || layout_get_current(c->screen) == layout_floating)
        titlebar_update_geometry_floating(c);
    else
        globalconf.screens[c->screen].need_arrange = true;
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
    client_list_swap(&globalconf.clients, *swap, *c);
    globalconf.screens[(*c)->screen].need_arrange = true;
    globalconf.screens[(*swap)->screen].need_arrange = true;
    widget_invalidate_cache((*c)->screen, WIDGET_CACHE_CLIENTS);
    widget_invalidate_cache((*swap)->screen, WIDGET_CACHE_CLIENTS);
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

/** Focus a client (DEPRECATED).
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_focus_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_focus(*c);
    deprecate();
    return 0;
}

/** Raise a client on top of others which are on the same layer.
 * \param L The Lua VM state.
 *
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
    return 0;
}

/** Return a formated string for a client.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue  A client.
 * \lreturn A string.
 */
static int
luaA_client_tostring(lua_State *L)
{
    client_t **p = luaA_checkudata(L, 1, "client");
    lua_pushfstring(L, "[client udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Stop managing a client.
 * \param L The Lua VM state.
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
    titlebar_t **t = NULL;

    if((*c)->invalid)
        luaL_error(L, "client is invalid\n");

    switch(a_tokenize(buf, len))
    {
      case A_TK_NAME:
        buf = luaL_checklstring(L, 3, &len);
        p_delete(&(*c)->name);
        a_iso2utf8(&(*c)->name, buf, len);
        widget_invalidate_cache((*c)->screen, WIDGET_CACHE_CLIENTS);
        break;
      case A_TK_FLOATING_PLACEMENT:
        (*c)->floating_placement = name_func_lookup(luaL_checkstring(L, 3),
                                                    FloatingPlacementList);
        break;
      case A_TK_SCREEN:
        if(globalconf.screens_info->xinerama_is_active)
        {
            i = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(i);
            if(i != (*c)->screen)
                screen_client_moveto(*c, i, true);
        }
        break;
      case A_TK_HIDE:
        b = luaA_checkboolean(L, 3);
        if(b != (*c)->ishidden)
        {
            (*c)->ishidden = b;
            if(client_isvisible(*c, (*c)->screen))
                globalconf.screens[(*c)->screen].need_arrange = true;
        }
        break;
      case A_TK_ICON_PATH:
        buf = luaL_checkstring(L, 3);
        p_delete(&(*c)->icon_path);
        (*c)->icon_path = a_strdup(buf);
        widget_invalidate_cache((*c)->screen, WIDGET_CACHE_CLIENTS);
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
      case A_TK_FLOATING:
        client_setfloating(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_HONORSIZEHINTS:
        (*c)->honorsizehints = luaA_checkboolean(L, 3);
        if(client_isvisible(*c, (*c)->screen))
            globalconf.screens[(*c)->screen].need_arrange = true;
        break;
      case A_TK_BORDER_WIDTH:
        client_setborder(*c, luaL_checknumber(L, 3));
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len))
           && xcolor_init_reply(globalconf.connection,
                                xcolor_init_unchecked(globalconf.connection,
                                                      &(*c)->border_color,
                                                      (*c)->phys_screen, buf,
                                                      len)))
            xcb_change_window_attributes(globalconf.connection, (*c)->win,
                                         XCB_CW_BORDER_PIXEL, &(*c)->border_color.pixel);
        break;
      case A_TK_COORDS:
        if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
        {
            area_t geometry;
            luaA_checktable(L, 3);
            geometry.x = luaA_getopt_number(L, 3, "x", (*c)->geometry.x);
            geometry.y = luaA_getopt_number(L, 3, "y", (*c)->geometry.y);
            geometry.width = luaA_getopt_number(L, 3, "width", (*c)->geometry.width);
            geometry.height = luaA_getopt_number(L, 3, "height", (*c)->geometry.height);
            client_resize(*c, geometry, false);
        }
        break;
      case A_TK_TITLEBAR:
        if(!lua_isnil(L, 3))
        {
            t = luaA_checkudata(L, 3, "titlebar");
            if(client_getbytitlebar(*t))
                luaL_error(L, "titlebar is already used by another client");
        }

        /* If client had a titlebar, unref it */
        if((*c)->titlebar)
        {
            simplewindow_delete(&(*c)->titlebar->sw);
            titlebar_unref(&(*c)->titlebar);
            globalconf.screens[(*c)->screen].need_arrange = true;
        }

        if(t)
        {
            /* Attach titlebar to client */
            (*c)->titlebar = *t;
            titlebar_ref(t);
            titlebar_init(*c);
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
 * \lfield class The client class.
 * \lfield instance The client instance.
 * \lfield pid The client PID, if available.
 * \lfield machine The machine client is running on.
 * \lfield icon_name The client name when iconified.
 * \lfield floating_placement The floating placement used for this client.
 * \lfield screen Client screen number.
 * \lfield hide Define if the client must be hidden, i.e. never mapped.
 * \lfield icon_path Path to the icon used to identify.
 * \lfield floating True always floating.
 * \lfield honorsizehints Honor size hints, i.e. respect size ratio.
 * \lfield border_width The client border width.
 * \lfield border_color The client border color.
 * \lfield coords The client coordinates.
 * \lfield titlebar The client titlebar.
 * \lfield urgent The client urgent state.
 * \lfield focus The focused client.
 * \lfield opacity The client opacity between 0 and 1.
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
    xutil_class_hint_t hint;
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
      case A_TK_CLASS:
        if(!xutil_class_hint_get(globalconf.connection, (*c)->win, &hint))
             return 0;
        lua_pushstring(L, hint.res_class);
        break;
      case A_TK_INSTANCE:
        if(!xutil_class_hint_get(globalconf.connection, (*c)->win, &hint))
            return 0;
        lua_pushstring(L, hint.res_name);
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
      case A_TK_MACHINE:
        if(!xutil_text_prop_get(globalconf.connection, (*c)->win,
                                WM_CLIENT_MACHINE, &value, &slen))
            return 0;
        lua_pushlstring(L, value, slen);
        p_delete(&value);
        break;
      case A_TK_ICON_NAME:
        if(!xutil_text_prop_get(globalconf.connection, (*c)->win,
                                _NET_WM_ICON_NAME, &value, &slen))
            if(!xutil_text_prop_get(globalconf.connection, (*c)->win,
                                    WM_ICON_NAME, &value, &slen))
                return 0;
        lua_pushlstring(L, value, slen);
        p_delete(&value);
        break;
      case A_TK_FLOATING_PLACEMENT:
        lua_pushstring(L, name_func_rlookup((*c)->floating_placement,
                                            FloatingPlacementList));
        break;
      case A_TK_SCREEN:
        lua_pushnumber(L, 1 + (*c)->screen);
        break;
      case A_TK_HIDE:
        lua_pushboolean(L, (*c)->ishidden);
        break;
      case A_TK_ICON_PATH:
        lua_pushstring(L, (*c)->icon_path);
        break;
      case A_TK_OPACITY:
        if((d = window_opacity_get((*c)->win)) >= 0)
            lua_pushnumber(L, d);
        else
            return 0;
        break;
      case A_TK_FLOATING:
        lua_pushboolean(L, (*c)->isfloating);
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
      case A_TK_COORDS:
        lua_newtable(L);
        lua_pushnumber(L, (*c)->geometry.width);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, (*c)->geometry.height);
        lua_setfield(L, -2, "height");
        lua_pushnumber(L, (*c)->geometry.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, (*c)->geometry.y);
        lua_setfield(L, -2, "y");
        break;
      case A_TK_TITLEBAR:
        if((*c)->titlebar)
            return luaA_titlebar_userdata_new(globalconf.L, (*c)->titlebar);
        return 0;
      case A_TK_URGENT:
        lua_pushboolean(L, (*c)->isurgent);
        break;
      default:
        return 0;
    }

    return 1;
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
    { "focus_get", luaA_client_focus_get },
    { "visible_get", luaA_client_visible_get },
    { "__index", luaA_client_module_index },
    { "__newindex", luaA_client_module_newindex },
    { NULL, NULL }
};
const struct luaL_reg awesome_client_meta[] =
{
    { "tags", luaA_client_tags },
    { "kill", luaA_client_kill },
    { "swap", luaA_client_swap },
    { "focus_set", luaA_client_focus_set },
    { "raise", luaA_client_raise },
    { "redraw", luaA_client_redraw },
    { "mouse_resize", luaA_client_mouse_resize },
    { "mouse_move", luaA_client_mouse_move },
    { "unmanage", luaA_client_unmanage },
    { "mouse_add", luaA_client_mouse_add },
    { "mouse_remove", luaA_client_mouse_remove },
    { "__index", luaA_client_index },
    { "__newindex", luaA_client_newindex },
    { "__eq", luaA_client_eq },
    { "__gc", luaA_client_gc },
    { "__tostring", luaA_client_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
