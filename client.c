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
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <xcb/shape.h>

#include "client.h"
#include "tag.h"
#include "rules.h"
#include "window.h"
#include "focus.h"
#include "ewmh.h"
#include "widget.h"
#include "screen.h"
#include "titlebar.h"
#include "layouts/floating.h"
#include "common/xutil.h"
#include "common/xscreen.h"

extern AwesomeConf globalconf;

/** Load windows properties, restoring client's tag
 * and floating state before awesome was restarted if any
 * \todo this may bug if number of tags is != than before
 * \param c Client ref
 * \param screen Screen ID
 * \return true if client had property
 */
static bool
client_loadprops(Client * c, int screen)
{
    int i, ntags = 0;
    Tag *tag;
    char *prop;
    bool result = false;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 3);

    if(xutil_gettextprop(globalconf.connection, c->win,
                         xutil_intern_atom(globalconf.connection, "_AWESOME_PROPERTIES"),
                         prop, ntags + 3))
    {
        for(i = 0, tag = globalconf.screens[screen].tags; tag && i < ntags && prop[i]; i++, tag = tag->next)
            if(prop[i] == '1')
            {
                tag_client(c, tag);
                result = true;
            }
            else
                untag_client(c, tag);

        if(prop[i])
            client_setfloating(c, prop[i] == '1', (prop[i + 1] >= 0 && prop[i + 1] <= LAYER_FULLSCREEN) ? atoi(&prop[i + 1]) : prop[i] == '1' ? LAYER_FLOAT : LAYER_TILE);
    }

    p_delete(&prop);

    return result;
}

/** Check if client supports protocol WM_DELETE_WINDOW
 * \param disp the display
 * \param win the Window
 * \return true if client has WM_DELETE_WINDOW
 */
static bool
client_isprotodel(xcb_connection_t *c, xcb_window_t win)
{
    uint32_t i, n;
    xcb_atom_t *protocols;
    bool ret = false;

    if(xcb_get_wm_protocols(c, win, &n, &protocols))
    {
        for(i = 0; !ret && i < n; i++)
            if(protocols[i] == xutil_intern_atom(c, "WM_DELETE_WINDOW"))
                ret = true;
        p_delete(&protocols);
    }
    return ret;
}

/** Get a Client by its window
 * \param list Client list to look info
 * \param w Client window to find
 * \return client
 */
Client *
client_get_bywin(Client *list, xcb_window_t w)
{
    Client *c;

    for(c = list; c && c->win != w; c = c->next);
    return c;
}

/** Get a client by its name
 * \param list Client list
 * \param name name to search
 * \return first matching client
 */
Client *
client_get_byname(Client *list, char *name)
{
    Client *c;

    for(c = list; c; c = c->next)
        if(strstr(c->name, name))
            return c;

    return NULL;
}

/** Update client name attribute with its title
 * \param c the client
 */
void
client_updatetitle(Client *c)
{
    if(!xutil_gettextprop(globalconf.connection, c->win,
                          xutil_intern_atom(globalconf.connection, "_NET_WM_NAME"),
                          c->name, sizeof(c->name)))
        xutil_gettextprop(globalconf.connection, c->win,
                          xutil_intern_atom(globalconf.connection, "WM_NAME"),
                          c->name, sizeof(c->name));
    titlebar_draw(c);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

static void
client_unfocus(Client *c)
{
    if(globalconf.screens[c->screen].opacity_unfocused != -1)
        window_settrans(c->win, globalconf.screens[c->screen].opacity_unfocused);
    else if(globalconf.screens[c->screen].opacity_focused != -1)
        window_settrans(c->win, -1);
    focus_add_client(NULL);
    xcb_change_window_attributes(globalconf.connection, c->win, XCB_CW_BORDER_PIXEL,
                                 &globalconf.screens[c->screen].styles.normal.border.pixel);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    titlebar_draw(c);
}

/** Ban client and unmap it
 * \param c the client
 */
void
client_ban(Client *c)
{
    if(globalconf.focus->client == c)
        client_unfocus(c);
    xcb_unmap_window(globalconf.connection, c->win);
    window_setstate(c->win, XCB_WM_ICONIC_STATE);
    if(c->titlebar.position && c->titlebar.sw)
        xcb_unmap_window(globalconf.connection, c->titlebar.sw->window);
}

/** Give focus to client, or to first client if c is NULL
 * \param c client
 * \param screen Screen ID
 * \param raise raise window if true
 * \return true if a window (even root) has received focus, false otherwise
 */
bool
client_focus(Client *c, int screen, bool raise)
{
    int phys_screen;

    /* if c is NULL or invisible, take next client in the focus history */
    if((!c || (c && (!client_isvisible(c, screen))))
       && !(c = focus_get_current_client(screen)))
        /* if c is still NULL take next client in the stack */
        for(c = globalconf.clients; c && (c->skip || !client_isvisible(c, screen)); c = c->next);

    /* if c is already the focused window, then stop */
    if(c == globalconf.focus->client)
        return false;

    /* unfocus current selected client */
    if(globalconf.focus->client)
        client_unfocus(globalconf.focus->client);

    if(c)
    {
        /* unban the client before focusing or it will fail */
        client_unban(c);
        /* save sel in focus history */
        focus_add_client(c);
        if(globalconf.screens[c->screen].opacity_focused != -1)
            window_settrans(c->win, globalconf.screens[c->screen].opacity_focused);
        else if(globalconf.screens[c->screen].opacity_unfocused != -1)
            window_settrans(c->win, -1);
        xcb_change_window_attributes(globalconf.connection, c->win, XCB_CW_BORDER_PIXEL,
                                     &globalconf.screens[screen].styles.focus.border.pixel);
        titlebar_draw(c);
        xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
                            c->win, XCB_CURRENT_TIME);
        if(raise)
            client_stack(c);
        /* since we're dropping EnterWindow events and sometimes the window
         * will appear under the mouse, grabbuttons */
        window_grabbuttons(c->win, c->phys_screen);
        phys_screen = c->phys_screen;
    }
    else
    {
        phys_screen = screen_virttophys(screen);
        xcb_set_input_focus(globalconf.connection,
                            XCB_INPUT_FOCUS_POINTER_ROOT,
                            xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
                            XCB_CURRENT_TIME);
    }

    ewmh_update_net_active_window(phys_screen);
    widget_invalidate_cache(screen, WIDGET_CACHE_CLIENTS);

    return true;
}

void
client_stack(Client *c)
{
    uint32_t config_win_vals[2];
    Client *client;
    unsigned int layer;
    Layer maxlayer = LAYER_FULLSCREEN;

    config_win_vals[0] = XCB_NONE;
    config_win_vals[1] = XCB_STACK_MODE_ABOVE;

    for(layer = 0; layer < maxlayer; layer++)
    {
        for(client = globalconf.clients; client; client = client->next)
        {
            if(client->layer == layer && client != c && client_isvisible(client, c->screen))
            {
                if(client->titlebar.position && client->titlebar.sw)
                {
                    xcb_configure_window(globalconf.connection,
                                         client->titlebar.sw->window,
                                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                         config_win_vals);
                    config_win_vals[0] = client->titlebar.sw->window;
                }
                xcb_configure_window(globalconf.connection, client->win,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = client->win;
            }
        }
        if(c->layer == layer)
        {
            if(c->titlebar.position && c->titlebar.sw)
            {
                xcb_configure_window(globalconf.connection, c->titlebar.sw->window,
                                     XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                     config_win_vals);
                config_win_vals[0] = c->titlebar.sw->window;
            }
            xcb_configure_window(globalconf.connection, c->win,
                                 XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                 config_win_vals);
            config_win_vals[0] = c->win;
        }
    }
}

/** Manage a new client
 * \param w The window
 * \param wgeom Window geometry
 * \param screen Screen ID
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, int screen)
{
    Client *c, *t = NULL;
    xcb_window_t trans;
    bool rettrans, retloadprops;
    uint32_t config_win_val;
    Tag *tag;
    Rule *rule;
    xcb_size_hints_t *u_size_hints;

    c = p_new(Client, 1);

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
    c->newcomer = true;
    c->layer = c->oldlayer = LAYER_TILE;

    /* Set windows borders */
    config_win_val = c->border = globalconf.screens[screen].borderpx;
    xcb_configure_window(globalconf.connection, w, XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         &config_win_val);
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_BORDER_PIXEL,
                                 &config_win_val);
    /* propagates border_width, if size doesn't change */
    window_configure(c->win, c->geometry, c->border);

    /* update window title */
    client_updatetitle(c);

    /* update hints */
    u_size_hints = client_updatesizehints(c);
    client_updatewmhints(c);

    /* Try to load props if any */
    if(!(retloadprops = client_loadprops(c, screen)))
        move_client_to_screen(c, screen, true);

    /* Then check clients hints */
    ewmh_check_client_hints(c);

    /* default titlebar position */
    c->titlebar = globalconf.screens[screen].titlebar_default;

    /* get the matching rule if any */
    rule = rule_matching_client(c);

    /* Then apply rules if no props */
    if(!retloadprops && rule)
    {
        if(rule->screen != RULE_NOSCREEN)
            move_client_to_screen(c, rule->screen, true);
        tag_client_with_rule(c, rule);

        switch(rule->isfloating)
        {
          case Maybe:
            break;
          case Yes:
            client_setfloating(c, true, c->layer != LAYER_TILE ? c->layer : LAYER_FLOAT);
            break;
          case No:
            client_setfloating(c, false, LAYER_TILE);
            break;
        }

        if(rule->opacity >= 0.0f)
            window_settrans(c->win, rule->opacity);
    }

    /* check for transient and set tags like its parent */
    if((rettrans = xutil_get_transient_for_hint(globalconf.connection, w, &trans))
       && (t = client_get_bywin(globalconf.clients, trans)))
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            if(is_client_tagged(t, tag))
                tag_client(c, tag);

    /* should be floating if transsient or fixed */
    if(rettrans || c->isfixed)
        client_setfloating(c, true, c->layer != LAYER_TILE ? c->layer : LAYER_FLOAT);

    /* titlebar init */
    if(rule && rule->titlebar.position != Auto)
        c->titlebar = rule->titlebar;

    titlebar_init(c);

    if(!retloadprops
       && u_size_hints
       && !(xcb_size_hints_is_us_position(u_size_hints) | xcb_size_hints_is_p_position(u_size_hints)))
    {
        if(c->isfloating && !c->ismax)
            client_resize(c, globalconf.screens[c->screen].floating_placement(c), false);
        else
            c->f_geometry = globalconf.screens[c->screen].floating_placement(c);

        xcb_free_size_hints(u_size_hints);
    }

    /* update titlebar with real floating info now */
    titlebar_update_geometry_floating(c);

    const uint32_t select_input_val[] = {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |
        XCB_EVENT_MASK_ENTER_WINDOW };

    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK,
                                 select_input_val);

    /* handle xshape */
    if(globalconf.have_shape)
    {
        xcb_shape_select_input(globalconf.connection, w, true);
        window_setshape(c->win, c->phys_screen);
    }

    /* attach to the stack */
    if(rule)
        switch(rule->ismaster)
        {
          case Yes:
            client_list_push(&globalconf.clients, c);
            break;
          case No:
            client_list_append(&globalconf.clients, c);
            break;
          case Maybe:
            rule = NULL;
            break;
        }

    if(!rule)
    {
        if(globalconf.screens[c->screen].new_become_master)
            client_list_push(&globalconf.clients, c);
        else
            client_list_append(&globalconf.clients, c);
    }

    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    ewmh_update_net_client_list(c->phys_screen);
}

static area_t
client_geometry_hints(Client *c, area_t geometry)
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

/** Resize client window
 * \param c client to resize
 * \param geometry new window geometry
 * \param hints use resize hints
 * \param return true if resize has been done
 */
bool
client_resize(Client *c, area_t geometry, bool hints)
{
    int new_screen;
    area_t area;
    Layout *layout = layout_get_current(c->screen);
    bool resized = false;

    if(!c->ismoving && !c->isfloating && layout->arrange != layout_floating)
    {
        titlebar_update_geometry(c, geometry);
        geometry = titlebar_geometry_remove(&c->titlebar, geometry);
    }

    /* Values to configure a window is an array where values are
     * stored according to 'value_mask' */
    uint32_t values[5];

    if(hints)
        geometry = client_geometry_hints(c, geometry);

    if(geometry.width <= 0 || geometry.height <= 0)
        return false;

    /* offscreen appearance fixes */
    area = get_display_area(c->phys_screen, NULL,
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
           || layout_get_current(new_screen)->arrange == layout_floating)
        {
            if(!c->ismax)
                c->f_geometry = geometry;
            titlebar_update_geometry_floating(c);
        }

        xcb_configure_window(globalconf.connection, c->win,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             values);
        window_configure(c->win, geometry, c->border);

        if(c->screen != new_screen)
            move_client_to_screen(c, new_screen, false);

        resized = true;
    }

    /* call it again like it was floating,
     * we want it to be sticked to the window */
    if(!c->ismoving && !c->isfloating && layout->arrange != layout_floating)
       titlebar_update_geometry_floating(c);

    return resized;
}

void
client_setfloating(Client *c, bool floating, Layer layer)
{
    if(c->isfloating != floating)
    {
        if((c->isfloating = floating))
            client_resize(c, c->f_geometry, false);
        else if(c->ismax)
        {
            c->ismax = false;
            client_resize(c, c->m_geometry, false);
        }
        if(client_isvisible(c, c->screen))
            globalconf.screens[c->screen].need_arrange = true;
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        if(floating)
        {
            c->oldlayer = c->layer;
            c->layer = layer;
        }
        else
        {
            c->layer = c->oldlayer;
        }
        client_stack(c);
        client_saveprops(c);
    }
}

/** Save client properties as an X property
 * \param c client
 */
void
client_saveprops(Client *c)
{
    int i = 0, ntags = 0;
    char *prop;
    Tag *tag;

    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        ntags++;

    prop = p_new(char, ntags + 3);

    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next, i++)
        prop[i] = is_client_tagged(c, tag) ? '1' : '0';

    prop[i] = c->isfloating ? '1' : '0';

    sprintf(&prop[++i], "%d", c->layer);

    prop[++i] = '\0';

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, c->win,
                        xutil_intern_atom(globalconf.connection, "_AWESOME_PROPERTIES"),
                        STRING, 8, i, (unsigned char *) prop);

    p_delete(&prop);
}

void
client_unban(Client *c)
{
    xcb_map_window(globalconf.connection, c->win);
    window_setstate(c->win, XCB_WM_NORMAL_STATE);
    if(c->titlebar.sw && c->titlebar.position != Off)
        xcb_map_window(globalconf.connection, c->titlebar.sw->window);
}

void
client_unmanage(Client *c)
{
    Tag *tag;

    /* The server grab construct avoids race conditions. */
    xcb_grab_server(globalconf.connection);

    xcb_configure_window(globalconf.connection, c->win,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         (uint32_t *) &c->oldborder);

    /* remove client everywhere */
    client_list_detach(&globalconf.clients, c);
    focus_delete_client(c);
    if(globalconf.scratch.client == c)
        globalconf.scratch.client = NULL;
    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        untag_client(c, tag);

    if(globalconf.focus->client == c)
        client_focus(NULL, c->screen, true);

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, c->win, ANY_MODIFIER);
    window_setstate(c->win, XCB_WM_WITHDRAWN_STATE);

    xcb_aux_sync(globalconf.connection);
    xcb_ungrab_server(globalconf.connection);

    if(c->titlebar.sw)
        simplewindow_delete(&c->titlebar.sw);

    p_delete(&c);
}

void
client_updatewmhints(Client *c)
{
    xcb_wm_hints_t *wmh = NULL;

    if((wmh = xcb_get_wm_hints(globalconf.connection, c->win)))
    {
        if((c->isurgent = (xcb_wm_hints_is_x_urgency_hint(wmh) && globalconf.focus->client != c)))
        {
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
            titlebar_draw(c);
        }
        if(xcb_wm_hints_is_state_hint(wmh) && xcb_wm_hints_state_is_withdrawn(wmh))
        {
            c->border = 0;
            c->skip = true;
        }
        free(wmh);
    }
}

xcb_size_hints_t *
client_updatesizehints(Client *c)
{
    long msize;
    xcb_size_hints_t *size = xcb_alloc_size_hints();

    if(!xcb_get_wm_normal_hints(globalconf.connection, c->win, size, &msize) ||
        size == NULL)
        return NULL;

    if(xcb_size_hints_is_p_size(size))
        xcb_size_hints_get_base_size(size, &c->basew, &c->baseh);
    else if(xcb_size_hints_is_p_min_size(size))
        xcb_size_hints_get_min_size(size, &c->basew, &c->baseh);
    else
        c->basew = c->baseh = 0;
    if(xcb_size_hints_is_p_resize_inc(size))
        xcb_size_hints_get_increase(size, &c->incw, &c->inch);
    else
        c->incw = c->inch = 0;

    if(xcb_size_hints_is_p_max_size(size))
        xcb_size_hints_get_max_size(size, &c->maxw, &c->maxh);
    else
        c->maxw = c->maxh = 0;

    if(xcb_size_hints_is_p_min_size(size))
        xcb_size_hints_get_min_size(size, &c->minw, &c->minh);
    else if(xcb_size_hints_is_p_base_size(size))
        xcb_size_hints_get_base_size(size, &c->minw, &c->minh);
    else
        c->minw = c->minh = 0;

    if(xcb_size_hints_is_p_aspect(size))
    {
        xcb_size_hints_get_min_aspect(size, &c->minax, &c->minay);
        xcb_size_hints_get_max_aspect(size, &c->maxax, &c->maxay);
    }
    else
        c->minax = c->maxax = c->minay = c->maxay = 0;

    if(c->maxw && c->minw && c->maxh && c->minh
       && c->maxw == c->minw && c->maxh == c->minh)
        c->isfixed = true;

    return size;
}

/** Returns true if a client is tagged
 * with one of the tags
 * \return true or false
 */
bool
client_isvisible(Client *c, int screen)
{
    Tag *tag;

    if(!c || c->screen != screen)
        return false;

    if(globalconf.scratch.client == c)
        return globalconf.scratch.isvisible;

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected && is_client_tagged(c, tag))
            return true;
    return false;
}

/** Set the transparency of the selected client.
 * Argument should be an absolut or relativ floating between 0.0 and 1.0
 * \param screen Screen ID
 * \param arg unused arg
 * \ingroup ui_callback
 */
void
uicb_client_settrans(int screen __attribute__ ((unused)), char *arg)
{
    double delta = 1.0, current_opacity = 100.0;
    unsigned int current_opacity_raw = 0;
    int set_prop = 0;
    Client *sel = globalconf.focus->client;
    xcb_get_property_reply_t *prop_r;

    if(!sel)
        return;

    prop_r = xcb_get_property_reply(globalconf.connection,
                                    xcb_get_property_unchecked(globalconf.connection,
                                                               false, sel->win,
                                                               xutil_intern_atom(globalconf.connection, "_NET_WM_WINDOW_OPACITY"),
                                                               CARDINAL,
                                                               0, 1),
                                    NULL);

    if(prop_r)
    {
        memcpy(&current_opacity_raw, xcb_get_property_value(prop_r), sizeof(unsigned int));
        current_opacity = (double) current_opacity_raw / 0xffffffff;
        p_delete(&prop_r);
    }
    else
        set_prop = 1;

    delta = compute_new_value_from_arg(arg, current_opacity);

    if(delta <= 0.0)
        delta = 0.0;
    else if(delta > 1.0)
    {
        delta = 1.0;
        set_prop = 1;
    }

    if(delta == 1.0 && !set_prop)
        window_settrans(sel->win, -1);
    else
        window_settrans(sel->win, delta);
}

/** Find a visible client on screen. Return next client or previous client if
 * reverse is true.
 * \param sel current selected client
 * \param reverse return previous instead of next if true
 * \return next or previous client
 */
static Client *
client_find_visible(Client *sel, bool reverse)
{
    Client *next;
    Client *(*client_iter)(Client **, Client *) = client_list_next_cycle;

    if(!sel) return NULL;

    if(reverse)
        client_iter = client_list_prev_cycle;

    /* look for previous or next starting at sel */

    for(next = client_iter(&globalconf.clients, sel);
        next && next != sel && (next->skip || !client_isvisible(next, sel->screen));
        next = client_iter(&globalconf.clients, next));

    return next;
}

/** Swap the currently focused client with the previous visible one.
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapprev(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    Client *prev;

    if((prev = client_find_visible(globalconf.focus->client, true)))
    {
        client_list_swap(&globalconf.clients, prev, globalconf.focus->client);
        globalconf.screens[prev->screen].need_arrange = true;
        widget_invalidate_cache(prev->screen, WIDGET_CACHE_CLIENTS);
    }
}

/** Swap the currently focused client with the next visible one.
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_client_swapnext(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    Client *next;

    if((next = client_find_visible(globalconf.focus->client, false)))
    {
        client_list_swap(&globalconf.clients, globalconf.focus->client, next);
        globalconf.screens[next->screen].need_arrange = true;
        widget_invalidate_cache(next->screen, WIDGET_CACHE_CLIENTS);
    }
}

/** Move and resize a client. Argument should be in format "x y w h" with
 * absolute (1, 20, 300, ...) or relative (+10, -200, ...) values.
 * \param screen Screen ID
 * \param arg x y w h
 * \ingroup ui_callback
 */
void
uicb_client_moveresize(int screen, char *arg)
{
    int ox, oy, ow, oh; /* old geometry */
    char x[8], y[8], w[8], h[8];
    int nmx, nmy;
    area_t geometry;
    Client *sel = globalconf.focus->client;
    xcb_query_pointer_reply_t *xqp;
    Layout *curlay = layout_get_current(screen);

    if(!sel || sel->isfixed || !arg ||
       (curlay->arrange != layout_floating && !sel->isfloating))
        return;

    if(sscanf(arg, "%s %s %s %s", x, y, w, h) != 4)
        return;

    geometry.x = (int) compute_new_value_from_arg(x, sel->geometry.x);
    geometry.y = (int) compute_new_value_from_arg(y, sel->geometry.y);
    geometry.width = (int) compute_new_value_from_arg(w, sel->geometry.width);
    geometry.height = (int) compute_new_value_from_arg(h, sel->geometry.height);

    ox = sel->geometry.x;
    oy = sel->geometry.y;
    ow = sel->geometry.width;
    oh = sel->geometry.height;

    xqp = xcb_query_pointer_reply(globalconf.connection,
                                  xcb_query_pointer_unchecked(globalconf.connection,
                                                              xcb_aux_get_screen(globalconf.connection, sel->phys_screen)->root),
                                  NULL);
    if(globalconf.screens[sel->screen].resize_hints)
        geometry = client_geometry_hints(sel, geometry);
    client_resize(sel, geometry, false);
    if (xqp)
    {
        if(ox <= xqp->root_x && (ox + 2 * sel->border + ow) >= xqp->root_x &&
           oy <= xqp->root_y && (oy + 2 * sel->border + oh) >= xqp->root_y)
        {
            nmx = xqp->root_x - (ox + sel->border) + sel->geometry.width - ow;
            nmy = xqp->root_y - (oy + sel->border) + sel->geometry.height - oh;

            if(nmx < -sel->border) /* can happen on a resize */
                nmx = -sel->border;
            if(nmy < -sel->border)
                nmy = -sel->border;

            xcb_warp_pointer(globalconf.connection,
                             XCB_NONE, sel->win,
                             0, 0, 0, 0, nmx, nmy);
        }

        p_delete(&xqp);
    }
}

/** Kill a client via a WM_DELETE_WINDOW request or XKillClient if not
 * supported.
 * \param c the client to kill
 */
void
client_kill(Client *c)
{
    xcb_client_message_event_t ev;

    if(client_isprotodel(globalconf.connection, c->win))
    {
        /* Initialize all of event's fields first */
        memset(&ev, 0, sizeof(ev));

        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = c->win;
        ev.type = xutil_intern_atom(globalconf.connection, "WM_PROTOCOLS");
        ev.format = 32;

        ev.data.data32[0] = xutil_intern_atom(globalconf.connection, "WM_DELETE_WINDOW");
        ev.data.data32[1] = XCB_CURRENT_TIME;

        /* TODO: really useful? */
        ev.data.data32[2] = ev.data.data32[3] = ev.data.data32[4] = 0;

        xcb_send_event(globalconf.connection, false, c->win,
                       XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    }
    else
        xcb_kill_client(globalconf.connection, c->win);
}

/** Kill the currently focused client.
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_kill(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;

    if(sel)
        client_kill(sel);
}

/** Maximize the client to the given geometry.
 * \param c the client to maximize
 * \param geometry the geometry to use for maximizing
 */
static void
client_maximize(Client *c, area_t geometry)
{
    if((c->ismax = !c->ismax))
    {
        /* disable titlebar */
        c->titlebar.position = Off;
        c->wasfloating = c->isfloating;
        c->m_geometry = c->geometry;
        if(layout_get_current(c->screen)->arrange != layout_floating)
            client_setfloating(c, true, LAYER_FULLSCREEN);
        client_focus(c, c->screen, true);
        client_resize(c, geometry, false);
    }
    else if(c->wasfloating)
    {
        c->titlebar.position = c->titlebar.dposition;
        client_setfloating(c, true, LAYER_FULLSCREEN);
        client_resize(c, c->m_geometry, false);
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    }
    else if(layout_get_current(c->screen)->arrange == layout_floating)
    {
        c->titlebar.position = c->titlebar.dposition;
        client_resize(c, c->m_geometry, false);
    }
    else
    {
        c->titlebar.position = c->titlebar.dposition;
        client_setfloating(c, false, LAYER_TILE);
    }
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
}

/** Toggle maximization state for the focused client.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglemax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.width -= 2 * sel->border;
        area.height -= 2 * sel->border;
        client_maximize(sel, area);
    }
}

/** Toggle vertical maximization for the focused client.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_toggleverticalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.x = sel->geometry.x;
        area.width = sel->geometry.width;
        area.height -= 2 * sel->border;
        client_maximize(sel, area);
    }
}


/** Toggle horizontal maximization for the focused client.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_togglehorizontalmax(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    area_t area = screen_get_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(sel)
    {
        area.y = sel->geometry.y;
        area.height = sel->geometry.height;
        area.width -= 2 * sel->border;
        client_maximize(sel, area);
    }
}

/** Move the client to the master area.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_zoom(int screen, char *arg __attribute__ ((unused)))
{
    Client *c, *sel = globalconf.focus->client;

    if(!sel)
        return;

    for(c = globalconf.clients; !client_isvisible(c, screen); c = c->next);
    if(c == sel)
         for(sel = sel->next; sel && !client_isvisible(sel, screen); sel = sel->next);

    if(sel)
    {
        client_list_detach(&globalconf.clients, sel);
        client_list_push(&globalconf.clients, sel);
        globalconf.screens[screen].need_arrange = true;
    }
}

/** Give focus to the next visible client in the stack.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusnext(int screen, char *arg __attribute__ ((unused)))
{
    Client *next;

    if((next = client_find_visible(globalconf.focus->client, false)))
        client_focus(next, screen, true);
}

/** Give focus to the previous visible client in the stack.
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_client_focusprev(int screen, char *arg __attribute__ ((unused)))
{
    Client *prev;

    if((prev = client_find_visible(globalconf.focus->client, true)))
        client_focus(prev, screen, true);
}

/** Toggle the floating state of the focused client.
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_togglefloating(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    if(globalconf.focus->client)
        client_setfloating(globalconf.focus->client, !globalconf.focus->client->isfloating,
                globalconf.focus->client->layer == LAYER_FLOAT ? LAYER_TILE : LAYER_FLOAT);
}

/** Toggle the scratch client attribute on the focused client.
 * \param screen screen number
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_setscratch(int screen, char *arg __attribute__ ((unused)))
{
    if(!globalconf.focus->client)
        return;

    if(globalconf.scratch.client == globalconf.focus->client)
        globalconf.scratch.client = NULL;
    else
        globalconf.scratch.client = globalconf.focus->client;

    widget_invalidate_cache(screen, WIDGET_CACHE_CLIENTS | WIDGET_CACHE_TAGS);
    globalconf.screens[screen].need_arrange = true;
}

/** Toggle the scratch client's visibility.
 * \param screen screen number
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_togglescratch(int screen, char *arg __attribute__ ((unused)))
{
    if(globalconf.scratch.client)
    {
        globalconf.scratch.isvisible = !globalconf.scratch.isvisible;
        if(globalconf.scratch.isvisible)
            client_focus(globalconf.scratch.client, screen, true);
        globalconf.screens[globalconf.scratch.client->screen].need_arrange = true;
        widget_invalidate_cache(globalconf.scratch.client->screen, WIDGET_CACHE_CLIENTS);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
