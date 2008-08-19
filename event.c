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
#include <xcb/xcb_atom.h>
#include <xcb/randr.h>
#include <xcb/xcb_icccm.h>

#include "event.h"
#include "tag.h"
#include "window.h"
#include "ewmh.h"
#include "client.h"
#include "widget.h"
#include "titlebar.h"
#include "keybinding.h"
#include "keygrabber.h"
#include "lua.h"
#include "systray.h"
#include "layouts/floating.h"
#include "common/atoms.h"

extern awesome_t globalconf;

/** Handle mouse button press.
 * \param c The client on which the event happened or NULL.
 * \param button Button number
 * \param state Modkeys state
 * \param buttons Buttons list to check for
 */
static void
event_handle_mouse_button_press(client_t *c,
                                unsigned int button,
                                unsigned int state,
                                button_t *buttons)
{
    button_t *b;

    for(b = buttons; b; b = b->next)
        if(button == b->button && XUTIL_MASK_CLEAN(state) == b->mod && b->fct)
        {
            if(c)
            {
                luaA_client_userdata_new(globalconf.L, c);
                luaA_dofunction(globalconf.L, b->fct, 1, 0);
            }
            else
                luaA_dofunction(globalconf.L, b->fct, 0, 0);
        }
}

/** The button press event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_buttonpress(void *data __attribute__ ((unused)),
                         xcb_connection_t *connection, xcb_button_press_event_t *ev)
{
    int screen, tmp;
    const int nb_screen = xcb_setup_roots_length(xcb_get_setup(connection));
    client_t *c;
    widget_node_t *w;
    statusbar_t *statusbar;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
            if(statusbar->sw
               && (statusbar->sw->window == ev->event || statusbar->sw->window == ev->child))
            {
                /* If the statusbar is child, then x,y are
                 * relative to root window */
                if(statusbar->sw->window == ev->child)
                {
                    ev->event_x -= statusbar->sw->geometry.x;
                    ev->event_y -= statusbar->sw->geometry.y;
                }
                /* Need to transform coordinates like it was
                 * top/bottom */
                switch(statusbar->position)
                {
                  case Right:
                    tmp = ev->event_y;
                    ev->event_y = statusbar->height - ev->event_x;
                    ev->event_x = tmp;
                    break;
                  case Left:
                    tmp = ev->event_y;
                    ev->event_y = ev->event_x;
                    ev->event_x = statusbar->width - tmp;
                    break;
                  default:
                    break;
                }
                for(w = statusbar->widgets; w; w = w->next)
                    if(ev->event_x >= w->area.x && ev->event_x < w->area.x + w->area.width
                       && ev->event_y >= w->area.y && ev->event_y < w->area.y + w->area.height)
                    {
                        w->widget->button_press(w, ev, statusbar->screen,
                                                statusbar, AWESOME_TYPE_STATUSBAR);
                        return 0;
                    }
                /* return if no widget match */
                return 0;
            }

    if((c = client_getbytitlebarwin(ev->event)))
    {
        /* Need to transform coordinates like it was
         * top/bottom */
        switch(c->titlebar->position)
        {
          case Right:
            tmp = ev->event_y;
            ev->event_y = c->titlebar->height - ev->event_x;
            ev->event_x = tmp;
            break;
          case Left:
            tmp = ev->event_y;
            ev->event_y = ev->event_x;
            ev->event_x = c->titlebar->width - tmp;
            break;
          default:
            break;
        }
        for(w = c->titlebar->widgets; w; w = w->next)
            if(ev->event_x >= w->area.x && ev->event_x < w->area.x + w->area.width
               && ev->event_y >= w->area.y && ev->event_y < w->area.y + w->area.height)
            {
                w->widget->button_press(w, ev, c->screen,
                                        c->titlebar, AWESOME_TYPE_TITLEBAR);
                return 0;
            }
        /* return if no widget match */
        return 0;
    }

    if((c = client_getbywin(ev->event)))
    {
        event_handle_mouse_button_press(c, ev->detail, ev->state, c->buttons);
        xcb_allow_events(globalconf.connection, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
    }
    else
        for(screen = 0; screen < nb_screen; screen++)
            if(xutil_screen_get(connection, screen)->root == ev->event)
            {
                event_handle_mouse_button_press(NULL, ev->detail, ev->state,
                                                globalconf.buttons.root);
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
            if(c->isfloating || layout_get_current(c->screen) == layout_floating)
            {
                client_resize(c, geometry, false);
                titlebar_draw(c);
            }
            else
            {
                globalconf.screens[c->screen].need_arrange = true;
                /* If we do not resize the client, at least tell it that it
                 * has its new configuration. That fixes at least
                 * gnome-terminal */
                window_configure(c->win, c->geometry, c->border);
            }
        }
        else
        {
            titlebar_update_geometry_floating(c);
            window_configure(c->win, geometry, c->border);
        }
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
            ewmh_restart();

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
        for(i = 0; i < globalconf.screens_info->nscreen; i++)
            widget_invalidate_cache(i, WIDGET_CACHE_EMBEDDED);
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
        window_buttons_grab(c->win, ev->root, c->buttons);
        /* The idea behind saving pointer_x and pointer_y is Bob Marley powered.
         * this will allow us top drop some EnterNotify events and thus not giving
         * focus to windows appering under the cursor without a cursor move */
        globalconf.pointer_x = ev->root_x;
        globalconf.pointer_y = ev->root_y;

        luaA_client_userdata_new(globalconf.L, c);
        luaA_dofunction(globalconf.L, globalconf.hooks.mouseover, 1, 0);
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
    int screen;
    statusbar_t *statusbar;
    client_t *c;

    if(!ev->count)
    {
        for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
            for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
                if(statusbar->sw
                   && statusbar->sw->window == ev->window)
                {
                    simplewindow_refresh_pixmap(statusbar->sw);
                    return 0;
                }

        if((c = client_getbytitlebarwin(ev->window)))
           simplewindow_refresh_pixmap(c->titlebar->sw);
    }

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
    int screen_nbr = 0, ret = 0;
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
        if(client_maybevisible(c, c->screen))
        {
            c->ishidden = false;
            globalconf.screens[c->screen].need_arrange = true;
            xcb_map_window(globalconf.connection, ev->window);
            /* it will be raised, so just update ourself */
            client_raise(c);
        }
    }
    else
    {
        geom_c = xcb_get_geometry_unchecked(connection, ev->window);

        if(globalconf.screens_info->xinerama_is_active)
            qp_c = xcb_query_pointer_unchecked(connection, xutil_screen_get(globalconf.connection,
                                                                            screen_nbr)->root);

        if(!(geom_r = xcb_get_geometry_reply(connection, geom_c, NULL)))
        {
            if(globalconf.screens_info->xinerama_is_active)
            {
                qp_r = xcb_query_pointer_reply(connection, qp_c, NULL);
                p_delete(&qp_r);
            }
            ret = -1;
            goto bailout;
        }

        if(globalconf.screens_info->xinerama_is_active
           && (qp_r = xcb_query_pointer_reply(connection, qp_c, NULL)))
        {
            screen_nbr = screen_get_bycoord(globalconf.screens_info, screen_nbr,
                                            qp_r->root_x, qp_r->root_y);
            p_delete(&qp_r);
        }
        else
            for(iter = xcb_setup_roots_iterator(xcb_get_setup(connection)), screen_nbr = 0;
                iter.rem && iter.data->root != geom_r->root; xcb_screen_next (&iter), ++screen_nbr);

        client_manage(ev->window, geom_r, screen_nbr);
        p_delete(&geom_r);
    }

bailout:
    p_delete(&wa_r);
    return ret;
}

/** The property notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
event_handle_propertynotify(void *data __attribute__ ((unused)),
                            xcb_connection_t *connection, xcb_property_notify_event_t *ev)
{
    client_t *c;
    xcb_window_t trans;
    xembed_window_t *emwin;

    if(ev->state == XCB_PROPERTY_DELETE)
        return 0; /* ignore */
    else if((emwin = xembed_getbywin(globalconf.embedded, ev->window)))
        xembed_property_update(connection, emwin);
    else if((c = client_getbywin(ev->window)))
    {
        if(ev->atom == WM_TRANSIENT_FOR)
        {
            xcb_get_wm_transient_for(connection, c->win, &trans);
            if(!c->isfloating
               && (c->isfloating = (client_getbywin(trans) != NULL)))
                globalconf.screens[c->screen].need_arrange = true;
        }
        else if (ev->atom == WM_NORMAL_HINTS)
            xcb_free_size_hints(client_updatesizehints(c));
        else if (ev->atom == WM_HINTS)
            client_updatewmhints(c);
        else if(ev->atom == WM_NAME || ev->atom == _NET_WM_NAME)
            client_updatetitle(c);
        else if(ev->atom == _NET_WM_ICON)
        {
            xcb_get_property_cookie_t icon_q = ewmh_window_icon_get_unchecked(c->win);
            netwm_icon_delete(&c->icon);
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
            c->icon = ewmh_window_icon_get_reply(icon_q);
        }
    }

    return 0;
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

    /* event->send_event (Xlib)  is quivalent to  (ev->response_type &
     * 0x80)  in XCB  because the  SendEvent bit  is available  in the
     * response_type field */
    bool send_event = ((ev->response_type & 0x80) >> 7);

    if((c = client_getbywin(ev->window)))
    {
        if(ev->event == xutil_screen_get(connection, c->phys_screen)->root
           && send_event
           && window_state_get_reply(window_state_get_unchecked(c->win)) == XCB_WM_NORMAL_STATE)
            client_unmanage(c);
    }
    else if((em = xembed_getbywin(globalconf.embedded, ev->window)))
    {
        xembed_window_list_detach(&globalconf.embedded, em);
        for(i = 0; i < globalconf.screens_info->nscreen; i++)
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

    ewmh_restart();

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
           && ev->data.data32[0] == XCB_WM_ICONIC_STATE)
        {
            c->ishidden = true;
            globalconf.screens[c->screen].need_arrange = true;
        }
    }
    else if(ev->type == _XEMBED)
        return xembed_process_client_message(ev);
    else if(ev->type == _NET_SYSTEM_TRAY_OPCODE)
        return systray_process_client_message(ev);
    return ewmh_process_client_message(ev);
}

void a_xcb_set_event_handlers(void)
{
    const xcb_query_extension_reply_t *randr_query;

    set_button_press_event_handler(globalconf.evenths, event_handle_buttonpress, NULL);
    set_configure_request_event_handler(globalconf.evenths, event_handle_configurerequest, NULL);
    set_configure_notify_event_handler(globalconf.evenths, event_handle_configurenotify, NULL);
    set_destroy_notify_event_handler(globalconf.evenths, event_handle_destroynotify, NULL);
    set_enter_notify_event_handler(globalconf.evenths, event_handle_enternotify, NULL);
    set_expose_event_handler(globalconf.evenths, event_handle_expose, NULL);
    set_key_press_event_handler(globalconf.evenths, event_handle_keypress, NULL);
    set_map_request_event_handler(globalconf.evenths, event_handle_maprequest, NULL);
    set_property_notify_event_handler(globalconf.evenths, event_handle_propertynotify, NULL);
    set_unmap_notify_event_handler(globalconf.evenths, event_handle_unmapnotify, NULL);
    set_client_message_event_handler(globalconf.evenths, event_handle_clientmessage, NULL);

    /* check for randr extension */
    randr_query = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
    if((globalconf.have_randr = randr_query->present))
        xcb_set_event_handler(globalconf.evenths,
                              (randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
