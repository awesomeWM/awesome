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

#include "image.h"
#include "client.h"
#include "tag.h"
#include "window.h"
#include "ewmh.h"
#include "widget.h"
#include "screen.h"
#include "titlebar.h"
#include "lua.h"
#include "mouse.h"
#include "systray.h"
#include "statusbar.h"
#include "property.h"
#include "layouts/floating.h"
#include "common/markup.h"
#include "common/atoms.h"

extern awesome_t globalconf;

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
    xcb_get_property_cookie_t floating_q, fullscreen_q;
    xcb_get_property_reply_t *reply;
    void *data;

    if(!xutil_text_prop_get(globalconf.connection, c->win, _AWESOME_TAGS,
                            &prop, &len))
        return false;

    /* Send the GetProperty requests which will be processed later */
    floating_q = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                            _AWESOME_FLOATING, CARDINAL, 0, 1);

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

    /* check for floating */
    reply = xcb_get_property_reply(globalconf.connection, floating_q, NULL);

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        client_setfloating(c, *(bool *) data);
    p_delete(&reply);

    /* check for fullscreen */
    reply = xcb_get_property_reply(globalconf.connection, fullscreen_q, NULL);

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        client_setfullscreen(c, *(bool *) data);
    p_delete(&reply);

    return true;
}

/** Check if client supports protocol WM_DELETE_WINDOW.
 * \param win The window.
 * \return True if client has WM_DELETE_WINDOW, false otherwise.
 */
static bool
window_isprotodel(xcb_window_t win)
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
            if(protocols.atoms[i] == WM_DELETE_WINDOW)
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
    globalconf.screens[c->phys_screen].client_focus = NULL;

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
        window_state_set(c->win, XCB_WM_STATE_ICONIC);
    else
        window_state_set(c->win, XCB_WM_STATE_WITHDRAWN);
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
    if(!client_maybevisible(c, c->screen) || c->nofocus)
        return;

    /* unfocus current selected client */
    if(globalconf.screen_focus->client_focus
       && c != globalconf.screen_focus->client_focus)
        client_unfocus(globalconf.screen_focus->client_focus);

    /* stop hiding c */
    c->ishidden = false;
    c->isminimized = false;

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
    luaA_client_userdata_new(globalconf.L, globalconf.screen_focus->client_focus);
    luaA_dofunction(globalconf.L, globalconf.hooks.focus, 1, 0);

    ewmh_update_net_active_window(c->phys_screen);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

/** Stack a window below.
 * \param c The client.
 * \param previous The previous window on the stack.
 * \param return The next-previous!
 */
static xcb_window_t
client_stack_below(client_t *c, xcb_window_t previous)
{
    uint32_t config_win_vals[2];

    config_win_vals[0] = previous;
    config_win_vals[1] = XCB_STACK_MODE_BELOW;

    if(c->titlebar
       && c->titlebar->sw
       && c->titlebar->position)
    {
        xcb_configure_window(globalconf.connection,
                             c->titlebar->sw->window,
                             XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                             config_win_vals);
        config_win_vals[0] = c->titlebar->sw->window;
    }
    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                         config_win_vals);
    config_win_vals[0] = c->win;

    return c->win;
}

/** Get the real layer of a client according to its attribute (fullscreen, …)
 * \param c The client.
 * \return The real layer.
 */
static layer_t
client_layer_translator(client_t *c)
{
    if(c->isontop)
        return LAYER_ONTOP;
    else if(c->ismodal)
        return LAYER_MODAL;
    else if(c->isfullscreen)
        return LAYER_FULLSCREEN;
    else if(c->isabove)
        return LAYER_ABOVE;
    else if(c->isfloating)
        return LAYER_FLOAT;

    switch(c->type)
    {
      case WINDOW_TYPE_DOCK:
        return LAYER_ABOVE;
      case WINDOW_TYPE_SPLASH:
      case WINDOW_TYPE_DIALOG:
        return LAYER_ABOVE;
      case WINDOW_TYPE_DESKTOP:
        return LAYER_DESKTOP;
      default:
        return LAYER_TILE;
    }
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

    config_win_vals[0] = XCB_NONE;
    config_win_vals[1] = XCB_STACK_MODE_BELOW;

    /* first stack modal and fullscreen windows */
    for(layer = LAYER_OUTOFSPACE - 1; layer >= LAYER_FULLSCREEN; layer--)
        for(node = globalconf.stack; node; node = node->next)
            if(client_layer_translator(node->client) == layer)
                config_win_vals[0] = client_stack_below(node->client, config_win_vals[0]);

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
            if(client_layer_translator(node->client) == layer)
                config_win_vals[0] = client_stack_below(node->client, config_win_vals[0]);
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
    client_t *c;
    draw_image_t *icon;
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
        systray_request_handle(w, phys_screen, NULL);
        return;
    }

    c = p_new(client_t, 1);

    c->screen = screen_getbycoord(screen, wgeom->x, wgeom->y);

    c->phys_screen = phys_screen;

    /* Initial values */
    c->win = w;
    c->geometry.x = c->f_geometry.x = c->m_geometry.x = wgeom->x;
    c->geometry.y = c->f_geometry.y = c->m_geometry.y = wgeom->y;
    c->geometry.width = c->f_geometry.width = c->m_geometry.width = wgeom->width;
    c->geometry.height = c->f_geometry.height = c->m_geometry.height = wgeom->height;
    client_setborder(c, wgeom->border_width);
    if((icon = ewmh_window_icon_get_reply(ewmh_icon_cookie)))
    {
        c->icon = image_new(icon);
        image_ref(&c->icon);
    }

    /* we honor size hints by default */
    c->honorsizehints = true;

    /* update hints */
    property_update_wm_normal_hints(c, NULL);
    property_update_wm_hints(c, NULL);

    /* Try to load props if any */
    client_loadprops(c, &globalconf.screens[screen]);

    /* move client to screen, but do not tag it for now */
    screen_client_moveto(c, screen, false, true);

    /* Then check clients hints */
    ewmh_check_client_hints(c);

    /* Check if client has been tagged by loading props, or maybe with its
     * hints.
     * If not, we tag it with current selected ones.
     * This could be done on Lua side, but it's a sane behaviour. */
    if(!c->issticky)
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

    /* Check if it's a transient window, and manually set it floating. */
    if(!client_isfloating(c))
        property_update_wm_transient_for(c, NULL);

    /* Push client in client list */
    client_list_push(&globalconf.clients, c);
    client_ref(&c);
    /* Push client in stack */
    client_raise(c);

    /* update window title */
    property_update_wm_name(c);

    /* update strut */
    ewmh_client_strut_update(c, NULL);

    ewmh_update_net_client_list(c->phys_screen);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);

    /* call hook */
    luaA_client_userdata_new(globalconf.L, c);
    luaA_dofunction(globalconf.L, globalconf.hooks.manage, 1, 0);
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
    bool resized = false, fixed;
    /* Values to configure a window is an array where values are
     * stored according to 'value_mask' */
    uint32_t values[5];

    if(c->titlebar && !c->ismoving && !client_isfloating(c) && layout != layout_floating)
        geometry = titlebar_geometry_remove(c->titlebar, c->border, geometry);

    if(hints)
        geometry = client_geometry_hints(c, geometry);

    if(geometry.width <= 0 || geometry.height <= 0)
        return false;

    /* offscreen appearance fixes */
    area = display_area_get(c->phys_screen, NULL,
                            &globalconf.screens[c->screen].padding);

    fixed = client_isfixed(c);

    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width - 2 * c->border;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height - 2 * c->border;
    if(geometry.x + geometry.width + 2 * c->border < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height + 2 * c->border < 0)
        geometry.y = 0;

    /* fixed windows can only change their x,y */
    if((fixed && (c->geometry.x != geometry.x || c->geometry.y != geometry.y))
       || (!fixed && (c->geometry.x != geometry.x
                      || c->geometry.y != geometry.y
                      || c->geometry.width != geometry.width
                      || c->geometry.height != geometry.height)))
    {
        new_screen = screen_getbycoord(c->screen, geometry.x, geometry.y);

        c->geometry.x = values[0] = geometry.x;
        c->geometry.width = values[2] = geometry.width;
        c->geometry.y = values[1] = geometry.y;
        c->geometry.height = values[3] = geometry.height;
        values[4] = c->border;

        /* save the floating geometry if the window is floating but not
         * maximized */
        if(c->ismoving || client_isfloating(c)
           || layout_get_current(new_screen) == layout_floating)
        {
            titlebar_update_geometry_floating(c);
            if(!c->isfullscreen)
                c->f_geometry = geometry;
        }

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             values);
        window_configure(c->win, geometry, c->border);

        if(c->screen != new_screen)
            screen_client_moveto(c, new_screen, true, false);

        resized = true;
    }

    /* call it again like it was floating,
     * we want it to be sticked to the window */
    if(!c->ismoving && !client_isfloating(c) && layout != layout_floating)
        titlebar_update_geometry_floating(c);

    return resized;
}

/** Set a clinet floating.
 * \param c The client.
 * \param floating Set floating, or not.
 * \param layer Layer to put the floating window onto.
 */
void
client_setfloating(client_t *c, bool floating)
{
    if(c->isfloating != floating
       && (c->type == WINDOW_TYPE_NORMAL))
    {
        if((c->isfloating = floating))
        {
            if(!c->isfullscreen)
            {
                client_resize(c, c->f_geometry, false);
                titlebar_update_geometry_floating(c);
            }
        }
        client_need_arrange(c);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_stack();
        xcb_change_property(globalconf.connection,
                            XCB_PROP_MODE_REPLACE,
                            c->win, _AWESOME_FLOATING, CARDINAL, 8, 1,
                            &c->isfloating);
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
            /* disable titlebar and borders */
            if(c->titlebar && c->titlebar->sw && (c->titlebar->oldposition = c->titlebar->position))
            {
                xcb_unmap_window(globalconf.connection, c->titlebar->sw->window);
                c->titlebar->position = Off;
            }
            geometry = screen_area_get(c->screen, NULL, &globalconf.screens[c->screen].padding, false);
            c->m_geometry = c->geometry;
            c->oldborder = c->border;
            client_setborder(c, 0);
        }
        else
        {
            /* restore borders and titlebar */
            if(c->titlebar && c->titlebar->sw && (c->titlebar->position = c->titlebar->oldposition))
                xcb_map_window(globalconf.connection, c->titlebar->sw->window);
            geometry = c->m_geometry;
            client_setborder(c, c->oldborder);
            client_resize(c, c->m_geometry, false);
        }
        client_resize(c, geometry, false);
        client_need_arrange(c);
        client_stack();
        xcb_change_property(globalconf.connection,
                            XCB_PROP_MODE_REPLACE,
                            c->win, _AWESOME_FULLSCREEN, CARDINAL, 8, 1,
                            &c->isfullscreen);
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

/** Unban a client.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    xcb_map_window(globalconf.connection, c->win);
    window_state_set(c->win, XCB_WM_STATE_NORMAL);
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

    if(globalconf.screens[c->phys_screen].client_focus == c)
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
    window_state_set(c->win, XCB_WM_STATE_WITHDRAWN);

    xcb_flush(globalconf.connection);
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
        c->titlebar = NULL;
    }

    ewmh_update_net_client_list(c->phys_screen);

    /* delete properties */
    xcb_delete_property(globalconf.connection, c->win, _AWESOME_TAGS);
    xcb_delete_property(globalconf.connection, c->win, _AWESOME_FLOATING);

    if(client_hasstrut(c))
        /* All the statusbars (may) need to be repositioned */
        for(int screen = 0; screen < globalconf.screens_info->nscreen; screen++)
            for(statusbar_t *s = globalconf.screens[screen].statusbar; s; s = s->next)
                statusbar_position_update(s);

    /* set client as invalid */
    c->invalid = true;

    client_unref(&c);
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
        if(client_isvisible(c, screen))
        {
            luaA_client_userdata_new(L, c);
            lua_rawseti(L, -2, i++);
        }

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

    if(client_isvisible(c, c->screen))
    {
        if(client_isfloating(c) || layout_get_current(c->screen) == layout_floating)
            titlebar_update_geometry_floating(c);
        else
            globalconf.screens[c->screen].need_arrange = true;
    }
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
    client_need_arrange(*c);
    client_need_arrange(*swap);
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

    /* Set the focus on the current window if the redraw has been
       performed on the window where the pointer is currently on
       because after the unmapping/mapping, the focus is lost */
    if(globalconf.screen_focus->client_focus == *c)
        xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                            (*c)->win, XCB_CURRENT_TIME);

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

/** Return client coordinates.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new coordinates, or none.
 * \lreturn A table with client coordinates.
 */
static int
luaA_client_coords(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");

    if(lua_gettop(L) == 2)
    {
        if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
        {
            area_t geometry;

            luaA_checktable(L, 2);
            geometry.x = luaA_getopt_number(L, 2, "x", (*c)->geometry.x);
            geometry.y = luaA_getopt_number(L, 2, "y", (*c)->geometry.y);
            geometry.width = luaA_getopt_number(L, 2, "width", (*c)->geometry.width);
            geometry.height = luaA_getopt_number(L, 2, "height", (*c)->geometry.height);
            client_resize(*c, geometry, false);
        }
    }

    return luaA_pusharea(L, (*c)->geometry);
}

/** Return client coordinates, using also titlebar and border width.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new coordinates, or none.
 * \lreturn A table with client coordinates.
 */
static int
luaA_client_fullcoords(lua_State *L)
{
    client_t **c = luaA_checkudata(L, 1, "client");
    area_t geometry;

    if(lua_gettop(L) == 2)
    {
        if((*c)->isfloating || layout_get_current((*c)->screen) == layout_floating)
        {
            luaA_checktable(L, 2);
            geometry.x = luaA_getopt_number(L, 2, "x", (*c)->geometry.x);
            geometry.y = luaA_getopt_number(L, 2, "y", (*c)->geometry.y);
            geometry.width = luaA_getopt_number(L, 2, "width", (*c)->geometry.width);
            geometry.height = luaA_getopt_number(L, 2, "height", (*c)->geometry.height);
            geometry = titlebar_geometry_remove((*c)->titlebar,
                                                (*c)->border,
                                                geometry);
            client_resize(*c, geometry, false);
        }
    }

    return luaA_pusharea(L, titlebar_geometry_add((*c)->titlebar,
                                                  (*c)->border,
                                                  (*c)->geometry));
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
    image_t **image;

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
      case A_TK_SCREEN:
        if(globalconf.screens_info->xinerama_is_active)
        {
            i = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(i);
            if(i != (*c)->screen)
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
        b = luaA_checkboolean(L, 3);
        if(b != (*c)->isminimized)
        {
            client_need_arrange(*c);
            (*c)->isminimized = b;
            client_need_arrange(*c);
        }
        break;
      case A_TK_FULLSCREEN:
        client_setfullscreen(*c, luaA_checkboolean(L, 3));
        break;
      case A_TK_ICON:
        image = luaA_checkudata(L, 3, "image");
        image_unref(&(*c)->icon);
        image_ref(image);
        (*c)->icon = *image;
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
            (*c)->titlebar = NULL;
            client_need_arrange(*c);
        }

        if(t)
        {
            /* Attach titlebar to client */
            (*c)->titlebar = *t;
            titlebar_ref(t);
            titlebar_init(*c);
        }
        client_stack();
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
 * \lfield type The window type (desktop, normal, dock, …).
 * \lfield class The client class.
 * \lfield instance The client instance.
 * \lfield pid The client PID, if available.
 * \lfield role The window role, if available.
 * \lfield machine The machine client is running on.
 * \lfield icon_name The client name when iconified.
 * \lfield screen Client screen number.
 * \lfield hide Define if the client must be hidden, i.e. never mapped, not
 * visible in taskbar.
 * invisible in taskbar.
 * \lfield minimize Define it the client must be iconify, i.e. only visible in
 * taskbar.
 * \lfield icon_path Path to the icon used to identify.
 * \lfield floating True always floating.
 * \lfield honorsizehints Honor size hints, i.e. respect size ratio.
 * \lfield border_width The client border width.
 * \lfield border_color The client border color.
 * \lfield titlebar The client titlebar.
 * \lfield urgent The client urgent state.
 * \lfield focus The focused client.
 * \lfield opacity The client opacity between 0 and 1.
 * \lfield ontop The client is on top of every other windows.
 * \lfield fullscreen The client is fullscreen or not.
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
      case A_TK_SCREEN:
        lua_pushnumber(L, 1 + (*c)->screen);
        break;
      case A_TK_HIDE:
        lua_pushboolean(L, (*c)->ishidden);
        break;
      case A_TK_MINIMIZE:
        lua_pushboolean(L, (*c)->isminimized);
        break;
      case A_TK_FULLSCREEN:
        lua_pushboolean(L, (*c)->isfullscreen);
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
      case A_TK_FLOATING:
        lua_pushboolean(L, (*c)->isfloating);
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
            return luaA_titlebar_userdata_new(L, (*c)->titlebar);
        return 0;
      case A_TK_URGENT:
        lua_pushboolean(L, (*c)->isurgent);
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

const struct luaL_reg awesome_client_methods[] =
{
    { "get", luaA_client_get },
    { "visible_get", luaA_client_visible_get },
    { "__index", luaA_client_module_index },
    { "__newindex", luaA_client_module_newindex },
    { NULL, NULL }
};
const struct luaL_reg awesome_client_meta[] =
{
    { "coords", luaA_client_coords },
    { "fullcoords", luaA_client_fullcoords },
    { "buttons", luaA_client_buttons },
    { "tags", luaA_client_tags },
    { "kill", luaA_client_kill },
    { "swap", luaA_client_swap },
    { "raise", luaA_client_raise },
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
