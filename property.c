/*
 * property.c - property handlers
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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
#include "client.h"
#include "ewmh.h"
#include "wibox.h"
#include "common/atoms.h"
#include "common/xutil.h"
#include "window.h"

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
                                                                               c->win),
                                            &trans, NULL))
            return;
    }

    c->type = WINDOW_TYPE_DIALOG;
    c->transient_for = client_getbywin(trans);
    client_setabove(c, false);
}

static int
property_handle_wm_transient_for(void *data,
                                 xcb_connection_t *connection,
                                 uint8_t state,
                                 xcb_window_t window,
                                 xcb_atom_t name,
                                 xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_transient_for(c, reply);

    return 0;
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
        client_leader_q = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                                     WM_CLIENT_LEADER, WINDOW, 0, 32);

        reply = xcb_get_property_reply(globalconf.connection, client_leader_q, NULL);
    }

    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
        c->leader_win = *(xcb_window_t *) data;

    /* Only free when we created a reply ourselves. */
    if(no_reply)
        p_delete(&reply);
}

static int
property_handle_wm_client_leader(void *data,
                                xcb_connection_t *connection,
                                uint8_t state,
                                xcb_window_t window,
                                xcb_atom_t name,
                                xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_client_leader(c, reply);

    return 0;
}

/** Update the size hints of a client.
 * \param c The client.
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
                                                                            c->win),
                                          &c->size_hints, NULL))
            return;
    }
}

static int
property_handle_wm_normal_hints(void *data,
                                xcb_connection_t *connection,
                                uint8_t state,
                                xcb_window_t window,
                                xcb_atom_t name,
                                xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_normal_hints(c, reply);

    return 0;
}

/** Update the WM hints of a client.
 * \param c The client.
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
                                  xcb_get_wm_hints_unchecked(globalconf.connection, c->win),
                                  &wmh, NULL))
            return;
    }

    bool isurgent = xcb_wm_hints_get_urgency(&wmh);
    client_seturgent(c, isurgent);
    if(wmh.flags & XCB_WM_HINT_STATE &&
       wmh.initial_state == XCB_WM_STATE_WITHDRAWN)
        client_setborder(c, 0);

    if(wmh.flags & XCB_WM_HINT_INPUT)
        c->nofocus = !wmh.input;

    if(wmh.flags & XCB_WM_HINT_WINDOW_GROUP)
        c->group_win = wmh.window_group;
}

static int
property_handle_wm_hints(void *data,
                         xcb_connection_t *connection,
                         uint8_t state,
                         xcb_window_t window,
                         xcb_atom_t name,
                         xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_hints(c, reply);

    return 0;
}

/** Update client name attribute with its new title.
 * \param c The client.
 * \param Return true if it has been updated.
 */
void
property_update_wm_name(client_t *c)
{
    char *name;
    ssize_t len;

    if(!xutil_text_prop_get(globalconf.connection, c->win, _NET_WM_NAME, &name, &len))
        if(!xutil_text_prop_get(globalconf.connection, c->win, WM_NAME, &name, &len))
            return;

    p_delete(&c->name);

    c->name = name;

    /* call hook */
    hook_property(client, c, "name");
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
                                   xcb_get_wm_class_unchecked(globalconf.connection, c->win),
                                   &hint, NULL))
            return;
    }

    p_delete(&c->instance);
    p_delete(&c->class);

    c->instance = a_strdup(hint.instance_name);
    c->class = a_strdup(hint.class_name);
    /* only delete reply if we get it ourselves */
    if(!reply)
        xcb_get_wm_class_reply_wipe(&hint);
}

/** Update client icon name attribute with its new title.
 * \param c The client.
 * \param Return true if it has been updated.
 */
void
property_update_wm_icon_name(client_t *c)
{
    char *name;
    ssize_t len;

    if(!xutil_text_prop_get(globalconf.connection, c->win, _NET_WM_ICON_NAME, &name, &len))
        if(!xutil_text_prop_get(globalconf.connection, c->win, WM_ICON_NAME, &name, &len))
            return;

    p_delete(&c->icon_name);

    c->icon_name = name;

    /* call hook */
    hook_property(client, c, "icon_name");
}

static int
property_handle_wm_name(void *data,
                        xcb_connection_t *connection,
                        uint8_t state,
                        xcb_window_t window,
                        xcb_atom_t name,
                        xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_name(c);

    return 0;
}

static int
property_handle_wm_icon_name(void *data,
                             xcb_connection_t *connection,
                             uint8_t state,
                             xcb_window_t window,
                             xcb_atom_t name,
                             xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_icon_name(c);

    return 0;
}

static int
property_handle_wm_class(void *data,
                         xcb_connection_t *connection,
                         uint8_t state,
                         xcb_window_t window,
                         xcb_atom_t name,
                         xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        property_update_wm_class(c, reply);

    return 0;
}

static int
property_handle_net_wm_strut_partial(void *data,
                                     xcb_connection_t *connection,
                                     uint8_t state,
                                     xcb_window_t window,
                                     xcb_atom_t name,
                                     xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
        ewmh_process_client_strut(c, reply);

    return 0;
}

static int
property_handle_net_wm_icon(void *data,
                            xcb_connection_t *connection,
                            uint8_t state,
                            xcb_window_t window,
                            xcb_atom_t name,
                            xcb_get_property_reply_t *reply)
{
    client_t *c = client_getbywin(window);

    if(c)
    {
        image_unref(globalconf.L, c->icon);
        if(ewmh_window_icon_from_reply(reply))
            c->icon = image_ref(globalconf.L, -1);
        else
            c->icon = NULL;
        /* execute hook */
        hook_property(client, c, "icon");
    }

    return 0;
}

/** The property notify event handler.
 * \param data currently unused.
 * \param connection The connection to the X server.
 * \param ev The event.
 */
static int
property_handle_xembed_info(void *data __attribute__ ((unused)),
                            xcb_connection_t *connection,
                            uint8_t state,
                            xcb_window_t window,
                            xcb_atom_t name,
                            xcb_get_property_reply_t *reply)
{
    xembed_window_t *emwin = xembed_getbywin(&globalconf.embedded, window);

    if(emwin)
        xembed_property_update(connection, emwin, reply);

    return 0;
}

static int
property_handle_xrootpmap_id(void *data __attribute__ ((unused)),
                             xcb_connection_t *connection,
                             uint8_t state,
                             xcb_window_t window,
                             xcb_atom_t name,
                             xcb_get_property_reply_t *reply)
{
    if(globalconf.xinerama_is_active)
        foreach(w, globalconf.wiboxes)
            (*w)->need_update = true;
    else
    {
        int screen = xutil_root2screen(connection, window);
        foreach(w, globalconf.wiboxes)
            if(screen == screen_array_indexof(&globalconf.screens, (*w)->screen))
               (*w)->need_update = true;
    }

    return 0;
}

static int
property_handle_opacity(void *data __attribute__ ((unused)),
                             xcb_connection_t *connection,
                             uint8_t state,
                             xcb_window_t window,
                             xcb_atom_t name,
                             xcb_get_property_reply_t *reply)
{
    wibox_t *wibox = wibox_getbywin(window);

    if (wibox)
        wibox->sw.opacity = window_opacity_get_from_reply(reply);
    return 0;
}

void a_xcb_set_property_handlers(void)
{
    /* init */
    xcb_property_handlers_init(&globalconf.prophs, &globalconf.evenths);

    /* Xembed stuff */
    xcb_property_set_handler(&globalconf.prophs, _XEMBED_INFO, UINT_MAX,
                             property_handle_xembed_info, NULL);

    /* ICCCM stuff */
    xcb_property_set_handler(&globalconf.prophs, WM_TRANSIENT_FOR, UINT_MAX,
                             property_handle_wm_transient_for, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_CLIENT_LEADER, UINT_MAX,
                             property_handle_wm_client_leader, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_NORMAL_HINTS, UINT_MAX,
                             property_handle_wm_normal_hints, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_HINTS, UINT_MAX,
                             property_handle_wm_hints, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_NAME, UINT_MAX,
                             property_handle_wm_name, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_ICON_NAME, UINT_MAX,
                             property_handle_wm_icon_name, NULL);
    xcb_property_set_handler(&globalconf.prophs, WM_CLASS, UINT_MAX,
                             property_handle_wm_class, NULL);

    /* EWMH stuff */
    xcb_property_set_handler(&globalconf.prophs, _NET_WM_NAME, UINT_MAX,
                             property_handle_wm_name, NULL);
    xcb_property_set_handler(&globalconf.prophs, _NET_WM_ICON_NAME, UINT_MAX,
                             property_handle_wm_icon_name, NULL);
    xcb_property_set_handler(&globalconf.prophs, _NET_WM_STRUT_PARTIAL, UINT_MAX,
                             property_handle_net_wm_strut_partial, NULL);
    xcb_property_set_handler(&globalconf.prophs, _NET_WM_ICON, UINT_MAX,
                             property_handle_net_wm_icon, NULL);

    /* background change */
    xcb_property_set_handler(&globalconf.prophs, _XROOTPMAP_ID, 1,
                             property_handle_xrootpmap_id, NULL);

    /* Opacity */
    xcb_property_set_handler(&globalconf.prophs, _NET_WM_WINDOW_OPACITY, 1,
                             property_handle_opacity, NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
