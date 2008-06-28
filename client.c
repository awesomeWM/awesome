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

#include "client.h"
#include "tag.h"
#include "window.h"
#include "focus.h"
#include "ewmh.h"
#include "widget.h"
#include "screen.h"
#include "titlebar.h"
#include "lua.h"
#include "stack.h"
#include "mouse.h"
#include "layouts/floating.h"
#include "common/markup.h"

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
    return luaA_settype(L, "client");
}

DO_LUA_EQ(client_t, client, "client")

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any.
 * \param c A client pointer.
 * \param screen A virtual screen.
 * \return True if client had property, false otherwise.
 */
static bool
client_loadprops(client_t * c, screen_t *screen)
{
    tag_array_t *tags = &screen->tags;
    char *prop = NULL;

    if(!xutil_gettextprop(globalconf.connection, c->win, &globalconf.atoms,
                          xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms,
                                                  xutil_intern_atom(globalconf.connection,
                                                                    &globalconf.atoms,
                                                                    "_AWESOME_PROPERTIES")),
                         &prop))
        return false;

    if(a_strlen(prop) != tags->len + 2)
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
    xcb_atom_t wm_delete_win_atom;
    xcb_atom_t *protocols;
    bool ret = false;

    if(xcb_get_wm_protocols(globalconf.connection, win, &n, &protocols))
    {
        wm_delete_win_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms,
                                                     xutil_intern_atom(globalconf.connection,
                                                                       &globalconf.atoms,
                                                                       "WM_DELETE_WINDOW"));

        for(i = 0; !ret && i < n; i++)
            if(protocols[i] == wm_delete_win_atom)
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
client_isvisible(client_t *c, int screen)
{
    if(c && !c->ishidden && c->screen == screen)
    {
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

/** Update client name attribute with its new title.
 * \param c The client.
 */
void
client_updatetitle(client_t *c)
{
    char *name;
    xutil_intern_atom_request_t net_wm_name_q;
    xcb_atom_t net_wm_name;

    net_wm_name_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "_NET_WM_NAME");
    net_wm_name = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, net_wm_name_q);

    if(!xutil_gettextprop(globalconf.connection, c->win, &globalconf.atoms, net_wm_name, &name))
        if(!xutil_gettextprop(globalconf.connection, c->win, &globalconf.atoms, WM_NAME, &name))
            return;

    p_delete(&c->name);
    a_iso2utf8(name, &c->name);

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.titleupdate, 1);

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

/** Unfocus a client.
 * \param c The client.
 */
static void
client_unfocus(client_t *c)
{
    /* Call hook */
    luaA_client_userdata_new(globalconf.L, globalconf.focus->client);
    luaA_dofunction(globalconf.L, globalconf.hooks.unfocus, 1);

    focus_client_push(NULL);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    ewmh_update_net_active_window(c->phys_screen);
}

/** Ban client and unmap it.
 * \param c The client.
 */
void
client_ban(client_t *c)
{
    if(globalconf.focus->client == c)
        client_unfocus(c);
    xcb_unmap_window(globalconf.connection, c->win);
    if(c->ishidden)
        window_setstate(c->win, XCB_WM_ICONIC_STATE);
    else
        window_setstate(c->win, XCB_WM_WITHDRAWN_STATE);
    if(c->titlebar && c->titlebar->position && c->titlebar->sw)
        xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
}

/** Give focus to client, or to first client if client is NULL.
 * \param c The client or NULL.
 * \param screen Virtual screen number.
 * \return True if a window (even root) has received focus, false otherwise.
 */
void
client_focus(client_t *c, int screen)
{
    int phys_screen;

    /* if c is NULL or invisible, take next client in the focus history */
    if((!c || (c && (!client_isvisible(c, screen))))
       && !(c = focus_get_current_client(screen)))
        /* if c is still NULL take next client in the stack */
        for(c = globalconf.clients; c && (c->skip || !client_isvisible(c, screen)); c = c->next);

    /* unfocus current selected client */
    if(globalconf.focus->client && c != globalconf.focus->client)
        client_unfocus(globalconf.focus->client);

    if(c)
    {
        /* unban the client before focusing or it will fail */
        client_unban(c);
        /* save sel in focus history */
        focus_client_push(c);
        xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                            c->win, XCB_CURRENT_TIME);
        phys_screen = c->phys_screen;

        /* Some layouts use focused client differently, so call them back. */
        globalconf.screens[c->screen].need_arrange = true;

        /* execute hook */
        luaA_client_userdata_new(globalconf.L, globalconf.focus->client);
        luaA_dofunction(globalconf.L, globalconf.hooks.focus, 1);
    }
    else
    {
        phys_screen = screen_virttophys(screen);
        xcb_set_input_focus(globalconf.connection,
                            XCB_INPUT_FOCUS_POINTER_ROOT,
                            xutil_screen_get(globalconf.connection, phys_screen)->root,
                            XCB_CURRENT_TIME);
    }

    ewmh_update_net_active_window(phys_screen);
    widget_invalidate_cache(screen, WIDGET_CACHE_CLIENTS);
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
static void
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
    client_t *c, *t = NULL;
    xcb_window_t trans;
    bool rettrans, retloadprops;
    xcb_size_hints_t *u_size_hints;
    const uint32_t select_input_val[] = {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |
        XCB_EVENT_MASK_ENTER_WINDOW };

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
    c->oldborder = wgeom->border_width;
    c->layer = c->oldlayer = LAYER_TILE;

    /* update hints */
    u_size_hints = client_updatesizehints(c);
    client_updatewmhints(c);

    /* Try to load props if any */
    if(!(retloadprops = client_loadprops(c, &globalconf.screens[screen])))
        screen_client_moveto(c, screen, true);

    /* Then check clients hints */
    ewmh_check_client_hints(c);

    /* check for transient and set tags like its parent */
    if((rettrans = xutil_get_transient_for_hint(globalconf.connection, w, &trans))
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

    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);

    /* Push client in client list */
    client_list_push(&globalconf.clients, c);
    /* Append client in history: it'll be last. */
    focus_client_append(c);
    /* Push client in stack */
    client_raise(c);

    /* update window title */
    client_updatetitle(c);

    ewmh_update_net_client_list(c->phys_screen);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.manage, 1);

    if(c->floating_placement
       && !retloadprops
       && u_size_hints
       && !(xcb_size_hints_get_flags(u_size_hints) & (XCB_SIZE_US_POSITION_HINT |
                                                      XCB_SIZE_P_POSITION_HINT)))
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
    xutil_intern_atom_request_t atom_q;
    int i;

    atom_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "_AWESOME_PROPERTIES");

    for(i = 0; i < tags->len; i++)
        prop[i] = is_client_tagged(c, tags->tab[i]) ? '1' : '0';

    prop[i++] = c->isfloating ? '1' : '0';
    prop[i++] = '0' + c->layer;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, c->win,
                        xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, atom_q),
                        STRING, 8, i, prop);
}

/** Unban a client.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    xcb_map_window(globalconf.connection, c->win);
    window_setstate(c->win, XCB_WM_NORMAL_STATE);
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

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.unmanage, 1);

    /* The server grab construct avoids race conditions. */
    xcb_grab_server(globalconf.connection);

    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         (uint32_t *) &c->oldborder);

    /* remove client everywhere */
    client_list_detach(&globalconf.clients, c);
    focus_client_delete(c);
    stack_client_delete(c);
    for(int i = 0; i < tags->len; i++)
        untag_client(c, tags->tab[i]);

    if(globalconf.focus->client == c)
        client_focus(NULL, c->screen);

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, c->win, ANY_MODIFIER);
    window_setstate(c->win, XCB_WM_WITHDRAWN_STATE);

    xcb_aux_sync(globalconf.connection);
    xcb_ungrab_server(globalconf.connection);

    if(c->titlebar)
    {
        simplewindow_delete(&c->titlebar->sw);
        titlebar_unref(&c->titlebar);
    }

    ewmh_update_net_client_list(c->phys_screen);

    p_delete(&c);
}

/** Update the WM hints of a client.
 * \param c The client.
 */
void
client_updatewmhints(client_t *c)
{
    xcb_wm_hints_t *wmh = NULL;
    uint32_t wm_hints_flags;

    if((wmh = xcb_get_wm_hints(globalconf.connection, c->win)))
    {
        wm_hints_flags = xcb_wm_hints_get_flags(wmh);
        if((c->isurgent = (wm_hints_flags & XCB_WM_X_URGENCY_HINT)))
        {
            /* execute hook */
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.urgent, 1);

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

/** Update the size hintz of a client.
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

/** Markup parsing hook.
 * For now only substitute <title />
 */
static void
client_markup_on_elem(markup_parser_data_t *p, const char *elem,
                      const char **names, const char **values)
{
    assert(!a_strcmp(elem, "title"));
    buffer_add_xmlescaped(&p->text, NONULL(p->priv));
}

/** Parse a markup string which contains special markup sequence relative to a
 * client, i.e. its title, etc.
 * \param c The client concerned by the markup string.
 * \param str The markup string.
 * \param len The string length.
 */
char *
client_markup_parse(client_t *c, const char *str, ssize_t len)
{
    static char const * const elements[] = { "title", NULL };
    markup_parser_data_t p = {
        .elements   = elements,
        .priv       = c->name,
        .on_element = client_markup_on_elem,
    };
    char *ret;

    markup_parser_data_init(&p);

    if(markup_parse(&p, str, len))
        ret = buffer_detach(&p.text);
    else
        ret = a_strdup(str);

    markup_parser_data_wipe(&p);

    return ret;
}

/** Kill a client via a WM_DELETE_WINDOW request or XKillClient if not
 * supported.
 * \param c The client to kill.
 */
void
client_kill(client_t *c)
{
    xcb_client_message_event_t ev;
    xutil_intern_atom_request_t wm_protocols_q, wm_delete_window_q;

    if(window_isprotodel(c->win))
    {
        wm_protocols_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "WM_PROTOCOLS");
        wm_delete_window_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "WM_DELETE_WINDOW");

        /* Initialize all of event's fields first */
        p_clear(&ev, 1);

        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = c->win;
        ev.format = 32;
        ev.data.data32[1] = XCB_CURRENT_TIME;
        ev.type = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, wm_protocols_q);
        ev.data.data32[0] = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, wm_delete_window_q);

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

/** Get the currently focused client.
 * \param L The Lua VM state.
 * \luastack
 * \lreturn The currently focused client.
 */
static int
luaA_client_focus_get(lua_State *L __attribute__ ((unused)))
{
    if(globalconf.focus->client)
        return luaA_client_userdata_new(globalconf.L, globalconf.focus->client);
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
    globalconf.screens[c->screen].need_arrange = true;
}

/** Set the client border width and color.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A table with `width' key for the border width in pixel and `color' key
 * for the border color.
 */
static int
luaA_client_border_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    int width = luaA_getopt_number(L, 2, "width", (*c)->border);
    const char *colorstr = luaA_getopt_string(L, 2, "color", NULL);
    xcolor_t color;

    client_setborder(*c, width);

    if(colorstr
        && xcolor_new(globalconf.connection, (*c)->phys_screen, colorstr, &color))
        xcb_change_window_attributes(globalconf.connection, (*c)->win, XCB_CW_BORDER_PIXEL,
                                     &color.pixel);

    return 0;
}

/** Move the client to another screen.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A screen number.
 */
static int
luaA_client_screen_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    int screen = luaL_checknumber(L, 2) - 1;
    luaA_checkscreen(screen);
    screen_client_moveto(*c, screen, true);
    return 0;
}

/** Get the screen number the client is onto.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A screen number.
 */
static int
luaA_client_screen_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_pushnumber(L, 1 + (*c)->screen);
    return 1;
}

/** Tag a client with a specified tag.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A tag object.
 * \lparam A boolean value: true to add this tag to clients, false to remove.
 */
static int
luaA_client_tag(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    tag_t **tag = luaA_checkudata(L, 2, "tag");
    bool tag_the_client = luaA_checkboolean(L, 3);

    if((*tag)->screen != (*c)->screen)
        luaL_error(L, "tag and client are on different screens");

    if(tag_the_client)
        tag_client(*c, *tag);
    else
        untag_client(*c, *tag);

    return 0;
}

/** Check if a client is tagged with the specified tag.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A tag object.
 * \lreturn A boolean value, true if the client is tagged with this tag, false
 * otherwise.
 */
static int
luaA_client_istagged(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    tag_t **tag = luaA_checkudata(L, 2, "tag");
    lua_pushboolean(L, is_client_tagged(*c, *tag));
    return 1;
}

/** Get the client coordinates on the display.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A table with keys `width', `height', `x' and `y'.
 */
static int
luaA_client_coords_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_newtable(L);
    lua_pushnumber(L, (*c)->geometry.width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, (*c)->geometry.height);
    lua_setfield(L, -2, "height");
    lua_pushnumber(L, (*c)->geometry.x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, (*c)->geometry.y);
    lua_setfield(L, -2, "y");
    return 1;
}

/** Set client coordinates. This only operates if the client is floating.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A table with keys: x, y, width, height.
 */
static int
luaA_client_coords_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    area_t geometry;

    if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
    {
        luaA_checktable(L, 2);
        geometry.x = luaA_getopt_number(L, 2, "x", (*c)->geometry.x);
        geometry.y = luaA_getopt_number(L, 2, "y", (*c)->geometry.y);
        geometry.width = luaA_getopt_number(L, 2, "width", (*c)->geometry.width);
        geometry.height = luaA_getopt_number(L, 2, "height", (*c)->geometry.height);
        client_resize(*c, geometry, false);
    }

    return 0;
}

/** Set the client opacity.
 * Note: this requires an external composite manager.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A floating value between 0 and 1.
 */
static int
luaA_client_opacity_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    double opacity = luaL_checknumber(L, 2);

    if(opacity == -1 || (opacity >= 0 && opacity <= 1))
        window_settrans((*c)->win, opacity);
    return 0;
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

/** Focus a client.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_focus_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    client_focus(*c, (*c)->screen);
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

/** Set the client floating attribute.
 * \param L The Lua VM state.
 * \luastack
 * \lparam A client.
 * \lparam A boolean, true to set, false to unset.
 */
static int
luaA_client_floating_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    bool f = luaA_checkboolean(L, 2);
    client_setfloating(*c, f);
    return 0;
}

/** Check if a client has the floating attribute.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A boolean, true if the client has the floating attribute set, false
 * otherwise.
 */
static int
luaA_client_floating_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_pushboolean(L, (*c)->isfloating);
    return 1;
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

/** Get the client name.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A string with the client class.
 */
static int
luaA_client_class_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    class_hint_t *hint=xutil_get_class_hint(globalconf.connection, (*c)->win);
    if (hint)
        lua_pushstring(L, hint->res_class);
    else
        luaL_error(L, "Unable to get the class property for client");
    return 1;
}

/** Set the default icon for this client.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A path to an icon image, or nil to remove.
 */
static int
luaA_client_icon_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    const char *icon = luaL_optstring(L, 2, NULL);

    p_delete(&(*c)->icon_path);
    (*c)->icon_path = a_strdup(icon);

    return 0;
}

/** Get the client name.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A string with the client name.
 */
static int
luaA_client_name_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_pushstring(L, (*c)->name);
    return 1;
}

/** Change the client name. It'll change it only from awesome point of view.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A string with the new client name.
 */
static int
luaA_client_name_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    const char *name = luaL_checkstring(L, 2);
    p_delete(&(*c)->name);
    a_iso2utf8(name, &(*c)->name);
    return 0;
}

/** Set the client's titlebar.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A titlebar.
 */
static int
luaA_client_titlebar_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    titlebar_t **t = NULL;

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        t = luaA_checkudata(L, 2, "titlebar");
        if(client_getbytitlebar(*t))
            luaL_error(L, "titlebar is already used by another client");
    }

    /* If client had a titlebar, unref it */
    if((*c)->titlebar)
    {
        simplewindow_delete(&(*c)->titlebar->sw);
        titlebar_unref(&(*c)->titlebar);
    }

    if(t)
    {
        /* Attach titlebar to client */
        (*c)->titlebar = *t;
        titlebar_ref(t);
        titlebar_init(*c);
    }
    else
        (*c)->titlebar = NULL;

    if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
        titlebar_update_geometry_floating(*c);
    else
        globalconf.screens[(*c)->screen].need_arrange = true;

    return 0;
}

/** Get the titlebar of a client.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lreturn A titlebar or nil if the client has no titlebar.
 */
static int
luaA_client_titlebar_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");

    if((*c)->titlebar)
        return luaA_titlebar_userdata_new(globalconf.L, (*c)->titlebar);

    return 0;
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

/** Hide or unhide a client.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_hide_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    bool v = luaA_checkboolean(L, 2);
    if(v != (*c)->ishidden)
    {
        (*c)->ishidden = v;
        globalconf.screens[(*c)->screen].need_arrange = true;
    }
    return 0;
}

/** Guess if a client has been hidden.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 * \lreturn A boolean, true if the client has been hidden with hide(), false
 * otherwise.
 */
static int
luaA_client_hide_get(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    lua_pushboolean(L, (*c)->ishidden);
    return 1;
}

/** Set the floating placement algorithm. This will be used to compute the
 * initial floating position of the window then floating.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 * \lparam An algorith name, either `none', `smart' or `under_mouse'.
 */
static int
luaA_client_floating_placement_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    const char *pl = luaL_checkstring(L, 2);
    (*c)->floating_placement = name_func_lookup(pl, FloatingPlacementList);
    return 0;
}

/** Define if awesome should respect client size hints when resizing
 * windows in tiled mode. If you set this to true, you will experience gaps
 * between windows, but they will have the best size they can have.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 * \lparam A boolean value, true to enable, false to disable.
 */
static int
luaA_honorsizehints_set(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    (*c)->honorsizehints = luaA_checkboolean(L, 2);
    return 0;
}

const struct luaL_reg awesome_client_methods[] =
{
    { "get", luaA_client_get },
    { "focus_get", luaA_client_focus_get },
    { "visible_get", luaA_client_visible_get },
    { NULL, NULL }
};
const struct luaL_reg awesome_client_meta[] =
{
    { "honorsizehints_set", luaA_honorsizehints_set },
    { "floating_placement_set", luaA_client_floating_placement_set },
    { "titlebar_set", luaA_client_titlebar_set },
    { "titlebar_get", luaA_client_titlebar_get },
    { "name_get", luaA_client_name_get },
    { "name_set", luaA_client_name_set },
    { "screen_set", luaA_client_screen_set },
    { "screen_get", luaA_client_screen_get },
    { "border_set", luaA_client_border_set },
    { "tag", luaA_client_tag },
    { "istagged", luaA_client_istagged },
    { "coords_get", luaA_client_coords_get },
    { "coords_set", luaA_client_coords_set },
    { "opacity_set", luaA_client_opacity_set },
    { "kill", luaA_client_kill },
    { "swap", luaA_client_swap },
    { "focus_set", luaA_client_focus_set  },
    { "raise", luaA_client_raise },
    { "redraw", luaA_client_redraw },
    { "floating_set", luaA_client_floating_set },
    { "floating_get", luaA_client_floating_get },
    { "icon_set", luaA_client_icon_set },
    { "class_get", luaA_client_class_get },
    { "mouse_resize", luaA_client_mouse_resize },
    { "mouse_move", luaA_client_mouse_move },
    { "unmanage", luaA_client_unmanage },
    { "hide_set", luaA_client_hide_set },
    { "hide_get", luaA_client_hide_get },
    { "mouse_add", luaA_client_mouse_add },
    { "mouse_remove", luaA_client_mouse_remove },
    { "__eq", luaA_client_eq },
    { "__tostring", luaA_client_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
