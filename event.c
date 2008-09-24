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
#include "keybinding.h"
#include "keygrabber.h"
#include "luaa.h"
#include "systray.h"
#include "screen.h"
#include "layouts/floating.h"
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
        {
            if(c)
            {
                luaA_client_userdata_new(globalconf.L, c);
                luaA_dofunction(globalconf.L,
                                type == XCB_BUTTON_PRESS ?
                                buttons->tab[i]->press : buttons->tab[i]->release,
                                1, 0);
            }
            else
                luaA_dofunction(globalconf.L,
                                type == XCB_BUTTON_PRESS ?
                                buttons->tab[i]->press : buttons->tab[i]->release,
                                0, 0);
        }
}

/** Get a widget node from a wibox by coords.
 * \param Container position.
 * \param widgets The widget list.
 * \param width The container width.
 * \param height The container height.
 * \param x X coordinate of the widget.
 * \param y Y coordinate of the widget.
 * \return A widget node.
 */
static widget_node_t *
widget_getbycoords(position_t position, widget_node_t *widgets, int width, int height, int16_t *x, int16_t *y)
{
    int tmp;
    widget_node_t *w;

    /* Need to transform coordinates like it was top/bottom */
    switch(position)
    {
      case Right:
        tmp = *y;
        *y = width - *x;
        *x = tmp;
        break;
      case Left:
        tmp = *y;
        *y = *x;
        *x = height - tmp;
        break;
      default:
        break;
    }

    for(w = widgets; w; w = w->next)
        if(*x >= w->area.x && *x < w->area.x + w->area.width
           && *y >= w->area.y && *y < w->area.y + w->area.height)
            return w;

    return NULL;
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
    widget_node_t *w;
    wibox_t *wibox;

    /* ev->state is
     * button status (8 bits) + modifiers status (8 bits)
     * we don't care for button status that we get, especially on release, so
     * drop them */
    ev->state &= 0x00ff;

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
        if((w = widget_getbycoords(wibox->position, wibox->widgets,
                                   wibox->sw.geometry.width,
                                   wibox->sw.geometry.height,
                                   &ev->event_x, &ev->event_y)))
            w->widget->button(w, ev, wibox->screen, wibox);
        /* return even if no widget match */
        return 0;
    }

    if((c = client_getbywin(ev->event)))
    {
        event_handle_mouse_button(c, ev->response_type, ev->detail, ev->state, &c->buttons);
        xcb_allow_events(globalconf.connection,
                         XCB_ALLOW_REPLAY_POINTER,
                         XCB_CURRENT_TIME);
    }
    else
        for(screen = 0; screen < nb_screen; screen++)
            if(xutil_screen_get(connection, screen)->root == ev->event)
            {
                event_handle_mouse_button(NULL, ev->response_type, ev->detail, ev->state, &globalconf.buttons);
                return 0;
            }

    return 0;
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
        geometry = c->geometry;

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
            geometry.x = ev->x;
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
            geometry.y = ev->y;
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            geometry.width = ev->width;
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            geometry.height = ev->height;

        if(geometry.x != c->geometry.x || geometry.y != c->geometry.y
           || geometry.width != c->geometry.width || geometry.height != c->geometry.height)
        {
            if(client_isfloating(c) || layout_get_current(c->screen) == layout_floating)
            {
                client_resize(c, geometry, false);
                if(client_hasstrut(c))
                    /* All the wiboxes (may) need to be repositioned */
                    for(int screen = 0; screen < globalconf.nscreen; screen++)
                        for(int i = 0; i < globalconf.screens[screen].wiboxes.len; i++)
                        {
                            wibox_t *s = globalconf.screens[screen].wiboxes.tab[i];
                            wibox_position_update(s);
                        }
            }
            else
            {
                client_need_arrange(c);
                /* If we do not resize the client, at least tell it that it
                 * has its new configuration. That fixes at least
                 * gnome-terminal */
                window_configure(c->win, c->geometry, c->border);
            }
        }
        else
            window_configure(c->win, geometry, c->border);
    }
    else
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

        xcb_configure_window(connection, ev->window, config_win_mask, config_win_vals);
    }

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
    xembed_window_t *emwin;
    int i;

    if((c = client_getbywin(ev->window)))
        client_unmanage(c);
    else if((emwin = xembed_getbywin(globalconf.embedded, ev->event)))
    {
        xembed_window_list_detach(&globalconf.embedded, emwin);
        for(i = 0; i < globalconf.nscreen; i++)
            widget_invalidate_cache(i, WIDGET_CACHE_EMBEDDED);
    }

    return 0;
}

/** Handle a motion notify over widgets.
 * \param object The object.
 * \param mouse_over The pointer to the registered mouse over widget.
 * \param w The new widget the mouse is over.
 */
static void
event_handle_widget_motionnotify(void *object,
                                 widget_node_t **mouse_over,
                                 widget_node_t *w)
{
    if(w != *mouse_over)
    {
        if(*mouse_over)
        {
            /* call mouse leave function on old widget */
            luaA_wibox_userdata_new(globalconf.L, object);
            luaA_dofunction(globalconf.L, (*mouse_over)->widget->mouse_leave, 1, 0);
        }
        if(w)
        {
            /* call mouse enter function on new widget and register it */
            *mouse_over = w;
            luaA_wibox_userdata_new(globalconf.L, object);
            luaA_dofunction(globalconf.L, w->widget->mouse_enter, 1, 0);
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
    wibox_t *wibox = wibox_getbywin(ev->event);
    widget_node_t *w;

    if(wibox)
    {
        w = widget_getbycoords(wibox->position, wibox->widgets,
                               wibox->sw.geometry.width,
                               wibox->sw.geometry.height,
                               &ev->event_x, &ev->event_y);
        event_handle_widget_motionnotify(wibox, &wibox->mouse_over, w);
    }

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
    wibox_t *wibox = wibox_getbywin(ev->event);

    if(wibox && wibox->mouse_over)
    {
        /* call mouse leave function on widget the mouse was over */
        luaA_wibox_userdata_new(globalconf.L, wibox);
        luaA_dofunction(globalconf.L, wibox->mouse_over->widget->mouse_leave, 1, 0);
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
    xembed_window_t *emwin;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL
       || (ev->root_x == globalconf.pointer_x
           && ev->root_y == globalconf.pointer_y))
        return 0;

    if((c = client_getbytitlebarwin(ev->event))
       || (c = client_getbywin(ev->event)))
    {
        window_buttons_grab(c->win, ev->root, &c->buttons);
        /* The idea behind saving pointer_x and pointer_y is Bob Marley powered.
         * this will allow us top drop some EnterNotify events and thus not giving
         * focus to windows appering under the cursor without a cursor move */
        globalconf.pointer_x = ev->root_x;
        globalconf.pointer_y = ev->root_y;

        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.mouse_enter, 1, 0);
    }
    else if((emwin = xembed_getbywin(globalconf.embedded, ev->event)))
        xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY,
                          xutil_screen_get(connection, emwin->phys_screen)->root,
                          XCB_BUTTON_MASK_ANY);
    else
        window_root_buttons_grab(ev->root);

    return 0;
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
    wibox_t *wibox = wibox_getbywin(ev->window);

    if(wibox)
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
event_handle_keypress(void *data __attribute__ ((unused)),
                      xcb_connection_t *connection __attribute__ ((unused)),
                      xcb_key_press_event_t *ev)
{
    if(globalconf.keygrabber != LUA_REFNIL)
    {
        lua_rawgeti(globalconf.L, LUA_REGISTRYINDEX, globalconf.keygrabber);
        if(keygrabber_handlekpress(globalconf.L, ev))
        {
            if(lua_pcall(globalconf.L, 2, 1, 0))
            {
                warn("error running function: %s", lua_tostring(globalconf.L, -1));
                keygrabber_ungrab();
            }
            else if(!lua_isboolean(globalconf.L, -1) || !lua_toboolean(globalconf.L, -1))
                keygrabber_ungrab();
        }
        lua_pop(globalconf.L, 1);  /* pop returned value or function if not called */
    }
    else
    {
        keybinding_t *k = keybinding_find(ev);
        if (k && k->fct != LUA_REFNIL)
            luaA_dofunction(globalconf.L, k->fct, 0, 0);
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
    int phys_screen, screen = 0, ret = 0;
    client_t *c;
    xcb_get_window_attributes_cookie_t wa_c;
    xcb_get_window_attributes_reply_t *wa_r;
    xcb_query_pointer_cookie_t qp_c = { 0 };
    xcb_query_pointer_reply_t *qp_r = NULL;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;
    xcb_screen_iterator_t iter;

    wa_c = xcb_get_window_attributes_unchecked(connection, ev->window);

    if(!(wa_r = xcb_get_window_attributes_reply(connection, wa_c, NULL)))
        return -1;

    if(wa_r->override_redirect)
        goto bailout;

    if(xembed_getbywin(globalconf.embedded, ev->window))
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

        if(globalconf.xinerama_is_active)
            qp_c = xcb_query_pointer_unchecked(connection,
                                               xutil_screen_get(globalconf.connection,
                                                                globalconf.default_screen)->root);

        if(!(geom_r = xcb_get_geometry_reply(connection, geom_c, NULL)))
        {
            if(globalconf.xinerama_is_active)
                qp_r = xcb_query_pointer_reply(connection, qp_c, NULL);
            ret = -1;
            goto bailout;
        }


        for(iter = xcb_setup_roots_iterator(xcb_get_setup(connection)), phys_screen = 0;
            iter.rem && iter.data->root != geom_r->root; xcb_screen_next(&iter), ++phys_screen);

        if(globalconf.xinerama_is_active
           && (qp_r = xcb_query_pointer_reply(connection, qp_c, NULL)))
            screen = screen_getbycoord(screen, qp_r->root_x, qp_r->root_y);
        else
            screen = phys_screen;

        client_manage(ev->window, geom_r, phys_screen, screen);
        p_delete(&geom_r);
    }

bailout:
    p_delete(&qp_r);
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
    xembed_window_t *em;
    int i;

    if((c = client_getbywin(ev->window)))
    {
        if(ev->event == xutil_screen_get(connection, c->phys_screen)->root
           && XCB_EVENT_SENT(ev)
           && window_state_get_reply(window_state_get_unchecked(c->win)) == XCB_WM_STATE_NORMAL)
            client_unmanage(c);
    }
    else if((em = xembed_getbywin(globalconf.embedded, ev->window)))
    {
        xembed_window_list_detach(&globalconf.embedded, em);
        for(i = 0; i < globalconf.nscreen; i++)
            widget_invalidate_cache(i, WIDGET_CACHE_EMBEDDED);
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
    xcb_event_set_motion_notify_handler(&globalconf.evenths, event_handle_motionnotify, NULL);
    xcb_event_set_expose_handler(&globalconf.evenths, event_handle_expose, NULL);
    xcb_event_set_key_press_handler(&globalconf.evenths, event_handle_keypress, NULL);
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
