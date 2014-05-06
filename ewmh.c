/*
 * ewmh.c - EWMH support functions
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include <sys/types.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "ewmh.h"
#include "objects/tag.h"
#include "screen.h"
#include "objects/client.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/buffer.h"
#include "common/xutil.h"

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

/** Update client EWMH hints.
 * \param c The client.
 */
static int
ewmh_client_update_hints(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    xcb_atom_t state[10]; /* number of defined state atoms */
    int i = 0;

    if(c->modal)
        state[i++] = _NET_WM_STATE_MODAL;
    if(c->fullscreen)
        state[i++] = _NET_WM_STATE_FULLSCREEN;
    if(c->maximized_vertical)
        state[i++] = _NET_WM_STATE_MAXIMIZED_VERT;
    if(c->maximized_horizontal)
        state[i++] = _NET_WM_STATE_MAXIMIZED_HORZ;
    if(c->sticky)
        state[i++] = _NET_WM_STATE_STICKY;
    if(c->skip_taskbar)
        state[i++] = _NET_WM_STATE_SKIP_TASKBAR;
    if(c->above)
        state[i++] = _NET_WM_STATE_ABOVE;
    if(c->below)
        state[i++] = _NET_WM_STATE_BELOW;
    if(c->minimized)
        state[i++] = _NET_WM_STATE_HIDDEN;
    if(c->urgent)
        state[i++] = _NET_WM_STATE_DEMANDS_ATTENTION;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        c->window, _NET_WM_STATE, XCB_ATOM_ATOM, 32, i, state);

    return 0;
}

static int
ewmh_update_net_active_window(lua_State *L)
{
    xcb_window_t win;

    if(globalconf.focus.client)
        win = globalconf.focus.client->window;
    else
        win = XCB_NONE;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			globalconf.screen->root,
			_NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &win);

    return 0;
}

static int
ewmh_update_net_client_list(lua_State *L)
{
    xcb_window_t *wins = p_alloca(xcb_window_t, globalconf.clients.len);

    int n = 0;
    foreach(client, globalconf.clients)
        wins[n++] = (*client)->window;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.screen->root,
                        _NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, n, wins);

    return 0;
}

void
ewmh_init(void)
{
    xcb_window_t father;
    xcb_screen_t *xscreen = globalconf.screen;
    xcb_atom_t atom[] =
    {
        _NET_SUPPORTED,
        _NET_SUPPORTING_WM_CHECK,
        _NET_STARTUP_ID,
        _NET_CLIENT_LIST,
        _NET_CLIENT_LIST_STACKING,
        _NET_NUMBER_OF_DESKTOPS,
        _NET_CURRENT_DESKTOP,
        _NET_DESKTOP_NAMES,
        _NET_ACTIVE_WINDOW,
        _NET_CLOSE_WINDOW,
        _NET_WM_NAME,
        _NET_WM_STRUT_PARTIAL,
        _NET_WM_ICON_NAME,
        _NET_WM_VISIBLE_ICON_NAME,
        _NET_WM_DESKTOP,
        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_DESKTOP,
        _NET_WM_WINDOW_TYPE_DOCK,
        _NET_WM_WINDOW_TYPE_TOOLBAR,
        _NET_WM_WINDOW_TYPE_MENU,
        _NET_WM_WINDOW_TYPE_UTILITY,
        _NET_WM_WINDOW_TYPE_SPLASH,
        _NET_WM_WINDOW_TYPE_DIALOG,
        _NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        _NET_WM_WINDOW_TYPE_POPUP_MENU,
        _NET_WM_WINDOW_TYPE_TOOLTIP,
        _NET_WM_WINDOW_TYPE_NOTIFICATION,
        _NET_WM_WINDOW_TYPE_COMBO,
        _NET_WM_WINDOW_TYPE_DND,
        _NET_WM_WINDOW_TYPE_NORMAL,
        _NET_WM_ICON,
        _NET_WM_PID,
        _NET_WM_STATE,
        _NET_WM_STATE_STICKY,
        _NET_WM_STATE_SKIP_TASKBAR,
        _NET_WM_STATE_FULLSCREEN,
        _NET_WM_STATE_MAXIMIZED_HORZ,
        _NET_WM_STATE_MAXIMIZED_VERT,
        _NET_WM_STATE_ABOVE,
        _NET_WM_STATE_BELOW,
        _NET_WM_STATE_MODAL,
        _NET_WM_STATE_HIDDEN,
        _NET_WM_STATE_DEMANDS_ATTENTION
    };
    int i;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, _NET_SUPPORTED, XCB_ATOM_ATOM, 32,
                        countof(atom), atom);

    /* create our own window */
    father = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, xscreen->root_depth,
                      father, xscreen->root, -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, xscreen->root_visual, 0, NULL);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, _NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32,
                        1, &father);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32,
                        1, &father);

    /* set the window manager name */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_NAME, UTF8_STRING, 8, 7, "awesome");

    /* set the window manager PID */
    i = getpid();
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_PID, XCB_ATOM_CARDINAL, 32, 1, &i);


    luaA_class_connect_signal(globalconf.L, &client_class, "focus", ewmh_update_net_active_window);
    luaA_class_connect_signal(globalconf.L, &client_class, "unfocus", ewmh_update_net_active_window);
    luaA_class_connect_signal(globalconf.L, &client_class, "manage", ewmh_update_net_client_list);
    luaA_class_connect_signal(globalconf.L, &client_class, "unmanage", ewmh_update_net_client_list);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::modal" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::fullscreen" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::maximized_horizontal" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::maximized_vertical" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::sticky" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::skip_taskbar" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::above" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::below" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::minimized" , ewmh_client_update_hints);
    luaA_class_connect_signal(globalconf.L, &client_class, "property::urgent" , ewmh_client_update_hints);
}

/** Set the client list in stacking order, bottom to top.
 */
void
ewmh_update_net_client_list_stacking(void)
{
    int n = 0;
    xcb_window_t *wins = p_alloca(xcb_window_t, globalconf.stack.len);

    foreach(client, globalconf.stack)
        wins[n++] = (*client)->window;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			globalconf.screen->root,
			_NET_CLIENT_LIST_STACKING, XCB_ATOM_WINDOW, 32, n, wins);
}

void
ewmh_update_net_numbers_of_desktop(void)
{
    uint32_t count = globalconf.tags.len;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			globalconf.screen->root,
			_NET_NUMBER_OF_DESKTOPS, XCB_ATOM_CARDINAL, 32, 1, &count);
}

void
ewmh_update_net_current_desktop(void)
{
    uint32_t idx = tags_get_first_selected_index();

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.screen->root,
                        _NET_CURRENT_DESKTOP, XCB_ATOM_CARDINAL, 32, 1, &idx);
}

void
ewmh_update_net_desktop_names(void)
{
    buffer_t buf;

    buffer_inita(&buf, BUFSIZ);

    foreach(tag, globalconf.tags)
    {
        buffer_adds(&buf, tag_get_name(*tag));
        buffer_addc(&buf, '\0');
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			globalconf.screen->root,
			_NET_DESKTOP_NAMES, UTF8_STRING, 8, buf.len, buf.s);
    buffer_wipe(&buf);
}

static void
ewmh_process_state_atom(client_t *c, xcb_atom_t state, int set)
{
    luaA_object_push(globalconf.L, c);

    if(state == _NET_WM_STATE_STICKY)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_sticky(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_sticky(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_sticky(globalconf.L, -1, !c->sticky);
    }
    else if(state == _NET_WM_STATE_SKIP_TASKBAR)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_skip_taskbar(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_skip_taskbar(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_skip_taskbar(globalconf.L, -1, !c->skip_taskbar);
    }
    else if(state == _NET_WM_STATE_FULLSCREEN)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_fullscreen(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_fullscreen(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_fullscreen(globalconf.L, -1, !c->fullscreen);
    }
    else if(state == _NET_WM_STATE_MAXIMIZED_HORZ)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_maximized_horizontal(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_maximized_horizontal(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_maximized_horizontal(globalconf.L, -1, !c->maximized_horizontal);
    }
    else if(state == _NET_WM_STATE_MAXIMIZED_VERT)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_maximized_vertical(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_maximized_vertical(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_maximized_vertical(globalconf.L, -1, !c->maximized_vertical);
    }
    else if(state == _NET_WM_STATE_ABOVE)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_above(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_above(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_above(globalconf.L, -1, !c->above);
    }
    else if(state == _NET_WM_STATE_BELOW)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_below(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_below(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_below(globalconf.L, -1, !c->below);
    }
    else if(state == _NET_WM_STATE_MODAL)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_modal(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_modal(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_modal(globalconf.L, -1, !c->modal);
    }
    else if(state == _NET_WM_STATE_HIDDEN)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_minimized(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_minimized(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_minimized(globalconf.L, -1, !c->minimized);
    }
    else if(state == _NET_WM_STATE_DEMANDS_ATTENTION)
    {
        if(set == _NET_WM_STATE_REMOVE)
            client_set_urgent(globalconf.L, -1, false);
        else if(set == _NET_WM_STATE_ADD)
            client_set_urgent(globalconf.L, -1, true);
        else if(set == _NET_WM_STATE_TOGGLE)
            client_set_urgent(globalconf.L, -1, !c->urgent);
    }

    lua_pop(globalconf.L, 1);
}

static void
ewmh_process_desktop(client_t *c, uint32_t desktop)
{
    int idx = desktop;
    if(desktop == 0xffffffff)
    {
        luaA_object_push(globalconf.L, c);
        lua_pushnil(globalconf.L);
        luaA_object_emit_signal(globalconf.L, -2, "request::tag", 1);
        /* Pop the client, arguments are already popped */
        lua_pop(globalconf.L, 1);
    }
    else if (idx >= 0 && idx < globalconf.tags.len)
    {
        luaA_object_push(globalconf.L, c);
        luaA_object_push(globalconf.L, globalconf.tags.tab[idx]);
        luaA_object_emit_signal(globalconf.L, -2, "request::tag", 1);
        /* Pop the client, arguments are already popped */
        lua_pop(globalconf.L, 1);
    }
}

int
ewmh_process_client_message(xcb_client_message_event_t *ev)
{
    client_t *c;

    if(ev->type == _NET_CURRENT_DESKTOP)
    {
        int idx = ev->data.data32[0];
        if (idx >= 0 && idx < globalconf.tags.len)
        {
            luaA_object_push(globalconf.L, globalconf.tags.tab[idx]);
            luaA_object_emit_signal(globalconf.L, -1, "request::select", 0);
            lua_pop(globalconf.L, 1);
        }
    }
    else if(ev->type == _NET_CLOSE_WINDOW)
    {
        if((c = client_getbywin(ev->window)))
           client_kill(c);
    }
    else if(ev->type == _NET_WM_DESKTOP)
    {
        if((c = client_getbywin(ev->window)))
        {
            ewmh_process_desktop(c, ev->data.data32[0]);
        }
    }
    else if(ev->type == _NET_WM_STATE)
    {
        if((c = client_getbywin(ev->window)))
        {
            ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[1], ev->data.data32[0]);
            if(ev->data.data32[2])
                ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[2],
                                        ev->data.data32[0]);
        }
    }
    else if(ev->type == _NET_ACTIVE_WINDOW)
    {
        if((c = client_getbywin(ev->window))) {
            luaA_object_push(globalconf.L, c);
            lua_pushstring(globalconf.L,"ewmh");
            luaA_object_emit_signal(globalconf.L, -2, "request::activate", 1);
            lua_pop(globalconf.L, 1);
        }
    }

    return 0;
}

/** Update the client active desktop.
 * This is "wrong" since it can be on several tags, but EWMH has a strict view
 * of desktop system so just take the first tag.
 * \param c The client.
 */
void
ewmh_client_update_desktop(client_t *c)
{
    int i;

    for(i = 0; i < globalconf.tags.len; i++)
        if(is_client_tagged(c, globalconf.tags.tab[i]))
        {
            xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                                c->window, _NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1, &i);
            return;
        }
    /* It doesn't have any tags, remove the property */
    xcb_delete_property(globalconf.connection, c->window, _NET_WM_DESKTOP);
}

/** Update the client struts.
 * \param window The window to update the struts for.
 * \param strut The strut type to update the window with.
 */
void
ewmh_update_strut(xcb_window_t window, strut_t *strut)
{
    if(window)
    {
        const uint32_t state[] =
        {
            strut->left,
            strut->right,
            strut->top,
            strut->bottom,
            strut->left_start_y,
            strut->left_end_y,
            strut->right_start_y,
            strut->right_end_y,
            strut->top_start_x,
            strut->top_end_x,
            strut->bottom_start_x,
            strut->bottom_end_x
        };

        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                            window, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, countof(state), state);
    }
}

/** Update the window type.
 * \param window The window to update.
 * \param type The new type to set.
 */
void
ewmh_update_window_type(xcb_window_t window, uint32_t type)
{
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        window, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 32, 1, &type);
}

void
ewmh_client_check_hints(client_t *c)
{
    xcb_atom_t *state;
    void *data = NULL;
    xcb_get_property_cookie_t c0, c1, c2;
    xcb_get_property_reply_t *reply;

    /* Send the GetProperty requests which will be processed later */
    c0 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_DESKTOP, XCB_GET_PROPERTY_TYPE_ANY, 0, 1);

    c1 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_STATE, XCB_ATOM_ATOM, 0, UINT32_MAX);

    c2 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 0, UINT32_MAX);

    reply = xcb_get_property_reply(globalconf.connection, c0, NULL);
    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
    {
        ewmh_process_desktop(c, *(uint32_t *) data);
    }

    p_delete(&reply);

    reply = xcb_get_property_reply(globalconf.connection, c1, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(int i = 0; i < xcb_get_property_value_length(reply) / ssizeof(xcb_atom_t); i++)
            ewmh_process_state_atom(c, state[i], _NET_WM_STATE_ADD);
    }

    p_delete(&reply);

    reply = xcb_get_property_reply(globalconf.connection, c2, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(int i = 0; i < xcb_get_property_value_length(reply) / ssizeof(xcb_atom_t); i++)
            if(state[i] == _NET_WM_WINDOW_TYPE_DESKTOP)
                c->type = MAX(c->type, WINDOW_TYPE_DESKTOP);
            else if(state[i] == _NET_WM_WINDOW_TYPE_DIALOG)
                c->type = MAX(c->type, WINDOW_TYPE_DIALOG);
            else if(state[i] == _NET_WM_WINDOW_TYPE_SPLASH)
                c->type = MAX(c->type, WINDOW_TYPE_SPLASH);
            else if(state[i] == _NET_WM_WINDOW_TYPE_DOCK)
                c->type = MAX(c->type, WINDOW_TYPE_DOCK);
            else if(state[i] == _NET_WM_WINDOW_TYPE_MENU)
                c->type = MAX(c->type, WINDOW_TYPE_MENU);
            else if(state[i] == _NET_WM_WINDOW_TYPE_TOOLBAR)
                c->type = MAX(c->type, WINDOW_TYPE_TOOLBAR);
            else if(state[i] == _NET_WM_WINDOW_TYPE_UTILITY)
                c->type = MAX(c->type, WINDOW_TYPE_UTILITY);
    }

    p_delete(&reply);
}

/** Process the WM strut of a client.
 * \param c The client.
 * \param strut_r (Optional) An existing reply.
 */
void
ewmh_process_client_strut(client_t *c)
{
    void *data;
    xcb_get_property_reply_t *strut_r;

    xcb_get_property_cookie_t strut_q = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                                                   _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 0, 12);
    strut_r = xcb_get_property_reply(globalconf.connection, strut_q, NULL);

    if(strut_r
       && strut_r->value_len
       && (data = xcb_get_property_value(strut_r)))
    {
        uint32_t *strut = data;

        if(c->strut.left != strut[0]
           || c->strut.right != strut[1]
           || c->strut.top != strut[2]
           || c->strut.bottom != strut[3]
           || c->strut.left_start_y != strut[4]
           || c->strut.left_end_y != strut[5]
           || c->strut.right_start_y != strut[6]
           || c->strut.right_end_y != strut[7]
           || c->strut.top_start_x != strut[8]
           || c->strut.top_end_x != strut[9]
           || c->strut.bottom_start_x != strut[10]
           || c->strut.bottom_end_x != strut[11])
        {
            c->strut.left = strut[0];
            c->strut.right = strut[1];
            c->strut.top = strut[2];
            c->strut.bottom = strut[3];
            c->strut.left_start_y = strut[4];
            c->strut.left_end_y = strut[5];
            c->strut.right_start_y = strut[6];
            c->strut.right_end_y = strut[7];
            c->strut.top_start_x = strut[8];
            c->strut.top_end_x = strut[9];
            c->strut.bottom_start_x = strut[10];
            c->strut.bottom_end_x = strut[11];

            luaA_object_push(globalconf.L, c);
            luaA_object_emit_signal(globalconf.L, -1, "property::struts", 0);
            lua_pop(globalconf.L, 1);
        }
    }

    p_delete(&strut_r);
}

/** Send request to get NET_WM_ICON (EWMH)
 * \param w The window.
 * \return The cookie associated with the request.
 */
xcb_get_property_cookie_t
ewmh_window_icon_get_unchecked(xcb_window_t w)
{
  return xcb_get_property_unchecked(globalconf.connection, false, w,
                                    _NET_WM_ICON, XCB_ATOM_CARDINAL, 0, UINT32_MAX);
}

static cairo_surface_t *
ewmh_window_icon_from_reply(xcb_get_property_reply_t *r)
{
    uint32_t *data;
    uint64_t len;

    if(!r || r->type != XCB_ATOM_CARDINAL || r->format != 32 || r->length < 2)
        return 0;

    data = (uint32_t *) xcb_get_property_value(r);
    if (!data)
        return 0;

    /* Check that the property is as long as it should be, handling integer
     * overflow. <uint32_t> times <another uint32_t casted to uint64_t> always
     * fits into an uint64_t and thus this multiplication cannot overflow.
     */
    len = data[0] * (uint64_t) data[1];
    if (!data[0] || !data[1] || len > r->length - 2)
        return 0;

    return draw_surface_from_data(data[0], data[1], data + 2);
}

/** Get NET_WM_ICON.
 * \param cookie The cookie.
 * \return The number of elements on stack.
 */
cairo_surface_t *
ewmh_window_icon_get_reply(xcb_get_property_cookie_t cookie)
{
    xcb_get_property_reply_t *r = xcb_get_property_reply(globalconf.connection, cookie, NULL);
    cairo_surface_t *surface = ewmh_window_icon_from_reply(r);
    p_delete(&r);
    return surface;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
