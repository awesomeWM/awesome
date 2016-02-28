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
#include <xcb/shape.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

#include "awesome.h"
#include "event.h"
#include "property.h"
#include "objects/tag.h"
#include "objects/drawin.h"
#include "xwindow.h"
#include "ewmh.h"
#include "objects/client.h"
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

/** Emit a button signal.
 * The top of the lua stack has to be the object on which to emit the event.
 * \param ev The event to handle.
 */
static void
event_emit_button(xcb_button_press_event_t *ev)
{
    const char *name;
    switch(XCB_EVENT_RESPONSE_TYPE(ev))
    {
    case XCB_BUTTON_PRESS:
        name = "button::press";
        break;
    case XCB_BUTTON_RELEASE:
        name = "button::release";
        break;
    default:
        fatal("Invalid event type");
    }

    /* Push the event's info */
    lua_pushnumber(globalconf.L, ev->event_x);
    lua_pushnumber(globalconf.L, ev->event_y);
    lua_pushnumber(globalconf.L, ev->detail);
    luaA_pushmodifiers(globalconf.L, ev->state);
    /* And emit the signal */
    luaA_object_emit_signal(globalconf.L, -5, name, 4);
}

/** The button press event handler.
 * \param ev The event.
 */
static void
event_handle_button(xcb_button_press_event_t *ev)
{
    client_t *c;
    drawin_t *drawin;

    globalconf.timestamp = ev->time;

    if(event_handle_mousegrabber(ev->root_x, ev->root_y, 1 << (ev->detail - 1 + 8)))
        return;

    /* ev->state is
     * button status (8 bits) + modifiers status (8 bits)
     * we don't care for button status that we get, especially on release, so
     * drop them */
    ev->state &= 0x00ff;

    if((drawin = drawin_getbywin(ev->event))
       || (drawin = drawin_getbywin(ev->child)))
    {
        /* If the drawin is child, then x,y are
         * relative to root window */
        if(drawin->window == ev->child)
        {
            ev->event_x -= drawin->geometry.x + drawin->border_width;
            ev->event_y -= drawin->geometry.y + drawin->border_width;
        }

        /* Push the drawable */
        luaA_object_push(globalconf.L, drawin);
        luaA_object_push_item(globalconf.L, -1, drawin->drawable);
        /* and handle the button raw button event */
        event_emit_button(ev);
        lua_pop(globalconf.L, 1);
        /* check if any button object matches */
        event_button_callback(ev, &drawin->buttons, -1, 1, NULL);
        /* Either we are receiving this due to ButtonPress/Release on the root
         * window or because we grabbed the button on the window. In the later
         * case we have to call AllowEvents.
         * Use AsyncPointer instead of ReplayPointer so that the event is
         * "eaten" instead of being handled again on the root window.
         */
        if(ev->child == XCB_NONE)
            xcb_allow_events(globalconf.connection,
                             XCB_ALLOW_ASYNC_POINTER,
                             XCB_CURRENT_TIME);
    }
    else if((c = client_getbyframewin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        /* And handle the button raw button event */
        event_emit_button(ev);
        /* then check if a titlebar was "hit" */
        int x = ev->event_x, y = ev->event_y;
        drawable_t *d = client_get_drawable_offset(c, &x, &y);
        if (d)
        {
            /* Copy the event so that we can fake x/y */
            xcb_button_press_event_t event = *ev;
            event.event_x = x;
            event.event_y = y;
            luaA_object_push_item(globalconf.L, -1, d);
            event_emit_button(&event);
            lua_pop(globalconf.L, 1);
        }
        /* then check if any button objects match */
        event_button_callback(ev, &c->buttons, -1, 1, NULL);
        xcb_allow_events(globalconf.connection,
                         XCB_ALLOW_REPLAY_POINTER,
                         XCB_CURRENT_TIME);
    }
    else if(ev->child == XCB_NONE)
        if(globalconf.screen->root == ev->event)
        {
            event_button_callback(ev, &globalconf.buttons, 0, 0, NULL);
            return;
        }
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
 * \param ev The event.
 */
static void
event_handle_configurerequest(xcb_configure_request_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
    {
        area_t geometry = c->geometry;
        uint16_t bw = c->border_width;
        uint16_t tb_left = c->titlebar[CLIENT_TITLEBAR_LEFT].size;
        uint16_t tb_right = c->titlebar[CLIENT_TITLEBAR_RIGHT].size;
        uint16_t tb_top = c->titlebar[CLIENT_TITLEBAR_TOP].size;
        uint16_t tb_bottom = c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;
        uint16_t deco_left = bw + tb_left;
        uint16_t deco_right = bw + tb_right;
        uint16_t deco_top = bw + tb_top;
        uint16_t deco_bottom = bw + tb_bottom;
        int16_t diff_w = 0, diff_h = 0, diff_border = 0;

        if(ev->value_mask & XCB_CONFIG_WINDOW_X)
        {
            int16_t diff = 0;
            geometry.x = ev->x;
            xwindow_translate_for_gravity(c->size_hints.win_gravity, deco_left, 0, deco_right, 0, &diff, NULL);
            geometry.x += diff;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_Y)
        {
            int16_t diff = 0;
            geometry.y = ev->y;
            xwindow_translate_for_gravity(c->size_hints.win_gravity, 0, deco_top, 0, deco_bottom, NULL, &diff);
            geometry.y += diff;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        {
            uint16_t old_w = geometry.width;
            geometry.width = ev->width;
            /* The ConfigureRequest specifies the size of the client window, we want the frame */
            geometry.width += tb_left + tb_right;
            diff_w = geometry.width - old_w;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        {
            uint16_t old_h = geometry.height;
            geometry.height = ev->height;
            /* The ConfigureRequest specifies the size of the client window, we want the frame */
            geometry.height += tb_top + tb_bottom;
            diff_h = geometry.height - old_h;
        }
        if(ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)
        {
            diff_border = ev->border_width - bw;
            diff_h += diff_border;
            diff_w += diff_border;

            luaA_object_push(globalconf.L, c);
            window_set_border_width(globalconf.L, -1, ev->border_width);
            lua_pop(globalconf.L, 1);
        }

        /* If the client resizes without moving itself, apply window gravity */
        if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY)
        {
            int16_t diff_x = 0, diff_y = 0;
            xwindow_translate_for_gravity(c->size_hints.win_gravity, diff_border, diff_border, diff_w, diff_h, &diff_x, &diff_y);
            if(!(ev->value_mask & XCB_CONFIG_WINDOW_X))
                geometry.x += diff_x;
            if(!(ev->value_mask & XCB_CONFIG_WINDOW_Y))
                geometry.y += diff_y;
        }

        if(!client_resize(c, geometry, false))
            /* ICCCM 4.1.5 / 4.2.3, if nothing was changed, send an event saying so */
            client_send_configure(c);
    }
    else
        event_handle_configurerequest_configure_window(ev);
}

/** The configure notify event handler.
 * \param ev The event.
 */
static void
event_handle_configurenotify(xcb_configure_notify_event_t *ev)
{
    const xcb_screen_t *screen = globalconf.screen;

    if(ev->window == screen->root
       && (ev->width != screen->width_in_pixels
           || ev->height != screen->height_in_pixels))
        /* it's not that we panic, but restart */
        awesome_restart();
}

/** The destroy notify event handler.
 * \param ev The event.
 */
static void
event_handle_destroynotify(xcb_destroy_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
        client_unmanage(c, false);
    else
        for(int i = 0; i < globalconf.embedded.len; i++)
            if(globalconf.embedded.tab[i].win == ev->window)
            {
                xembed_window_array_take(&globalconf.embedded, i);
                luaA_systray_invalidate();
            }
}

/** Record that the given drawable contains the pointer.
 */
static void
event_drawable_under_mouse(lua_State *L, int ud)
{
    void *d;

    lua_pushvalue(L, ud);
    d = luaA_object_ref(L, -1);

    if (d == globalconf.drawable_under_mouse)
    {
        /* Nothing to do */
        luaA_object_unref(L, d);
        return;
    }

    if (globalconf.drawable_under_mouse != NULL)
    {
        /* Emit leave on previous drawable */
        luaA_object_push(L, globalconf.drawable_under_mouse);
        luaA_object_emit_signal(L, -1, "mouse::leave", 0);
        lua_pop(L, 1);

        /* Unref the previous drawable */
        luaA_object_unref(L, globalconf.drawable_under_mouse);
        globalconf.drawable_under_mouse = NULL;
    }
    if (d != NULL)
    {
        /* Reference the drawable for leave event later */
        globalconf.drawable_under_mouse = d;

        /* Emit enter */
        luaA_object_emit_signal(L, ud, "mouse::enter", 0);
    }
}

/** The motion notify event handler.
 * \param ev The event.
 */
static void
event_handle_motionnotify(xcb_motion_notify_event_t *ev)
{
    drawin_t *w;
    client_t *c;

    globalconf.timestamp = ev->time;

    if(event_handle_mousegrabber(ev->root_x, ev->root_y, ev->state))
        return;

    if((c = client_getbyframewin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        lua_pushnumber(globalconf.L, ev->event_x);
        lua_pushnumber(globalconf.L, ev->event_y);
        luaA_object_emit_signal(globalconf.L, -3, "mouse::move", 2);

        /* now check if a titlebar was "hit" */
        int x = ev->event_x, y = ev->event_y;
        drawable_t *d = client_get_drawable_offset(c, &x, &y);
        if (d)
        {
            luaA_object_push_item(globalconf.L, -1, d);
            event_drawable_under_mouse(globalconf.L, -1);
            lua_pushnumber(globalconf.L, x);
            lua_pushnumber(globalconf.L, y);
            luaA_object_emit_signal(globalconf.L, -3, "mouse::move", 2);
            lua_pop(globalconf.L, 1);
        }
        lua_pop(globalconf.L, 1);
    }

    if((w = drawin_getbywin(ev->event)))
    {
        luaA_object_push(globalconf.L, w);
        luaA_object_push_item(globalconf.L, -1, w->drawable);
        event_drawable_under_mouse(globalconf.L, -1);
        lua_pushnumber(globalconf.L, ev->event_x);
        lua_pushnumber(globalconf.L, ev->event_y);
        luaA_object_emit_signal(globalconf.L, -3, "mouse::move", 2);
        lua_pop(globalconf.L, 2);
    }
}

/** The leave notify event handler.
 * \param ev The event.
 */
static void
event_handle_leavenotify(xcb_leave_notify_event_t *ev)
{
    client_t *c;

    globalconf.timestamp = ev->time;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return;

    /* Ignore leave with detail inferior (we were left for a window contained in
     * our window, so technically the pointer is still inside of this window).
     */
    if(ev->detail != XCB_NOTIFY_DETAIL_INFERIOR && (c = client_getbyframewin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "mouse::leave", 0);
        lua_pop(globalconf.L, 1);
    }

    lua_pushnil(globalconf.L);
    event_drawable_under_mouse(globalconf.L, -1);
    lua_pop(globalconf.L, 1);
}

/** The enter notify event handler.
 * \param ev The event.
 */
static void
event_handle_enternotify(xcb_enter_notify_event_t *ev)
{
    client_t *c;
    drawin_t *drawin;

    globalconf.timestamp = ev->time;

    if(ev->mode != XCB_NOTIFY_MODE_NORMAL)
        return;

    if((drawin = drawin_getbywin(ev->event)))
    {
        luaA_object_push(globalconf.L, drawin);
        luaA_object_push_item(globalconf.L, -1, drawin->drawable);
        event_drawable_under_mouse(globalconf.L, -1);
        lua_pop(globalconf.L, 2);
    }

    if((c = client_getbyframewin(ev->event)))
    {
        luaA_object_push(globalconf.L, c);
        /* Ignore enter with detail inferior: The pointer was previously inside
         * of a child window, so technically this isn't a 'real' enter.
         */
        if (ev->detail != XCB_NOTIFY_DETAIL_INFERIOR)
            luaA_object_emit_signal(globalconf.L, -1, "mouse::enter", 0);

        drawable_t *d = client_get_drawable(c, ev->event_x, ev->event_y);
        if (d)
        {
            luaA_object_push_item(globalconf.L, -1, d);
            event_drawable_under_mouse(globalconf.L, -1);
            lua_pop(globalconf.L, 1);
        }
        lua_pop(globalconf.L, 1);
    }
    else if (ev->event == globalconf.screen->root) {
        /* When there are multiple X screens with awesome running separate
         * instances, reset focus.
         */
        globalconf.focus.need_update = true;
    }
}

/** The focus in event handler.
 * \param ev The event.
 */
static void
event_handle_focusin(xcb_focus_in_event_t *ev)
{
    if (ev->mode == XCB_NOTIFY_MODE_GRAB
            || ev->mode == XCB_NOTIFY_MODE_UNGRAB)
        /* Ignore focus changes due to keyboard grabs */
        return;

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

            if((c = client_getbywin(ev->event))) {
                /* If there is still a pending focus change, do it now. */
                client_focus_refresh();
                client_focus_update(c);
            }
          }
        /* all other events are ignored */
        default:
            break;
    }
}

/** The expose event handler.
 * \param ev The event.
 */
static void
event_handle_expose(xcb_expose_event_t *ev)
{
    drawin_t *drawin;
    client_t *client;

    if((drawin = drawin_getbywin(ev->window)))
        drawin_refresh_pixmap_partial(drawin,
                                      ev->x, ev->y,
                                      ev->width, ev->height);
    if ((client = client_getbyframewin(ev->window)))
        client_refresh_partial(client, ev->x, ev->y, ev->width, ev->height);
}

/** The key press event handler.
 * \param ev The event.
 */
static void
event_handle_key(xcb_key_press_event_t *ev)
{
    globalconf.timestamp = ev->time;

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
        }
        lua_pop(globalconf.L, 1);  /* pop returned value or function if not called */
    }
    else
    {
        /* get keysym ignoring all modifiers */
        xcb_keysym_t keysym = keyresolv_get_keysym(ev->detail, 0);
        client_t *c;
        if((c = client_getbywin(ev->event)) || (c = client_getbynofocuswin(ev->event)))
        {
            luaA_object_push(globalconf.L, c);
            event_key_callback(ev, &c->keys, -1, 1, &keysym);
        }
        else
            event_key_callback(ev, &globalconf.keys, 0, 0, &keysym);
    }
}

/** The map request event handler.
 * \param ev The event.
 */
static void
event_handle_maprequest(xcb_map_request_event_t *ev)
{
    client_t *c;
    xcb_get_window_attributes_cookie_t wa_c;
    xcb_get_window_attributes_reply_t *wa_r;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;

    wa_c = xcb_get_window_attributes_unchecked(globalconf.connection, ev->window);

    if(!(wa_r = xcb_get_window_attributes_reply(globalconf.connection, wa_c, NULL)))
        return;

    if(wa_r->override_redirect)
        goto bailout;

    if(xembed_getbywin(&globalconf.embedded, ev->window))
    {
        xcb_map_window(globalconf.connection, ev->window);
        xembed_window_activate(globalconf.connection, ev->window);
    }
    else if((c = client_getbywin(ev->window)))
    {
        /* Check that it may be visible, but not asked to be hidden */
        if(client_maybevisible(c) && !c->hidden)
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
        geom_c = xcb_get_geometry_unchecked(globalconf.connection, ev->window);

        if(!(geom_r = xcb_get_geometry_reply(globalconf.connection, geom_c, NULL)))
        {
            goto bailout;
        }

        client_manage(ev->window, geom_r, false);

        p_delete(&geom_r);
    }

bailout:
    p_delete(&wa_r);
}

/** The unmap notify event handler.
 * \param ev The event.
 */
static void
event_handle_unmapnotify(xcb_unmap_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)))
        client_unmanage(c, true);
    else
        for(int i = 0; i < globalconf.embedded.len; i++)
            if(globalconf.embedded.tab[i].win == ev->window)
            {
                xembed_window_array_take(&globalconf.embedded, i);
                xcb_change_save_set(globalconf.connection, XCB_SET_MODE_DELETE, ev->window);
                luaA_systray_invalidate();
            }
}

/** The randr screen change notify event handler.
 * \param ev The event.
 */
static void
event_handle_randr_screen_change_notify(xcb_randr_screen_change_notify_event_t *ev)
{
    /* Code  of  XRRUpdateConfiguration Xlib  function  ported to  XCB
     * (only the code relevant  to RRScreenChangeNotify) as the latter
     * doesn't provide this kind of function */
    if(ev->rotation & (XCB_RANDR_ROTATION_ROTATE_90 | XCB_RANDR_ROTATION_ROTATE_270))
        xcb_randr_set_screen_size(globalconf.connection, ev->root, ev->height, ev->width,
                                  ev->mheight, ev->mwidth);
    else
        xcb_randr_set_screen_size(globalconf.connection, ev->root, ev->width, ev->height,
                                  ev->mwidth, ev->mheight);

    /* XRRUpdateConfiguration also executes the following instruction
     * but it's not useful because SubpixelOrder is not used at all at
     * the moment
     *
     * XRenderSetSubpixelOrder(dpy, snum, scevent->subpixel_order);
     */

    awesome_restart();
}

/** The shape notify event handler.
 * \param ev The event.
 */
static void
event_handle_shape_notify(xcb_shape_notify_event_t *ev)
{
    client_t *c = client_getbywin(ev->affected_window);
    if (c)
    {
        luaA_object_push(globalconf.L, c);
        if (ev->shape_kind == XCB_SHAPE_SK_BOUNDING)
            luaA_object_emit_signal(globalconf.L, -1, "property::shape_client_bounding", 0);
        if (ev->shape_kind == XCB_SHAPE_SK_CLIP)
            luaA_object_emit_signal(globalconf.L, -1, "property::shape_client_clip", 0);
        lua_pop(globalconf.L, 1);
    }
}

/** The client message event handler.
 * \param ev The event.
 */
static void
event_handle_clientmessage(xcb_client_message_event_t *ev)
{
    /* check for startup notification messages */
    if(sn_xcb_display_process_event(globalconf.sndisplay, (xcb_generic_event_t *) ev))
        return;

    if(ev->type == WM_CHANGE_STATE)
    {
        client_t *c;
        if((c = client_getbywin(ev->window))
           && ev->format == 32
           && ev->data.data32[0] == XCB_ICCCM_WM_STATE_ICONIC)
        {
            luaA_object_push(globalconf.L, c);
            client_set_minimized(globalconf.L, -1, true);
            lua_pop(globalconf.L, 1);
        }
    }
    else if(ev->type == _XEMBED)
        xembed_process_client_message(ev);
    else if(ev->type == _NET_SYSTEM_TRAY_OPCODE)
        systray_process_client_message(ev);
    else
        ewmh_process_client_message(ev);
}

/** The keymap change notify event handler.
 * \param ev The event.
 */
static void
event_handle_mappingnotify(xcb_mapping_notify_event_t *ev)
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

        /* regrab everything */
        xcb_screen_t *s = globalconf.screen;
        /* yes XCB_BUTTON_MASK_ANY is also for grab_key even if it's look weird */
        xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, s->root, XCB_BUTTON_MASK_ANY);
        xwindow_grabkeys(s->root, &globalconf.keys);

        foreach(_c, globalconf.clients)
        {
            client_t *c = *_c;
            xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, c->window, XCB_BUTTON_MASK_ANY);
            xwindow_grabkeys(c->window, &c->keys);
            if (c->nofocus_window)
                xwindow_grabkeys(c->nofocus_window, &c->keys);
        }
    }
}

static void
event_handle_reparentnotify(xcb_reparent_notify_event_t *ev)
{
    client_t *c;

    if((c = client_getbywin(ev->window)) && c->frame_window != ev->parent)
    {
        /* Ignore reparents to the root window, they *might* be caused by
         * ourselves if a client quickly unmaps and maps itself again. */
        if (ev->parent != globalconf.screen->root)
            client_unmanage(c, true);
    }
}

/** \brief awesome xerror function.
 * There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).
 * \param e The error event.
 */
static void
xerror(xcb_generic_error_t *e)
{
    /* ignore this */
    if(e->error_code == XCB_WINDOW
       || (e->error_code == XCB_MATCH
           && e->major_code == XCB_SET_INPUT_FOCUS)
       || (e->error_code == XCB_VALUE
           && e->major_code == XCB_KILL_CLIENT)
       || (e->error_code == XCB_MATCH
           && e->major_code == XCB_CONFIGURE_WINDOW))
        return;

    warn("X error: request=%s (major %d, minor %d), error=%s (%d)",
         xcb_event_get_request_label(e->major_code),
         e->major_code, e->minor_code,
         xcb_event_get_error_label(e->error_code),
         e->error_code);

    return;
}

void event_handle(xcb_generic_event_t *event)
{
    uint8_t response_type = XCB_EVENT_RESPONSE_TYPE(event);

    if(response_type == 0)
    {
        /* This is an error, not a event */
        xerror((xcb_generic_error_t *) event);
        return;
    }

    switch(response_type)
    {
#define EVENT(type, callback) case type: callback((void *) event); return
        EVENT(XCB_BUTTON_PRESS, event_handle_button);
        EVENT(XCB_BUTTON_RELEASE, event_handle_button);
        EVENT(XCB_CONFIGURE_REQUEST, event_handle_configurerequest);
        EVENT(XCB_CONFIGURE_NOTIFY, event_handle_configurenotify);
        EVENT(XCB_DESTROY_NOTIFY, event_handle_destroynotify);
        EVENT(XCB_ENTER_NOTIFY, event_handle_enternotify);
        EVENT(XCB_CLIENT_MESSAGE, event_handle_clientmessage);
        EVENT(XCB_EXPOSE, event_handle_expose);
        EVENT(XCB_FOCUS_IN, event_handle_focusin);
        EVENT(XCB_KEY_PRESS, event_handle_key);
        EVENT(XCB_KEY_RELEASE, event_handle_key);
        EVENT(XCB_LEAVE_NOTIFY, event_handle_leavenotify);
        EVENT(XCB_MAPPING_NOTIFY, event_handle_mappingnotify);
        EVENT(XCB_MAP_REQUEST, event_handle_maprequest);
        EVENT(XCB_MOTION_NOTIFY, event_handle_motionnotify);
        EVENT(XCB_PROPERTY_NOTIFY, property_handle_propertynotify);
        EVENT(XCB_REPARENT_NOTIFY, event_handle_reparentnotify);
        EVENT(XCB_UNMAP_NOTIFY, event_handle_unmapnotify);
#undef EVENT
    }

    static uint8_t randr_screen_change_notify = 0;
    static uint8_t shape_notify = 0;

    if(randr_screen_change_notify == 0)
    {
        /* check for randr extension */
        const xcb_query_extension_reply_t *randr_query;
        randr_query = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
        if(randr_query->present)
            randr_screen_change_notify = randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY;
    }
    if(shape_notify == 0)
    {
        /* check for shape extension */
        const xcb_query_extension_reply_t *shape_query;
        shape_query = xcb_get_extension_data(globalconf.connection, &xcb_shape_id);
        if(shape_query->present)
            shape_notify = shape_query->first_event + XCB_SHAPE_NOTIFY;
    }

    if (response_type == randr_screen_change_notify)
        event_handle_randr_screen_change_notify((void *) event);
    if (response_type == shape_notify)
        event_handle_shape_notify((void *) event);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
