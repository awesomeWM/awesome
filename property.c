/*
 * property.c - property handlers
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include "screen.h"
#include "property.h"
#include "objects/client.h"
#include "ewmh.h"
#include "objects/wibox.h"
#include "xwindow.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/xutil.h"


#define HANDLE_TEXT_PROPERTY(funcname, atom, setfunc) \
    void \
    property_update_##funcname(client_t *c, xcb_get_property_reply_t *reply) \
    { \
        bool no_reply = !reply; \
        if(no_reply) \
            reply = xcb_get_property_reply(globalconf.connection, \
                                           xcb_get_property(globalconf.connection, \
                                                            false, \
                                                            c->window, \
                                                            atom, \
                                                            XCB_GET_PROPERTY_TYPE_ANY, \
                                                            0, \
                                                            UINT_MAX), NULL); \
        luaA_object_push(globalconf.L, c); \
        setfunc(globalconf.L, -1, xutil_get_text_property_from_reply(reply)); \
        lua_pop(globalconf.L, 1); \
        if(no_reply) \
            p_delete(&reply); \
    } \
    static int \
    property_handle_##funcname(uint8_t state, \
                               xcb_window_t window, \
                               xcb_atom_t name, \
                               xcb_get_property_reply_t *reply) \
    { \
        client_t *c = client_getbywin(window); \
        if(c) \
            property_update_##funcname(c, reply); \
        return 0; \
    }


HANDLE_TEXT_PROPERTY(wm_name, WM_NAME, client_set_alt_name)
HANDLE_TEXT_PROPERTY(net_wm_name, _NET_WM_NAME, client_set_name)
HANDLE_TEXT_PROPERTY(wm_icon_name, WM_ICON_NAME, client_set_alt_icon_name)
HANDLE_TEXT_PROPERTY(net_wm_icon_name, _NET_WM_ICON_NAME, client_set_icon_name)
HANDLE_TEXT_PROPERTY(wm_client_machine, WM_CLIENT_MACHINE, client_set_machine)
HANDLE_TEXT_PROPERTY(wm_window_role, WM_WINDOW_ROLE, client_set_role)

#undef HANDLE_TEXT_PROPERTY

#define HANDLE_PROPERTY(name) \
    static int \
    property_handle_##name(uint8_t state, \
                           xcb_window_t window, \
                           xcb_atom_t name, \
                           xcb_get_property_reply_t *reply) \
    { \
        client_t *c = client_getbywin(window); \
        if(c) \
            property_update_##name(c, reply); \
        return 0; \
    }

HANDLE_PROPERTY(wm_protocols)
HANDLE_PROPERTY(wm_transient_for)
HANDLE_PROPERTY(wm_client_leader)
HANDLE_PROPERTY(wm_normal_hints)
HANDLE_PROPERTY(wm_hints)
HANDLE_PROPERTY(wm_class)
HANDLE_PROPERTY(net_wm_icon)
HANDLE_PROPERTY(net_wm_pid)

#undef HANDLE_PROPERTY

void
property_update_wm_transient_for(client_t *c, xcb_get_property_reply_t *reply)
{
    xcb_window_t trans;

    if(reply)
    {
        if(!xcb_get_wm_transient_for_from_reply(&trans, reply))
            return;
    }
    else
    {
        if(!xcb_get_wm_transient_for_reply(globalconf.connection,
                                            xcb_get_wm_transient_for_unchecked(globalconf.connection,
                                                                               c->window),
                                            &trans, NULL))
            return;
    }

    luaA_object_push(globalconf.L, c);
    client_set_type(globalconf.L, -1, WINDOW_TYPE_DIALOG);
    client_set_above(globalconf.L, -1, false);
    client_set_transient_for(globalconf.L, -1, client_getbywin(trans));
    lua_pop(globalconf.L, 1);
}

/** Update leader hint of a client.
 * \param c The client.
 * \param reply (Optional) An existing reply.
 */
void
property_update_wm_client_leader(client_t *c, xcb_get_property_reply_t *reply)
{
    xcb_get_property_cookie_t client_leader_q;
    void *data;
    bool no_reply = !reply;

    if(no_reply)
    {
        client_leader_q = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                                     WM_CLIENT_LEADER, WINDOW, 0, 32);

        reply = xcb_get_property_reply(globalconf.connection, client_leader_q, NULL);
    }

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        c->leader_window = *(xcb_window_t *) data;

    /* Only free when we created a reply ourselves. */
    if(no_reply)
        p_delete(&reply);
}

/** Update the size hints of a client.
 * \param c The client.
 * \param reply (Optional) An existing reply.
 */
void
property_update_wm_normal_hints(client_t *c, xcb_get_property_reply_t *reply)
{
    if(reply)
    {
        if(!xcb_get_wm_size_hints_from_reply(&c->size_hints, reply))
            return;
    }
    else
    {
        if(!xcb_get_wm_normal_hints_reply(globalconf.connection,
                                          xcb_get_wm_normal_hints_unchecked(globalconf.connection,
                                                                            c->window),
                                          &c->size_hints, NULL))
            return;
    }
}

/** Update the WM hints of a client.
 * \param c The client.
 * \param reply (Optional) An existing reply.
 */
void
property_update_wm_hints(client_t *c, xcb_get_property_reply_t *reply)
{
    xcb_wm_hints_t wmh;

    if(reply)
    {
        if(!xcb_get_wm_hints_from_reply(&wmh, reply))
            return;
    }
    else
    {
        if(!xcb_get_wm_hints_reply(globalconf.connection,
                                  xcb_get_wm_hints_unchecked(globalconf.connection, c->window),
                                  &wmh, NULL))
            return;
    }

    luaA_object_push(globalconf.L, c);
    client_set_urgent(globalconf.L, -1, xcb_wm_hints_get_urgency(&wmh));

    if(wmh.flags & XCB_WM_HINT_INPUT)
        c->nofocus = !wmh.input;

    if(wmh.flags & XCB_WM_HINT_WINDOW_GROUP)
        client_set_group_window(globalconf.L, -1, wmh.window_group);

    lua_pop(globalconf.L, 1);
}

/** Update WM_CLASS of a client.
 * \param c The client.
 * \param reply The reply to get property request, or NULL if none.
 */
void
property_update_wm_class(client_t *c, xcb_get_property_reply_t *reply)
{
    xcb_get_wm_class_reply_t hint;

    if(reply)
    {
        if(!xcb_get_wm_class_from_reply(&hint, reply))
            return;
    }
    else
    {
        if(!xcb_get_wm_class_reply(globalconf.connection,
                                   xcb_get_wm_class_unchecked(globalconf.connection, c->window),
                                   &hint, NULL))
            return;
    }

    luaA_object_push(globalconf.L, c);
    client_set_class_instance(globalconf.L, -1, hint.class_name, hint.instance_name);
    lua_pop(globalconf.L, 1);

    /* only delete reply if we get it ourselves */
    if(!reply)
        xcb_get_wm_class_reply_wipe(&hint);
}

static int
property_handle_net_wm_strut_partial(uint8_t state,
                                     xcb_window_t window,
                                     xcb_atom_t name,
                                     xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        ewmh_process_client_strut(c, reply);

    return 0;
}

void
property_update_net_wm_icon(client_t *c,
                            xcb_get_property_reply_t *reply)
{
    luaA_object_push(globalconf.L, c);

    if(reply)
    {
        if(ewmh_window_icon_from_reply(reply))
            client_set_icon(globalconf.L, -2, -1);
    }
    else if(ewmh_window_icon_get_reply(ewmh_window_icon_get_unchecked(c->window)))
        client_set_icon(globalconf.L, -2, -1);

    /* remove client */
    lua_pop(globalconf.L, 1);
}

void
property_update_net_wm_pid(client_t *c,
                           xcb_get_property_reply_t *reply)
{
    bool no_reply = !reply;

    if(no_reply)
    {
        xcb_get_property_cookie_t prop_c =
            xcb_get_property_unchecked(globalconf.connection, false, c->window, _NET_WM_PID, CARDINAL, 0L, 1L);
        reply = xcb_get_property_reply(globalconf.connection, prop_c, NULL);
    }

    if(reply && reply->value_len)
    {
        uint32_t *rdata = xcb_get_property_value(reply);
        if(rdata)
        {
            luaA_object_push(globalconf.L, c);
            client_set_pid(globalconf.L, -1, *rdata);
            lua_pop(globalconf.L, 1);
        }
    }

    if(no_reply)
        p_delete(&reply);
}

/** Update the list of supported protocols for a client.
 * \param c The client.
 * \param reply The xcb property reply.
 */
void
property_update_wm_protocols(client_t *c, xcb_get_property_reply_t *reply)
{
    xcb_get_wm_protocols_reply_t protocols;
    xcb_get_property_reply_t *reply_copy;

    if(reply)
    {
        reply_copy = p_dup(reply, 1);

        if(!xcb_get_wm_protocols_from_reply(reply_copy, &protocols))
	{
            p_delete(&reply_copy);
            return;
	}
    }
    else
    {
        /* If this fails for any reason, we still got the old value */
        if(!xcb_get_wm_protocols_reply(globalconf.connection,
                                      xcb_get_wm_protocols_unchecked(globalconf.connection,
                                                                     c->window, WM_PROTOCOLS),
                                      &protocols, NULL))
            return;
    }

    xcb_get_wm_protocols_reply_wipe(&c->protocols);
    memcpy(&c->protocols, &protocols, sizeof(protocols));
}

/** The property notify event handler.
 * \param state currently unused
 * \param window The window to obtain update the property with.
 * \param name The protocol atom, currently unused.
 * \param reply (Optional) An existing reply.
 */
static int
property_handle_xembed_info(uint8_t state,
                            xcb_window_t window,
                            xcb_atom_t name,
                            xcb_get_property_reply_t *reply)
{
    xembed_window_t *emwin = xembed_getbywin(&globalconf.embedded, window);

    if(emwin)
        xembed_property_update(globalconf.connection, emwin, reply);

    return 0;
}

static int
property_handle_xrootpmap_id(uint8_t state,
                             xcb_window_t window,
                             xcb_atom_t name,
                             xcb_get_property_reply_t *reply)
{
    if(globalconf.xinerama_is_active)
        foreach(w, globalconf.wiboxes)
            (*w)->need_update = true;
    else
    {
        int screen = xutil_root2screen(globalconf.connection, window);
        foreach(w, globalconf.wiboxes)
            if(screen == screen_array_indexof(&globalconf.screens, (*w)->screen))
               (*w)->need_update = true;
    }

    return 0;
}

static int
property_handle_net_wm_opacity(uint8_t state,
                               xcb_window_t window,
                               xcb_atom_t name,
                               xcb_get_property_reply_t *reply)
{
    wibox_t *wibox = wibox_getbywin(window);

    if(wibox)
    {
        luaA_object_push(globalconf.L, wibox);
        window_set_opacity(globalconf.L, -1, xwindow_get_opacity_from_reply(reply));
        lua_pop(globalconf.L, -1);
    }
    else
    {
        client_t *c = client_getbywin(window);
        if(c)
        {
            luaA_object_push(globalconf.L, c);
            window_set_opacity(globalconf.L, -1, xwindow_get_opacity_from_reply(reply));
            lua_pop(globalconf.L, 1);
        }
    }

    return 0;
}

/** The property notify event handler.
 * \param data Unused data.
 * \param connection The connection to the X server.
 * \param ev The event.
 * \return Status code, 0 if everything's fine.
 */
void
property_handle_propertynotify(xcb_property_notify_event_t *ev)
{
    uint32_t length;
    int (*handler)(uint8_t state,
                   xcb_window_t window,
                   xcb_atom_t name,
                   xcb_get_property_reply_t *reply) = NULL;

    /* Find the correct event handler */
#define HANDLE(atom_, cb, len) \
    if (ev->atom == atom_) \
    { \
        handler = cb; \
        length = len; \
    } else
#define HANDLE_L(atom, cb) HANDLE(atom, cb, UINT_MAX)
#define HANDLE_S(atom, cb) HANDLE(atom, cb, 1)
#define END return

    /* Xembed stuff */
    HANDLE_L(_XEMBED_INFO, property_handle_xembed_info)

    /* ICCCM stuff */
    HANDLE_L(WM_TRANSIENT_FOR, property_handle_wm_transient_for)
    HANDLE_L(WM_CLIENT_LEADER, property_handle_wm_client_leader)
    HANDLE_L(WM_NORMAL_HINTS, property_handle_wm_normal_hints)
    HANDLE_L(WM_HINTS, property_handle_wm_hints)
    HANDLE_L(WM_NAME, property_handle_wm_name)
    HANDLE_L(WM_ICON_NAME, property_handle_wm_icon_name)
    HANDLE_L(WM_CLASS, property_handle_wm_class)
    HANDLE_L(WM_PROTOCOLS, property_handle_wm_protocols)
    HANDLE_L(WM_CLIENT_MACHINE, property_handle_wm_client_machine)
    HANDLE_L(WM_WINDOW_ROLE, property_handle_wm_window_role)

    /* EWMH stuff */
    HANDLE_L(_NET_WM_NAME, property_handle_net_wm_name)
    HANDLE_L(_NET_WM_ICON_NAME, property_handle_net_wm_icon_name)
    HANDLE_L(_NET_WM_STRUT_PARTIAL, property_handle_net_wm_strut_partial)
    HANDLE_L(_NET_WM_ICON, property_handle_net_wm_icon)
    HANDLE_L(_NET_WM_PID, property_handle_net_wm_pid)
    HANDLE_S(_NET_WM_WINDOW_OPACITY, property_handle_net_wm_opacity)

    /* background change */
    HANDLE_S(_XROOTPMAP_ID, property_handle_xrootpmap_id)

    /* If nothing was found, return */
    END;

#undef HANDLE_L
#undef HANDLE_S
#undef END

    /* Get the property, if needed. */
    xcb_get_property_reply_t *propr = NULL;
    if(ev->state != XCB_PROPERTY_DELETE)
    {
        xcb_get_property_cookie_t cookie =
            xcb_get_property(globalconf.connection, 0, ev->window, ev->atom,
                             XCB_GET_PROPERTY_TYPE_ANY, 0, length);
        propr = xcb_get_property_reply(globalconf.connection, cookie, 0);
    }

    (*handler)(ev->state, ev->window, ev->atom, propr);
    p_delete(&propr);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
