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
#include "objects/tag.h"
#include "xwindow.h"
#include "ewmh.h"
#include "objects/client.h"
#include "objects/widget.h"
#include "keyresolv.h"
#include "keygrabber.h"
#include "mousegrabber.h"
#include "luaa.h"
#include "systray.h"
#include "screen.h"
#include "common/atoms.h"
#include "common/xutil.h"

#define DO_EVENT_HOOK_CALLBACK(type, xcbtype, xcbeventprefix, arraytype, match) \
    static void \
    event_##xcbtype##_callback(xcb_##xcbtype##_press_event_t *ev, \
                               arraytype *arr, \
                               int oud, \
                               int nargs, \
                               void *data) \
    { \
        int abs_oud = oud < 0 ? ((lua_gettop(globalconf.L) + 1) + oud) : oud; \
        int item_matching = 0; \
        foreach(item, *arr) \
            if(match(ev, *item, data)) \
            { \
                if(oud) \
                    luaA_object_push_item(globalconf.L, abs_oud, *item); \
                else \
                    luaA_object_push(globalconf.L, *item); \
                item_matching++; \
            } \
        for(; item_matching > 0; item_matching--) \
        { \
            switch(ev->response_type) \
            { \
              case xcbeventprefix##_PRESS: \
                for(int i = 0; i < nargs; i++) \
                    lua_pushvalue(globalconf.L, - nargs - item_matching); \
                luaA_object_emit_signal(globalconf.L, - nargs - 1, "press", nargs); \
                break; \
              case xcbeventprefix##_RELEASE: \
                for(int i = 0; i < nargs; i++) \
                    lua_pushvalue(globalconf.L, - nargs - item_matching); \
                luaA_object_emit_signal(globalconf.L, - nargs - 1, "release", nargs); \
                break; \
            } \
            lua_pop(globalconf.L, 1); \
        } \
        lua_pop(globalconf.L, nargs); \
    }

static bool
event_key_match(xcb_key_press_event_t *ev, keyb_t *k, void *data)
{
    assert(data);
    xcb_keysym_t keysym = *(xcb_keysym_t *) data;
    return (((k->keycode && ev->detail == k->keycode)
             || (k->keysym && keysym == k->keysym))
            && (k->modifiers == XCB_BUTTON_MASK_ANY || k->modifiers == ev->state));
}

static bool
event_button_match(xcb_button_press_event_t *ev, button_t *b, void *data)
{
    return ((!b->button || ev->detail == b->button)
            && (b->modifiers == XCB_BUTTON_MASK_ANY || b->modifiers == ev->state));
}

DO_EVENT_HOOK_CALLBACK(button_t, button, XCB_BUTTON, button_array_t, event_button_match)
DO_EVENT_HOOK_CALLBACK(keyb_t, key, XCB_KEY, key_array_t, event_key_match)

/** Handle an event with mouse grabber if needed
 * \param x The x coordinate.
 * \param y The y coordinate.
 * \param mask The mask buttons.
 * \return True if the event was handled.
 */
static bool
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
        return true;
    }
    return false;
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
    wibox_t *wibox;

    if(event_handle_mousegrabber(ev->root_x, ev->root_y, 1 << (ev->detail - 1 + 8)))
        return 0;

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
        if(wibox->window == ev->child)
        {
            ev->event_x -= wibox->geometry.x;
            ev->event_y -= wibox->geometry.y;
        }

        /* Push the wibox */
        luaA_object_push(globalconf.L, wibox);
        /* Duplicate the wibox */
        lua_pushvalue(globalconf.L, -1);
        /* Handle the button event on it */
        event_button_callback(ev, &wibox->buttons, -1, 1, NULL);

        /* then try to match a widget binding */
        widget_t *w = widget_getbycoords(wibox->orientation, &wibox->widgets,
                                         wibox->geometry.width,
                                         wibox->geometry.height,
                                         &ev->event_x, &ev->event_y);
        if(w)
        {
            /* Push widget (the wibox is already on stack) */
            luaA_object_push_item(globalconf.L, -1, w);
            /* Move widget before wibox */
            lua_insert(globalconf.L, -2);
            event_button_callback(ev, &w->buttons, -2, 2, NULL);
        }
        else
            /* Remove the wibox, we did not use it */
            lua_pop(globalconf.L, 1);

    }
    else if((c = client_getbywin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        event_button_callback(ev, &c->buttons, -1, 1, NULL);
        xcb_allow_events(globalconf.connection,
                         XCB_ALLOW_REPLAY_POINTER,
                         XCB_CURRENT_TIME);
    }
    else if(ev->child == XCB_NONE)
        for(screen = 0; screen < nb_screen; screen++)
            if(xutil_screen_get(connection, screen)->root == ev->event)
            {
                event_button_callback(ev, &globalconf.buttons, 0, 0, NULL);
                return 0;
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

    if((c = client_getbywin(ev->window)))
    {
        area_t geometry = c->geometries.internal;

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
            geometry.x = ev->x;
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
            geometry.y = ev->y;
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            geometry.width = ev->width;
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            geometry.height = ev->height;

        if(ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
        {
            luaA_object_push(globalconf.L, c);
            client_set_border_width(globalconf.L, -1, ev->border_width);
            lua_pop(globalconf.L, 1);
        }

        /* Clients are not allowed to directly mess with stacking parameters. */
        ev->value_mask &= ~(XCB_CONFIG_WINDOW_SIBLING |
                                            XCB_CONFIG_WINDOW_STACK_MODE);

        /** Configure request are sent with size relative to real (internal)
         * window size, i.e. without borders. */
        geometry.width += 2 * c->border_width;
        geometry.height += 2 * c->border_width;

        if(!client_resize(c, geometry, false))
        {
            geometry.width -= 2 * c->border_width;
            geometry.height -= 2 * c->border_width;
            xwindow_configure(c->window, geometry, c->border_width);
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
                widget_invalidate_bytype(widget_systray);
            }

    return 0;
}

/** Handle a motion notify over widgets.
 * \param wibox The wibox.
 * \param mouse_over The pointer to the registered mouse over widget.
 * \param widget The new widget the mouse is over.
 */
static void
event_handle_widget_motionnotify(wibox_t *wibox,
                                 widget_t **mouse_over,
                                 widget_t *widget)
{
    if(widget != *mouse_over)
    {
        if(*mouse_over)
        {
            /* Emit mouse leave signal on old widget:
             * - Push wibox.*/
            luaA_object_push(globalconf.L, wibox);
            /* - Push the widget the mouse was on */
            luaA_object_push_item(globalconf.L, -1, *mouse_over);
            /* - Emit the signal mouse::leave with the wibox as argument */
            luaA_object_emit_signal(globalconf.L, -2, "mouse::leave", 1);
            /* - Remove the widget */
            lua_pop(globalconf.L, 1);

            /* Re-push wibox */
            luaA_object_push(globalconf.L, wibox);
            luaA_object_unref_item(globalconf.L, -1, *mouse_over);
            *mouse_over = NULL;
            /* Remove the wibox */
            lua_pop(globalconf.L, 1);
        }
        if(widget)
        {
            /* Get a ref on this widget so that it can't be unref'd from
             * underneath us (-> invalid pointer dereference):
             * - Push the wibox */
            luaA_object_push(globalconf.L, wibox);
            /* - Push the widget */
            luaA_object_push_item(globalconf.L, -1, widget);
            /* - Reference the widget into the wibox */
            luaA_object_ref_item(globalconf.L, -2, -1);

            /* Store that this widget was the one with the mouse over */
            *mouse_over = widget;

            /* Emit mouse::enter signal on new widget:
             * - Push the widget */
            luaA_object_push_item(globalconf.L, -1, widget);
            /* - Move the widget before the wibox we pushed just above */
            lua_insert(globalconf.L, -2);
            /* - Emit the signal with the wibox as argument */

            luaA_object_emit_signal(globalconf.L, -2, "mouse::enter", 1);
            /* - Remove the widget from the stack */
            lua_pop(globalconf.L, 1);
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

    if(event_handle_mousegrabber(ev->root_x, ev->root_y, ev->state))
        return 0;

    if((wibox = wibox_getbywin(ev->event)))
    {
        widget_t *w = widget_getbycoords(wibox->orientation, &wibox->widgets,
                                         wibox->geometry.width,
                                         wibox->geometry.height,
                                         &ev->event_x, &ev->event_y);
        if(w)
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
    wibox_t *wibox;
    client_t *c;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return 0;

    if((c = client_getbywin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "mouse::leave", 0);
        lua_pop(globalconf.L, 1);
    }

    if((wibox = wibox_getbywin(ev->event)))
    {
        if(wibox->mouse_over)
        {
            /* Push the wibox */
            luaA_object_push(globalconf.L, wibox);
            /* Push the widget the mouse is over in this wibox */
            luaA_object_push_item(globalconf.L, -1, wibox->mouse_over);
            /* Emit mouse::leave signal on widget the mouse was over with
             * its wibox as argument */
            luaA_object_emit_signal(globalconf.L, -2, "mouse::leave", 1);
            /* Remove the widget from the stack */
            lua_pop(globalconf.L, 1);
            wibox_clear_mouse_over(wibox);
        }

        luaA_object_push(globalconf.L, wibox);
        luaA_object_emit_signal(globalconf.L, -1, "mouse::leave", 0);
        lua_pop(globalconf.L, 1);
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
    wibox_t *wibox;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return 0;

    if((wibox = wibox_getbywin(ev->event)))
    {
        widget_t *w = widget_getbycoords(wibox->orientation, &wibox->widgets,
                                         wibox->geometry.width,
                                         wibox->geometry.height,
                                         &ev->event_x, &ev->event_y);
        if(w)
            event_handle_widget_motionnotify(wibox, &wibox->mouse_over, w);

        luaA_object_push(globalconf.L, wibox);
        luaA_object_emit_signal(globalconf.L, -1, "mouse::enter", 0);
        lua_pop(globalconf.L, 1);
    }

    if((c = client_getbywin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "mouse::enter", 0);
        lua_pop(globalconf.L, 1);
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
    /* Events that we are interested in: */
    switch(ev->detail)
    {
        /* These are events that jump between root windows.
         */
        case XCB_NOTIFY_DETAIL_ANCESTOR:
        case XCB_NOTIFY_DETAIL_INFERIOR:

        /* These are events that jump between clients.
         * Virtual events ensure we always get an event on our top-level window.
         */
        case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
        case XCB_NOTIFY_DETAIL_NONLINEAR:
          {
            client_t *c;

            if((c = client_getbywin(ev->event)))
                client_focus_update(c);
          }
        /* all other events are ignored */
        default:
            break;
    }
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
    wibox_t *wibox;

    /* If the wibox got need_update set, skip this because it will be repainted
     * soon anyway. Without this we could be painting garbage to the screen!
     */
    if((wibox = wibox_getbywin(ev->window)) && !wibox->need_update)
        wibox_refresh_pixmap_partial(wibox,
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
    else
    {
        /* get keysym ignoring all modifiers */
        xcb_keysym_t keysym = keyresolv_get_keysym(ev->detail, 0);
        client_t *c;
        if((c = client_getbywin(ev->event)))
        {
            luaA_object_push(globalconf.L, c);
            event_key_callback(ev, &c->keys, -1, 1, &keysym);
        }
        else
            event_key_callback(ev, &globalconf.keys, 0, 0, &keysym);
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
        if(client_maybevisible(c, c->screen) && !c->hidden)
        {
            luaA_object_push(globalconf.L, c);
            client_set_minimized(globalconf.L, -1, false);
            lua_pop(globalconf.L, 1);
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
           && XCB_EVENT_SENT(ev))
        {
            client_unmanage(c);
            xcb_unmap_window(connection, ev->window);
        }
    }
    else
        for(int i = 0; i < globalconf.embedded.len; i++)
            if(globalconf.embedded.tab[i].win == ev->window)
            {
                xembed_window_array_take(&globalconf.embedded, i);
                widget_invalidate_bytype(widget_systray);
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
    /* check for startup notification messages */
    if(sn_xcb_display_process_event(globalconf.sndisplay, (xcb_generic_event_t *) ev))
        return 0;

    if(ev->type == WM_CHANGE_STATE)
    {
        client_t *c;
        if((c = client_getbywin(ev->window))
           && ev->format == 32
           && ev->data.data32[0] == XCB_WM_STATE_ICONIC)
        {
            luaA_object_push(globalconf.L, c);
            client_set_minimized(globalconf.L, -1, true);
            lua_pop(globalconf.L, 1);
        }
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
    if(ev->request == XCB_MAPPING_MODIFIER
       || ev->request == XCB_MAPPING_KEYBOARD)
    {
        xcb_get_modifier_mapping_cookie_t xmapping_cookie =
            xcb_get_modifier_mapping_unchecked(globalconf.connection);

        /* Free and then allocate the key symbols */
        xcb_key_symbols_free(globalconf.keysyms);
        globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

        xutil_lock_mask_get(globalconf.connection, xmapping_cookie,
                            globalconf.keysyms, &globalconf.numlockmask,
                            &globalconf.shiftlockmask, &globalconf.capslockmask,
                            &globalconf.modeswitchmask);

        int nscreen = xcb_setup_roots_length(xcb_get_setup(connection));

        /* regrab everything */
        for(int phys_screen = 0; phys_screen < nscreen; phys_screen++)
        {
            xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);
            /* yes XCB_BUTTON_MASK_ANY is also for grab_key even if it's look weird */
            xcb_ungrab_key(connection, XCB_GRAB_ANY, s->root, XCB_BUTTON_MASK_ANY);
            xwindow_grabkeys(s->root, &globalconf.keys);
        }

        foreach(_c, globalconf.clients)
        {
            client_t *c = *_c;
            xcb_ungrab_key(connection, XCB_GRAB_ANY, c->window, XCB_BUTTON_MASK_ANY);
            xwindow_grabkeys(c->window, &c->keys);
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
    if(randr_query->present)
        xcb_event_set_handler(&globalconf.evenths,
                              randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY,
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
