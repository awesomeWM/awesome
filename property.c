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
#include "objects/drawin.h"
#include "xwindow.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/xutil.h"

#define HANDLE_TEXT_PROPERTY(funcname, atom, setfunc) \
    xcb_get_property_cookie_t \
    property_get_##funcname(client_t *c) \
    { \
        return xcb_get_property(globalconf.connection, \
                                false, \
                                c->window, \
                                atom, \
                                XCB_GET_PROPERTY_TYPE_ANY, \
                                0, \
                                UINT_MAX); \
    } \
    void \
    property_update_##funcname(client_t *c, xcb_get_property_cookie_t cookie) \
    { \
        xcb_get_property_reply_t * reply = \
                    xcb_get_property_reply(globalconf.connection, cookie, NULL); \
        luaA_object_push(globalconf.L, c); \
        setfunc(globalconf.L, -1, xutil_get_text_property_from_reply(reply)); \
        lua_pop(globalconf.L, 1); \
        p_delete(&reply); \
    } \
    static int \
    property_handle_##funcname(uint8_t state, \
                               xcb_window_t window) \
    { \
        client_t *c = client_getbywin(window); \
        if(c) \
            property_update_##funcname(c, property_get_##funcname(c));\
        return 0; \
    }


HANDLE_TEXT_PROPERTY(wm_name, XCB_ATOM_WM_NAME, client_set_alt_name)
HANDLE_TEXT_PROPERTY(net_wm_name, _NET_WM_NAME, client_set_name)
HANDLE_TEXT_PROPERTY(wm_icon_name, XCB_ATOM_WM_ICON_NAME, client_set_alt_icon_name)
HANDLE_TEXT_PROPERTY(net_wm_icon_name, _NET_WM_ICON_NAME, client_set_icon_name)
HANDLE_TEXT_PROPERTY(wm_client_machine, XCB_ATOM_WM_CLIENT_MACHINE, client_set_machine)
HANDLE_TEXT_PROPERTY(wm_window_role, WM_WINDOW_ROLE, client_set_role)

#undef HANDLE_TEXT_PROPERTY

#define HANDLE_PROPERTY(name) \
    static int \
    property_handle_##name(uint8_t state, \
                           xcb_window_t window) \
    { \
        client_t *c = client_getbywin(window); \
        if(c) \
            property_update_##name(c, property_get_##name(c));\
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

xcb_get_property_cookie_t
property_get_wm_transient_for(client_t *c)
{
    return xcb_icccm_get_wm_transient_for_unchecked(globalconf.connection, c->window);
}

void
property_update_wm_transient_for(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_window_t trans;
    int counter;
    client_t *tc, *tmp;

    if(!xcb_icccm_get_wm_transient_for_reply(globalconf.connection,
					     cookie,
					     &trans, NULL))
            return;

    tmp = tc = client_getbywin(trans);

    luaA_object_push(globalconf.L, c);
    client_set_type(globalconf.L, -1, WINDOW_TYPE_DIALOG);
    client_set_above(globalconf.L, -1, false);

    /* Verify that there are no loops in the transient_for relation after we are done */
    for(counter = 0; tmp != NULL && counter <= globalconf.stack.len; counter++)
    {
        if (tmp == c)
            /* We arrived back at the client we started from, so there is a loop */
            counter = globalconf.stack.len+1;
        tmp = tmp->transient_for;
    }
    if (counter <= globalconf.stack.len)
        client_set_transient_for(globalconf.L, -1, tc);

    lua_pop(globalconf.L, 1);
}

xcb_get_property_cookie_t
property_get_wm_client_leader(client_t *c)
{
    return xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                      WM_CLIENT_LEADER, XCB_ATOM_WINDOW, 0, 32);
}

/** Update leader hint of a client.
 * \param c The client.
 * \param cookie Cookie returned by property_get_wm_client_leader.
 */
void
property_update_wm_client_leader(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_get_property_reply_t *reply;
    void *data;

    reply = xcb_get_property_reply(globalconf.connection, cookie, NULL);

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        c->leader_window = *(xcb_window_t *) data;

    p_delete(&reply);
}

xcb_get_property_cookie_t
property_get_wm_normal_hints(client_t *c)
{
    return xcb_icccm_get_wm_normal_hints_unchecked(globalconf.connection, c->window);
}

/** Update the size hints of a client.
 * \param c The client.
 * \param cookie Cookie returned by property_get_wm_normal_hints.
 */
void
property_update_wm_normal_hints(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_icccm_get_wm_normal_hints_reply(globalconf.connection,
					cookie,
					&c->size_hints, NULL);
}

xcb_get_property_cookie_t
property_get_wm_hints(client_t *c)
{
    return xcb_icccm_get_wm_hints_unchecked(globalconf.connection, c->window);
}

/** Update the WM hints of a client.
 * \param c The client.
 * \param cookie Cookie returned by property_get_wm_hints.
 */
void
property_update_wm_hints(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_icccm_wm_hints_t wmh;

    if(!xcb_icccm_get_wm_hints_reply(globalconf.connection,
				     cookie,
				     &wmh, NULL))
        return;

    luaA_object_push(globalconf.L, c);
    client_set_urgent(globalconf.L, -1, xcb_icccm_wm_hints_get_urgency(&wmh));

    if(wmh.flags & XCB_ICCCM_WM_HINT_INPUT)
        c->nofocus = !wmh.input;

    if(wmh.flags & XCB_ICCCM_WM_HINT_WINDOW_GROUP)
        client_set_group_window(globalconf.L, -1, wmh.window_group);

    if(!c->have_ewmh_icon)
    {
        if(wmh.flags & XCB_ICCCM_WM_HINT_ICON_PIXMAP)
        {
            if(wmh.flags & XCB_ICCCM_WM_HINT_ICON_MASK)
                client_set_icon_from_pixmaps(c, wmh.icon_pixmap, wmh.icon_mask);
            else
                client_set_icon_from_pixmaps(c, wmh.icon_pixmap, XCB_NONE);
        }
        else
            client_set_icon(c, NULL);
    }

    lua_pop(globalconf.L, 1);
}

xcb_get_property_cookie_t
property_get_wm_class(client_t *c)
{
    return xcb_icccm_get_wm_class_unchecked(globalconf.connection, c->window);
}

/** Update WM_CLASS of a client.
 * \param c The client.
 * \param cookie Cookie returned by property_get_wm_class.
 */
void
property_update_wm_class(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_icccm_get_wm_class_reply_t hint;

    if(!xcb_icccm_get_wm_class_reply(globalconf.connection,
				     cookie,
				     &hint, NULL))
        return;

    luaA_object_push(globalconf.L, c);
    client_set_class_instance(globalconf.L, -1, hint.class_name, hint.instance_name);
    lua_pop(globalconf.L, 1);

    xcb_icccm_get_wm_class_reply_wipe(&hint);
}

static int
property_handle_net_wm_strut_partial(uint8_t state,
                                     xcb_window_t window)
{
    client_t *c = client_getbywin(window);

    if(c)
        ewmh_process_client_strut(c);

    return 0;
}

xcb_get_property_cookie_t
property_get_net_wm_icon(client_t *c)
{
    return ewmh_window_icon_get_unchecked(c->window);
}

void
property_update_net_wm_icon(client_t *c, xcb_get_property_cookie_t cookie)
{
    cairo_surface_t *surface = ewmh_window_icon_get_reply(cookie);

    if(!surface)
        return;

    c->have_ewmh_icon = true;
    client_set_icon(c, surface);
    cairo_surface_destroy(surface);
}

xcb_get_property_cookie_t
property_get_net_wm_pid(client_t *c)
{
    return xcb_get_property_unchecked(globalconf.connection, false, c->window, _NET_WM_PID, XCB_ATOM_CARDINAL, 0L, 1L);
}

void
property_update_net_wm_pid(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_get_property_reply_t *reply;

    reply = xcb_get_property_reply(globalconf.connection, cookie, NULL);

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

    p_delete(&reply);
}

xcb_get_property_cookie_t
property_get_wm_protocols(client_t *c)
{
    return xcb_icccm_get_wm_protocols_unchecked(globalconf.connection,
						c->window, WM_PROTOCOLS);
}

/** Update the list of supported protocols for a client.
 * \param c The client.
 * \param cookie Cookie from property_get_wm_protocols.
 */
void
property_update_wm_protocols(client_t *c, xcb_get_property_cookie_t cookie)
{
    xcb_icccm_get_wm_protocols_reply_t protocols;

    /* If this fails for any reason, we still got the old value */
    if(!xcb_icccm_get_wm_protocols_reply(globalconf.connection,
					 cookie,
					 &protocols, NULL))
        return;

    xcb_icccm_get_wm_protocols_reply_wipe(&c->protocols);
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
                            xcb_window_t window)
{
    xembed_window_t *emwin = xembed_getbywin(&globalconf.embedded, window);

    if(emwin)
    {
        xcb_get_property_cookie_t cookie =
            xcb_get_property(globalconf.connection, 0, window, _XEMBED_INFO,
                             XCB_GET_PROPERTY_TYPE_ANY, 0, 3);
        xcb_get_property_reply_t *propr =
            xcb_get_property_reply(globalconf.connection, cookie, 0);
        xembed_property_update(globalconf.connection, emwin, propr);
        p_delete(&propr);
    }

    return 0;
}

static int
property_handle_net_wm_opacity(uint8_t state,
                               xcb_window_t window)
{
    drawin_t *drawin = drawin_getbywin(window);

    if(drawin)
    {
        luaA_object_push(globalconf.L, drawin);
        window_set_opacity(globalconf.L, -1, xwindow_get_opacity(drawin->window));
        lua_pop(globalconf.L, -1);
    }
    else
    {
        client_t *c = client_getbywin(window);
        if(c)
        {
            luaA_object_push(globalconf.L, c);
            window_set_opacity(globalconf.L, -1, xwindow_get_opacity(c->window));
            lua_pop(globalconf.L, 1);
        }
    }

    return 0;
}

static int
property_handle_xrootpmap_id(uint8_t state,
                             xcb_window_t window)
{
    signal_object_emit(globalconf.L, &global_signals, "wallpaper_changed", 0);
    return 0;
}

/** The property notify event handler handling xproperties.
 * \param ev The event.
 */
static void
property_handle_propertynotify_xproperty(xcb_property_notify_event_t *ev)
{
    xproperty_t *prop;
    xproperty_t lookup = { .atom = ev->atom };
    buffer_t buf;
    void *obj;

    prop = xproperty_array_lookup(&globalconf.xproperties, &lookup);
    if(!prop)
        /* Property is not registered */
        return;

    if (ev->window != globalconf.screen->root)
    {
        obj = client_getbywin(ev->window);
        if(!obj)
            obj = drawin_getbywin(ev->window);
        if(!obj)
            return;
    } else
        obj = NULL;

    /* Get us the name of the property */
    buffer_inita(&buf, a_strlen(prop->name) + a_strlen("xproperty::") + 1);
    buffer_addf(&buf, "xproperty::%s", prop->name);

    /* And emit the right signal */
    if (obj)
    {
        luaA_object_push(globalconf.L, obj);
        luaA_object_emit_signal(globalconf.L, -1, buf.s, 0);
        lua_pop(globalconf.L, 1);
    } else
        signal_object_emit(globalconf.L, &global_signals, buf.s, 0);
    buffer_wipe(&buf);
}

/** The property notify event handler.
 * \param ev The event.
 */
void
property_handle_propertynotify(xcb_property_notify_event_t *ev)
{
    int (*handler)(uint8_t state,
                   xcb_window_t window) = NULL;

    globalconf.timestamp = ev->time;

    property_handle_propertynotify_xproperty(ev);

    /* Find the correct event handler */
#define HANDLE(atom_, cb) \
    if (ev->atom == atom_) \
    { \
        handler = cb; \
    } else
#define END return

    /* Xembed stuff */
    HANDLE(_XEMBED_INFO, property_handle_xembed_info)

    /* ICCCM stuff */
    HANDLE(XCB_ATOM_WM_TRANSIENT_FOR, property_handle_wm_transient_for)
    HANDLE(WM_CLIENT_LEADER, property_handle_wm_client_leader)
    HANDLE(XCB_ATOM_WM_NORMAL_HINTS, property_handle_wm_normal_hints)
    HANDLE(XCB_ATOM_WM_HINTS, property_handle_wm_hints)
    HANDLE(XCB_ATOM_WM_NAME, property_handle_wm_name)
    HANDLE(XCB_ATOM_WM_ICON_NAME, property_handle_wm_icon_name)
    HANDLE(XCB_ATOM_WM_CLASS, property_handle_wm_class)
    HANDLE(WM_PROTOCOLS, property_handle_wm_protocols)
    HANDLE(XCB_ATOM_WM_CLIENT_MACHINE, property_handle_wm_client_machine)
    HANDLE(WM_WINDOW_ROLE, property_handle_wm_window_role)

    /* EWMH stuff */
    HANDLE(_NET_WM_NAME, property_handle_net_wm_name)
    HANDLE(_NET_WM_ICON_NAME, property_handle_net_wm_icon_name)
    HANDLE(_NET_WM_STRUT_PARTIAL, property_handle_net_wm_strut_partial)
    HANDLE(_NET_WM_ICON, property_handle_net_wm_icon)
    HANDLE(_NET_WM_PID, property_handle_net_wm_pid)
    HANDLE(_NET_WM_WINDOW_OPACITY, property_handle_net_wm_opacity)

    /* background change */
    HANDLE(_XROOTPMAP_ID, property_handle_xrootpmap_id)

    /* If nothing was found, return */
    END;

#undef HANDLE
#undef END

    (*handler)(ev->state, ev->window);
}

/** Register a new xproperty.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The name of the X11 property
 * \lparam One of "string", "number" or "boolean"
 */
int
luaA_register_xproperty(lua_State *L)
{
    const char *name;
    struct xproperty property;
    struct xproperty *found;
    const char *const args[] = { "string", "number", "boolean" };
    xcb_intern_atom_reply_t *atom_r;
    int type;

    name = luaL_checkstring(L, 1);
    type = luaL_checkoption(L, 2, NULL, args);
    if (type == 0)
        property.type = PROP_STRING;
    else if (type == 1)
        property.type = PROP_NUMBER;
    else
        property.type = PROP_BOOLEAN;

    atom_r = xcb_intern_atom_reply(globalconf.connection,
                                   xcb_intern_atom_unchecked(globalconf.connection, false,
                                                             a_strlen(name), name),
                                   NULL);
    if(!atom_r)
        return 0;

    property.atom = atom_r->atom;
    p_delete(&atom_r);

    found = xproperty_array_lookup(&globalconf.xproperties, &property);
    if(found)
    {
        /* Property already registered */
        if(found->type != property.type)
            return luaL_error(L, "xproperty '%s' already registered with different type", name);
    }
    else
    {
        buffer_t buf;
        buffer_inita(&buf, a_strlen(name) + a_strlen("xproperty::") + 1);
        buffer_addf(&buf, "xproperty::%s", name);

        property.name = a_strdup(name);
        xproperty_array_insert(&globalconf.xproperties, property);
        signal_add(&window_class.signals, buf.s);
        signal_add(&global_signals, buf.s);
        buffer_wipe(&buf);
    }

    return 0;
}

/** Set an xproperty.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_set_xproperty(lua_State *L)
{
    return window_set_xproperty(L, globalconf.screen->root, 1, 2);
}

/** Get an xproperty.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_get_xproperty(lua_State *L)
{
    return window_get_xproperty(L, globalconf.screen->root, 1);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
