/*
 * event.c - event handlers
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

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

#include "awesome.h"
#include "event.h"
#include "tag.h"
#include "window.h"
#include "ewmh.h"
#include "client.h"
#include "widget.h"
#include "titlebar.h"
#include "key.h"
#include "keygrabber.h"
#include "mousegrabber.h"
#include "luaa.h"
#include "systray.h"
#include "screen.h"
#include "common/atoms.h"

extern awesome_t globalconf;

/** Handle mouse button events.
 * \param c The client on which the event happened or NULL.
 * \param type Event type, press or release.
 * \param button Button number.
 * \param state Modkeys state.
 * \param buttons Buttons array to check for.
 */
static void
event_handle_mouse_button(client_t *c,
                          uint8_t type,
                          xcb_button_t button,
                          uint16_t state,
                          button_array_t *buttons)
{
    for(int i = 0; i < buttons->len; i++)
        if(button == buttons->tab[i]->button
           && XUTIL_MASK_CLEAN(state) == buttons->tab[i]->mod)
            switch(type)
            {
              case XCB_BUTTON_PRESS:
                if(buttons->tab[i]->press != LUA_REFNIL)
                {
                    if(c)
                    {
                        luaA_client_userdata_new(globalconf.L, c);
                        luaA_dofunction(globalconf.L, buttons->tab[i]->press, 1, 0);
                    }
                    else
                        luaA_dofunction(globalconf.L, buttons->tab[i]->press, 0, 0);
                }
                break;
              case XCB_BUTTON_RELEASE:
                if(buttons->tab[i]->release != LUA_REFNIL)
                {
                    if(c)
                    {
                        luaA_client_userdata_new(globalconf.L, c);
                        luaA_dofunction(globalconf.L, buttons->tab[i]->release, 1, 0);
                    }
                    else
                        luaA_dofunction(globalconf.L, buttons->tab[i]->release, 0, 0);
                }
                break;
            }
}

/** Handle an event with mouse grabber if needed
 * \param x The x coordinate.
 * \param y The y coordinate.
 * \param mask The mask buttons.
 */
static void
event_handle_mousegrabber(int x, int y, uint16_t mask)
{
    if(globalconf.mousegrabber != LUA_REFNIL)
    {
        lua_rawgeti(globalconf.L, LUA_REGISTRYINDEX, globalconf.mousegrabber);
        mousegrabber_handleevent(globalconf.L, x, y, mask);
        if(lua_pcall(globalconf.L, 1, 1, 0))
        {
            warn("error running function: %s", lua_tostring(globalconf.L, -1));
            luaA_mousegrabber_stop(globalconf.L);
        }
        else if(!lua_isboolean(globalconf.L, -1) || !lua_toboolean(globalconf.L, -1))
            luaA_mousegrabber_stop(globalconf.L);
        lua_pop(globalconf.L, 1);  /* pop returned value */
    }
}

/** The button press event handler.
 * \param data The type of mouse event.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_button(void *data, xcb_connection_t *connection, xcb_button_press_event_t *ev)
{
    int screen;
    const int nb_screen = xcb_setup_roots_length(xcb_get_setup(connection));
    client_t *c;
    widget_t *w;
    wibox_t *wibox;

    /* ev->state is
     * button status (8 bits) + modifiers status (8 bits)
     * we don't care for button status that we get, especially on release, so
     * drop them */
    ev->state &= 0x00ff;

    event_handle_mousegrabber(ev->root_x, ev->root_y, ev->state);

    if((wibox = wibox_getbywin(ev->event))
       || (wibox = wibox_getbywin(ev->child)))
    {
        /* If the wibox is child, then x,y are
         * relative to root window */
        if(wibox->sw.window == ev->child)
        {
            ev->event_x -= wibox->sw.geometry.x;
            ev->event_y -= wibox->sw.geometry.y;
        }

        /* check if we match a binding on the wibox */
        button_array_t *b = &wibox->buttons;

        for(int i = 0; i < b->len; i++)
            if(ev->detail == b->tab[i]->button
               && XUTIL_MASK_CLEAN(ev->state) == b->tab[i]->mod)
                switch(ev->response_type)
                {
                  case XCB_BUTTON_PRESS:
                    if(b->tab[i]->press != LUA_REFNIL)
                    {
                        luaA_wibox_userdata_new(globalconf.L, wibox);
                        luaA_dofunction(globalconf.L, b->tab[i]->press, 1, 0);
                    }
                    break;
                  case XCB_BUTTON_RELEASE:
                    if(b->tab[i]->release != LUA_REFNIL)
                    {
                        luaA_wibox_userdata_new(globalconf.L, wibox);
                        luaA_dofunction(globalconf.L, b->tab[i]->release, 1, 0);
                    }
                    break;
                }

        /* then try to match a widget binding */
        if((w = widget_getbycoords(wibox->position, &wibox->widgets,
                                   wibox->sw.geometry.width,
                                   wibox->sw.geometry.height,
                                   &ev->event_x, &ev->event_y)))
        {
            b = &w->buttons;

            for(int i = 0; i < b->len; i++)
                if(ev->detail == b->tab[i]->button
                   && XUTIL_MASK_CLEAN(ev->state) == b->tab[i]->mod)
                    switch(ev->response_type)
                    {
                      case XCB_BUTTON_PRESS:
                        if(b->tab[i]->press != LUA_REFNIL)
                        {
                            luaA_wibox_userdata_new(globalconf.L, wibox);
                            luaA_dofunction(globalconf.L, b->tab[i]->press, 1, 0);
                        }
                        break;
                      case XCB_BUTTON_RELEASE:
                        if(b->tab[i]->release != LUA_REFNIL)
                        {
                            luaA_wibox_userdata_new(globalconf.L, wibox);
                            luaA_dofunction(globalconf.L, b->tab[i]->release, 1, 0);
                        }
                        break;
                    }
        }
        /* return even if no widget match */
        return 0;
    }
    else if((c = client_getbywin(ev->event)))
    {
        event_handle_mouse_button(c, ev->response_type, ev->detail, ev->state, &c->buttons);
        xcb_allow_events(globalconf.connection,
                         XCB_ALLOW_REPLAY_POINTER,
                         XCB_CURRENT_TIME);
    }
    else if(ev->child == XCB_NONE)
    {
        for(screen = 0; screen < nb_screen; screen++)
            if(xutil_screen_get(connection, screen)->root == ev->event)
            {
                event_handle_mouse_button(NULL, ev->response_type, ev->detail, ev->state, &globalconf.buttons);
                return 0;
            }
    }

    return 0;
}

static void
event_handle_configurerequest_configure_window(xcb_configure_request_event_t *ev)
{
    uint16_t config_win_mask = 0;
    uint32_t config_win_vals[7];
    unsigned short i = 0;

    if(ev->value_mask & XCB_CONFIG_WINDOW_X)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_X;
        config_win_vals[i++] = ev->x;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_Y;
        config_win_vals[i++] = ev->y;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_WIDTH;
        config_win_vals[i++] = ev->width;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_HEIGHT;
        config_win_vals[i++] = ev->height;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
        config_win_vals[i++] = ev->border_width;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_SIBLING)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_SIBLING;
        config_win_vals[i++] = ev->sibling;
    }
    if(ev->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
    {
        config_win_mask |= XCB_CONFIG_WINDOW_STACK_MODE;
        config_win_vals[i++] = ev->stack_mode;
    }

    xcb_configure_window(globalconf.connection, ev->window, config_win_mask, config_win_vals);
}

/** The configure event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_configurerequest(void *data __attribute__ ((unused)),
                              xcb_connection_t *connection, xcb_configure_request_event_t *ev)
{
    client_t *c;
    area_t geometry;

    if((c = client_getbywin(ev->window)))
    {
        geometry = titlebar_geometry_remove(c->titlebar, c->border, c->geometry);

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
            geometry.x = ev->x;
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
            geometry.y = ev->y;
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            geometry.width = ev->width;
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            geometry.height = ev->height;

        if(ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
            client_setborder(c, ev->border_width);

        /* Clients are not allowed to directly mess with stacking parameters. */
        ev->value_mask &= ~(XCB_CONFIG_WINDOW_SIBLING |
                                            XCB_CONFIG_WINDOW_STACK_MODE);

        if(c->isbanned)
        {
            /* We'll be sending protocol geometry, so don't readd borders and titlebar. */
            /* We do have to ensure the windows don't end up in the visible screen. */
            ev->x = geometry.x = - (geometry.width + 2*c->border);
            ev->y = geometry.y = - (geometry.height + 2*c->border);
            window_configure(c->win, geometry, c->border);
            event_handle_configurerequest_configure_window(ev);
        }
        else
        {
            /** Configure request are sent with size relative to real (internal)
             * window size, i.e. without titlebars and borders. */
            geometry = titlebar_geometry_add(c->titlebar, c->border, geometry);

            if(client_resize(c, geometry, false))
            {
                /* All the wiboxes (may) need to be repositioned. */
                if(client_hasstrut(c))
                    wibox_update_positions();
                client_need_arrange(c);
            }
            else
            {
                /* Resize wasn't officially needed, but we don't want to break expectations. */
                geometry = titlebar_geometry_remove(c->titlebar, c->border, c->geometry);
                window_configure(c->win, geometry, c->border);
            }
        }
    }
    else
        event_handle_configurerequest_configure_window(ev);

    return 0;
}

/** The configure notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_configurenotify(void *data __attribute__ ((unused)),
                             xcb_connection_t *connection, xcb_configure_notify_event_t *ev)
{
    int screen_nbr;
    const xcb_screen_t *screen;

    for(screen_nbr = 0; screen_nbr < xcb_setup_roots_length(xcb_get_setup (connection)); screen_nbr++)
        if((screen = xutil_screen_get(connection, screen_nbr)) != NULL
           && ev->window == screen->root
           && (ev->width != screen->width_in_pixels
               || ev->height != screen->height_in_pixels))
            /* it's not that we panic, but restart */
            awesome_restart();

    return 0;
}

/** The destroy notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_destroynotify(void *data __attribute__ ((unused)),
                           xcb_connection_t *connection __attribute__ ((unused)),
                           xcb_destroy_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
        client_unmanage(c);
    else
        for(int i = 0; i < globalconf.embedded.len; i++)
            if(globalconf.embedded.tab[i].win == ev->window)
            {
                xembed_window_array_take(&globalconf.embedded, i);
                for(int j = 0; j < globalconf.nscreen; j++)
                    widget_invalidate_bytype(j, widget_systray);
            }

    return 0;
}

/** Handle a motion notify over widgets.
 * \param object The object.
 * \param mouse_over The pointer to the registered mouse over widget.
 * \param widget The new widget the mouse is over.
 */
static void
event_handle_widget_motionnotify(void *object,
                                 widget_t **mouse_over,
                                 widget_t *widget)
{
    if(widget != *mouse_over)
    {
        if(*mouse_over)
        {
            if((*mouse_over)->mouse_leave != LUA_REFNIL)
            {
                /* call mouse leave function on old widget */
                luaA_wibox_userdata_new(globalconf.L, object);
                luaA_dofunction(globalconf.L, (*mouse_over)->mouse_leave, 1, 0);
            }
        }
        if(widget)
        {
            /* call mouse enter function on new widget and register it */
            *mouse_over = widget;
            if(widget->mouse_enter != LUA_REFNIL)
            {
                luaA_wibox_userdata_new(globalconf.L, object);
                luaA_dofunction(globalconf.L, widget->mouse_enter, 1, 0);
            }
        }
    }
}

/** The motion notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_motionnotify(void *data __attribute__ ((unused)),
                          xcb_connection_t *connection,
                          xcb_motion_notify_event_t *ev)
{
    wibox_t *wibox;
    widget_t *w;

    event_handle_mousegrabber(ev->root_x, ev->root_y, ev->state);

    if((wibox = wibox_getbywin(ev->event))
       && (w = widget_getbycoords(wibox->position, &wibox->widgets,
                                  wibox->sw.geometry.width,
                                  wibox->sw.geometry.height,
                                  &ev->event_x, &ev->event_y)))
        event_handle_widget_motionnotify(wibox, &wibox->mouse_over, w);

    return 0;
}

/** The leave notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_leavenotify(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection,
                         xcb_leave_notify_event_t *ev)
{
    wibox_t *wibox;
    client_t *c;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return 0;

    if((c = client_getbywin(ev->event)))
    {
        if(globalconf.hooks.mouse_leave != LUA_REFNIL)
        {
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.mouse_leave, 1, 0);
        }
    }
    else if((wibox = wibox_getbywin(ev->event)))
    {
        if(wibox->mouse_over)
        {
            if(wibox->mouse_over->mouse_leave != LUA_REFNIL)
            {
                /* call mouse leave function on widget the mouse was over */
                luaA_wibox_userdata_new(globalconf.L, wibox);
                luaA_dofunction(globalconf.L, wibox->mouse_over->mouse_leave, 1, 0);
            }
            wibox->mouse_over = NULL;
        }

        if(wibox->mouse_leave != LUA_REFNIL)
            luaA_dofunction(globalconf.L, wibox->mouse_leave, 0, 0);
    }

    return 0;
}

/** The enter notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_enternotify(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection,
                         xcb_enter_notify_event_t *ev)
{
    client_t *c;
    widget_t *w;
    wibox_t *wibox;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return 0;

    if((wibox = wibox_getbywin(ev->event)))
    {
        if((w = widget_getbycoords(wibox->position, &wibox->widgets,
                                   wibox->sw.geometry.width,
                                   wibox->sw.geometry.height,
                                   &ev->event_x, &ev->event_y)))
            event_handle_widget_motionnotify(wibox, &wibox->mouse_over, w);

        if(wibox->mouse_enter != LUA_REFNIL)
            luaA_dofunction(globalconf.L, wibox->mouse_enter, 0, 0);
    }
    else if((c = client_getbytitlebarwin(ev->event))
       || (c = client_getbywin(ev->event)))
    {
        if(globalconf.hooks.mouse_enter != LUA_REFNIL)
        {
            luaA_client_userdata_new(globalconf.L, c);
            luaA_dofunction(globalconf.L, globalconf.hooks.mouse_enter, 1, 0);
        }
    }

    return 0;
}

/** The focus in event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_focusin(void *data __attribute__ ((unused)),
                     xcb_connection_t *connection,
                     xcb_focus_in_event_t *ev)
{
    client_t *c;

    /* Events that we are interested in: */
    switch(ev->detail)
    {
        /* These are events that jump between windows of a toplevel client.
         *
         * NotifyVirtual event is handled in case where NotifyAncestor or
         * NotifyInferior event is not generated on window that is managed by
         * awesome ( client_* returns NULL )
         *
         * Can someone explain exactly why they are needed ?
         */
        case XCB_NOTIFY_DETAIL_VIRTUAL:
        case XCB_NOTIFY_DETAIL_ANCESTOR:
        case XCB_NOTIFY_DETAIL_INFERIOR:

        /* These are events that jump between clients.
         * Virtual events ensure we always get an event on our top-level window.
         */
        case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
        case XCB_NOTIFY_DETAIL_NONLINEAR:
            if((c = client_getbytitlebarwin(ev->event))
               || (c = client_getbywin(ev->event)))
                client_focus_update(c);
        /* all other events are ignored */
        default:
            return 0;
    }
}

/** The focus out event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_focusout(void *data __attribute__ ((unused)),
                          xcb_connection_t *connection,
                          xcb_focus_out_event_t *ev)
{
    client_t *c;

    /* Events that we are interested in: */
    switch(ev->detail)
    {
        /* These are events that jump between clients.
         * Virtual events ensure we always get an event on our top-level window.
         */
        case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
        case XCB_NOTIFY_DETAIL_NONLINEAR:
            if((c = client_getbytitlebarwin(ev->event))
               || (c = client_getbywin(ev->event)))
                client_unfocus_update(c);
        /* all other events are ignored */
        default:
            return 0;
    }
}

/** The expose event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_expose(void *data __attribute__ ((unused)),
                    xcb_connection_t *connection __attribute__ ((unused)),
                    xcb_expose_event_t *ev)
{
    wibox_t *wibox;

    if((wibox = wibox_getbywin(ev->window)))
        simplewindow_refresh_pixmap_partial(&wibox->sw,
                                            ev->x, ev->y,
                                            ev->width, ev->height);

    return 0;
}

/** The key press event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_key(void *data __attribute__ ((unused)),
                 xcb_connection_t *connection __attribute__ ((unused)),
                 xcb_key_press_event_t *ev)
{
    client_t *c;

    if(globalconf.keygrabber != LUA_REFNIL)
    {
        lua_rawgeti(globalconf.L, LUA_REGISTRYINDEX, globalconf.keygrabber);
        if(keygrabber_handlekpress(globalconf.L, ev))
        {
            if(lua_pcall(globalconf.L, 3, 1, 0))
            {
                warn("error running function: %s", lua_tostring(globalconf.L, -1));
                luaA_keygrabber_stop(globalconf.L);
            }
            else if(!lua_isboolean(globalconf.L, -1) || !lua_toboolean(globalconf.L, -1))
                luaA_keygrabber_stop(globalconf.L);
        }
        lua_pop(globalconf.L, 1);  /* pop returned value or function if not called */
    }
    else if((c = client_getbywin(ev->event)))
    {
        keyb_t *k = key_find(&c->keys, ev);

        if(k)
            switch(ev->response_type)
            {
              case XCB_KEY_PRESS:
                if(k->press != LUA_REFNIL)
                {
                    luaA_client_userdata_new(globalconf.L, c);
                    luaA_dofunction(globalconf.L, k->press, 1, 0);
                }
                break;
              case XCB_KEY_RELEASE:
                if(k->release != LUA_REFNIL)
                {
                    luaA_client_userdata_new(globalconf.L, c);
                    luaA_dofunction(globalconf.L, k->release, 1, 0);
                }
                break;
            }
    }
    else
    {
        keyb_t *k = key_find(&globalconf.keys, ev);

        if(k)
            switch(ev->response_type)
            {
              case XCB_KEY_PRESS:
                if(k->press != LUA_REFNIL)
                    luaA_dofunction(globalconf.L, k->press, 0, 0);
                break;
              case XCB_KEY_RELEASE:
                if(k->release != LUA_REFNIL)
                    luaA_dofunction(globalconf.L, k->release, 0, 0);
                break;
            }
    }

    return 0;
}

/** The map request event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_maprequest(void *data __attribute__ ((unused)),
                        xcb_connection_t *connection, xcb_map_request_event_t *ev)
{
    int phys_screen, ret = 0;
    client_t *c;
    xcb_get_window_attributes_cookie_t wa_c;
    xcb_get_window_attributes_reply_t *wa_r;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;

    wa_c = xcb_get_window_attributes_unchecked(connection, ev->window);

    if(!(wa_r = xcb_get_window_attributes_reply(connection, wa_c, NULL)))
        return -1;

    if(wa_r->override_redirect)
        goto bailout;

    if(xembed_getbywin(&globalconf.embedded, ev->window))
    {
        xcb_map_window(connection, ev->window);
        xembed_window_activate(connection, ev->window);
    }
    else if((c = client_getbywin(ev->window)))
    {
        /* Check that it may be visible, but not asked to be hidden */
        if(client_maybevisible(c, c->screen) && !c->ishidden)
        {
            client_setminimized(c, false);
            /* it will be raised, so just update ourself */
            client_raise(c);
        }
    }
    else
    {
        geom_c = xcb_get_geometry_unchecked(connection, ev->window);

        if(!(geom_r = xcb_get_geometry_reply(connection, geom_c, NULL)))
        {
            ret = -1;
            goto bailout;
        }

        phys_screen = xutil_root2screen(connection, geom_r->root);

        client_manage(ev->window, geom_r, phys_screen, false);

        p_delete(&geom_r);
    }

bailout:
    p_delete(&wa_r);
    return ret;
}

/** The unmap notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_unmapnotify(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection, xcb_unmap_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
    {
        if(ev->event == xutil_screen_get(connection, c->phys_screen)->root
           && XCB_EVENT_SENT(ev)
           && window_state_get_reply(window_state_get_unchecked(c->win)) == XCB_WM_STATE_NORMAL)
            client_unmanage(c);
    }
    else
        for(int i = 0; i < globalconf.embedded.len; i++)
            if(globalconf.embedded.tab[i].win == ev->window)
            {
                xembed_window_array_take(&globalconf.embedded, i);
                for(int j = 0; j < globalconf.nscreen; j++)
                    widget_invalidate_bytype(j, widget_systray);
            }

    return 0;
}

/** The randr screen change notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_randr_screen_change_notify(void *data __attribute__ ((unused)),
                                        xcb_connection_t *connection __attribute__ ((unused)),
                                        xcb_randr_screen_change_notify_event_t *ev)
{
    if(!globalconf.have_randr)
        return -1;

    /* Code  of  XRRUpdateConfiguration Xlib  function  ported to  XCB
     * (only the code relevant  to RRScreenChangeNotify) as the latter
     * doesn't provide this kind of function */
    if(ev->rotation & (XCB_RANDR_ROTATION_ROTATE_90 | XCB_RANDR_ROTATION_ROTATE_270))
        xcb_randr_set_screen_size(connection, ev->root, ev->height, ev->width,
                                  ev->mheight, ev->mwidth);
    else
        xcb_randr_set_screen_size(connection, ev->root, ev->width, ev->height,
                                  ev->mwidth, ev->mheight);

    /* XRRUpdateConfiguration also executes the following instruction
     * but it's not useful because SubpixelOrder is not used at all at
     * the moment
     *
     * XRenderSetSubpixelOrder(dpy, snum, scevent->subpixel_order);
     */

    awesome_restart();

    return 0;
}

/** The client message event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_clientmessage(void *data __attribute__ ((unused)),
                           xcb_connection_t *connection,
                           xcb_client_message_event_t *ev)
{
    client_t *c;

    if(ev->type == WM_CHANGE_STATE)
    {
        if((c = client_getbywin(ev->window))
           && ev->format == 32
           && ev->data.data32[0] == XCB_WM_STATE_ICONIC)
            client_setminimized(c, true);
    }
    else if(ev->type == _XEMBED)
        return xembed_process_client_message(ev);
    else if(ev->type == _NET_SYSTEM_TRAY_OPCODE)
        return systray_process_client_message(ev);
    return ewmh_process_client_message(ev);
}

/** The keymap change notify event handler.
 * \param data Unused data.
 * \param connection The connection to the X server.
 * \param ev The event.
 * \return Status code, 0 if everything's fine.
 */
static int
event_handle_mappingnotify(void *data,
                           xcb_connection_t *connection,
                           xcb_mapping_notify_event_t *ev)
{
    xcb_get_modifier_mapping_cookie_t xmapping_cookie;

    if(ev->request == XCB_MAPPING_MODIFIER
       || ev->request == XCB_MAPPING_KEYBOARD)
    {
        /* Send the request to get the NumLock, ShiftLock and CapsLock masks */
        xmapping_cookie = xcb_get_modifier_mapping_unchecked(globalconf.connection);

        /* Free and then allocate the key symbols */
        xcb_key_symbols_free(globalconf.keysyms);
        globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

        /* Process the reply of previously sent mapping request */
        xutil_lock_mask_get(globalconf.connection, xmapping_cookie,
                            globalconf.keysyms, &globalconf.numlockmask,
                            &globalconf.shiftlockmask, &globalconf.capslockmask);

        int nscreen = xcb_setup_roots_length(xcb_get_setup(connection));

        /* regrab everything */
        for(int phys_screen = 0; phys_screen < nscreen; phys_screen++)
        {
            xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);
            /* yes XCB_BUTTON_MASK_ANY is also for grab_key even if it's look weird */
            xcb_ungrab_key(connection, XCB_GRAB_ANY, s->root, XCB_BUTTON_MASK_ANY);
            window_grabkeys(s->root, &globalconf.keys);
        }

        for(client_t *c = globalconf.clients; c; c = c->next)
        {
            xcb_ungrab_key(connection, XCB_GRAB_ANY, c->win, XCB_BUTTON_MASK_ANY);
            window_grabkeys(c->win, &c->keys);
        }
    }

    return 0;
}

static int
event_handle_reparentnotify(void *data,
                           xcb_connection_t *connection,
                           xcb_reparent_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
        client_unmanage(c);

    return 0;
}

void a_xcb_set_event_handlers(void)
{
    const xcb_query_extension_reply_t *randr_query;

    xcb_event_set_button_press_handler(&globalconf.evenths, event_handle_button, NULL);
    xcb_event_set_button_release_handler(&globalconf.evenths, event_handle_button, NULL);
    xcb_event_set_configure_request_handler(&globalconf.evenths, event_handle_configurerequest, NULL);
    xcb_event_set_configure_notify_handler(&globalconf.evenths, event_handle_configurenotify, NULL);
    xcb_event_set_destroy_notify_handler(&globalconf.evenths, event_handle_destroynotify, NULL);
    xcb_event_set_enter_notify_handler(&globalconf.evenths, event_handle_enternotify, NULL);
    xcb_event_set_leave_notify_handler(&globalconf.evenths, event_handle_leavenotify, NULL);
    xcb_event_set_focus_in_handler(&globalconf.evenths, event_handle_focusin, NULL);
    xcb_event_set_focus_out_handler(&globalconf.evenths, event_handle_focusout, NULL);
    xcb_event_set_motion_notify_handler(&globalconf.evenths, event_handle_motionnotify, NULL);
    xcb_event_set_expose_handler(&globalconf.evenths, event_handle_expose, NULL);
    xcb_event_set_key_press_handler(&globalconf.evenths, event_handle_key, NULL);
    xcb_event_set_key_release_handler(&globalconf.evenths, event_handle_key, NULL);
    xcb_event_set_map_request_handler(&globalconf.evenths, event_handle_maprequest, NULL);
    xcb_event_set_unmap_notify_handler(&globalconf.evenths, event_handle_unmapnotify, NULL);
    xcb_event_set_client_message_handler(&globalconf.evenths, event_handle_clientmessage, NULL);
    xcb_event_set_mapping_notify_handler(&globalconf.evenths, event_handle_mappingnotify, NULL);
    xcb_event_set_reparent_notify_handler(&globalconf.evenths, event_handle_reparentnotify, NULL);

    /* check for randr extension */
    randr_query = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
    if((globalconf.have_randr = randr_query->present))
        xcb_event_set_handler(&globalconf.evenths,
                              randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY,
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
