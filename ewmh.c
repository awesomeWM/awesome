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
#include "tag.h"
#include "screen.h"
#include "client.h"
#include "widget.h"
#include "wibox.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/buffer.h"
#include "common/xutil.h"

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

/** Update the desktop geometry.
 * \param phys_screen The physical screen id.
 */
static void
ewmh_update_desktop_geometry(int phys_screen)
{
    area_t geom = screen_area_get(&globalconf.screens.tab[phys_screen], false);
    uint32_t sizes[] = { geom.width, geom.height };

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xutil_screen_get(globalconf.connection, phys_screen)->root,
                        _NET_DESKTOP_GEOMETRY, CARDINAL, 32, countof(sizes), sizes);
}

void
ewmh_init(int phys_screen)
{
    xcb_window_t father;
    xcb_screen_t *xscreen = xutil_screen_get(globalconf.connection, phys_screen);
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
        _NET_DESKTOP_GEOMETRY,
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
                        xscreen->root, _NET_SUPPORTED, ATOM, 32,
                        countof(atom), atom);

    /* create our own window */
    father = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, xscreen->root_depth,
                      father, xscreen->root, -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, xscreen->root_visual, 0, NULL);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xscreen->root, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
                        1, &father);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
                        1, &father);

    /* set the window manager name */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_NAME, UTF8_STRING, 8, 7, "awesome");

    /* set the window manager PID */
    i = getpid();
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        father, _NET_WM_PID, CARDINAL, 32, 1, &i);

    ewmh_update_desktop_geometry(phys_screen);
}

void
ewmh_update_net_client_list(int phys_screen)
{
    xcb_window_t *wins = p_alloca(xcb_window_t, globalconf.clients.len);

    int n = 0;
    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;
        if(c->phys_screen == phys_screen)
            wins[n++] = c->window;
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_CLIENT_LIST, WINDOW, 32, n, wins);
}

/** Set the client list in stacking order, bottom to top.
 * \param phys_screen The physical screen id.
 */
void
ewmh_update_net_client_list_stacking(int phys_screen)
{
    int n = 0;
    xcb_window_t *wins = p_alloca(xcb_window_t, globalconf.stack.len);

    foreach(client, globalconf.stack)
        if((*client)->phys_screen == phys_screen)
            wins[n++] = (*client)->window;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_CLIENT_LIST_STACKING, WINDOW, 32, n, wins);
}

void
ewmh_update_net_numbers_of_desktop(int phys_screen)
{
    uint32_t count = globalconf.screens.tab[phys_screen].tags.len;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_NUMBER_OF_DESKTOPS, CARDINAL, 32, 1, &count);
}

void
ewmh_update_net_current_desktop(int phys_screen)
{
    uint32_t idx = tags_get_first_selected_index(&globalconf.screens.tab[phys_screen]);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xutil_screen_get(globalconf.connection, phys_screen)->root,
                        _NET_CURRENT_DESKTOP, CARDINAL, 32, 1, &idx);
}

void
ewmh_update_net_desktop_names(int phys_screen)
{
    buffer_t buf;

    buffer_inita(&buf, BUFSIZ);

    foreach(tag, globalconf.screens.tab[phys_screen].tags)
    {
        buffer_adds(&buf, tag_get_name(*tag));
        buffer_addc(&buf, '\0');
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_DESKTOP_NAMES, UTF8_STRING, 8, buf.len, buf.s);
    buffer_wipe(&buf);
}

void
ewmh_update_net_active_window(int phys_screen)
{
    xcb_window_t win;

    if(globalconf.screen_focus->client_focus
       && globalconf.screen_focus->client_focus->phys_screen == phys_screen)
        win = globalconf.screen_focus->client_focus->window;
    else
        win = XCB_NONE;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xutil_screen_get(globalconf.connection, phys_screen)->root,
			_NET_ACTIVE_WINDOW, WINDOW, 32, 1, &win);
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
        {
            client_set_skip_taskbar(globalconf.L, -1, false);
            ewmh_client_update_hints(c);
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            client_set_skip_taskbar(globalconf.L, -1, true);
            ewmh_client_update_hints(c);
        }
        else if(set == _NET_WM_STATE_TOGGLE)
        {
            client_set_skip_taskbar(globalconf.L, -1, !c->skip_taskbar);
            ewmh_client_update_hints(c);
        }
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
}

int
ewmh_process_client_message(xcb_client_message_event_t *ev)
{
    client_t *c;
    int screen;

    if(ev->type == _NET_CURRENT_DESKTOP)
        for(screen = 0;
            screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
            screen++)
        {
            if(ev->window == xutil_screen_get(globalconf.connection, screen)->root)
                tag_view_only_byindex(&globalconf.screens.tab[screen], ev->data.data32[0]);
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
            tag_array_t *tags = &c->screen->tags;

            if(ev->data.data32[0] == 0xffffffff)
                c->sticky = true;
            else
                for(int i = 0; i < tags->len; i++)
                    if((int)ev->data.data32[0] == i)
                    {
                        luaA_object_push(globalconf.L, tags->tab[i]);
                        tag_client(c);
                    }
                    else
                        untag_client(c, tags->tab[i]);
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
        if((c = client_getbywin(ev->window)))
            client_focus(c);
    }

    return 0;
}

/** Update client EWMH hints.
 * \param c The client.
 */
void
ewmh_client_update_hints(client_t *c)
{
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
                        c->window, _NET_WM_STATE, ATOM, 32, i, state);
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
    tag_array_t *tags = &c->screen->tags;

    for(i = 0; i < tags->len; i++)
        if(is_client_tagged(c, tags->tab[i]))
        {
            xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                                c->window, _NET_WM_DESKTOP, CARDINAL, 32, 1, &i);
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
                        window, _NET_WM_STRUT_PARTIAL, CARDINAL, 32, countof(state), state);
}

void
ewmh_client_check_hints(client_t *c)
{
    xcb_atom_t *state;
    void *data = NULL;
    int desktop;
    xcb_get_property_cookie_t c0, c1, c2;
    xcb_get_property_reply_t *reply;

    /* Send the GetProperty requests which will be processed later */
    c0 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_DESKTOP, XCB_GET_PROPERTY_TYPE_ANY, 0, 1);

    c1 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_STATE, ATOM, 0, UINT32_MAX);

    c2 = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                    _NET_WM_WINDOW_TYPE, ATOM, 0, UINT32_MAX);

    reply = xcb_get_property_reply(globalconf.connection, c0, NULL);
    if(reply && reply->value_len && (data = xcb_get_property_value(reply)))
    {
        tag_array_t *tags = &c->screen->tags;

        desktop = *(uint32_t *) data;
        if(desktop == -1)
            c->sticky = true;
        else
            for(int i = 0; i < tags->len; i++)
                if(desktop == i)
                {
                    luaA_object_push(globalconf.L, tags->tab[i]);
                    tag_client(c);
                }
                else
                    untag_client(c, tags->tab[i]);
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
ewmh_process_client_strut(client_t *c, xcb_get_property_reply_t *strut_r)
{
    void *data;
    xcb_get_property_reply_t *mstrut_r = NULL;

    if(!strut_r)
    {
        xcb_get_property_cookie_t strut_q = xcb_get_property_unchecked(globalconf.connection, false, c->window,
                                                                       _NET_WM_STRUT_PARTIAL, CARDINAL, 0, 12);
        strut_r = mstrut_r = xcb_get_property_reply(globalconf.connection, strut_q, NULL);
    }

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

            hook_property(c, "struts");
            luaA_object_push(globalconf.L, c);
            luaA_object_emit_signal(globalconf.L, -1, "property::struts", 0);
            lua_pop(globalconf.L, 1);
        }
    }

    p_delete(&mstrut_r);
}

/** Send request to get NET_WM_ICON (EWMH)
 * \param w The window.
 * \return The cookie associated with the request.
 */
xcb_get_property_cookie_t
ewmh_window_icon_get_unchecked(xcb_window_t w)
{
  return xcb_get_property_unchecked(globalconf.connection, false, w,
                                    _NET_WM_ICON, CARDINAL, 0, UINT32_MAX);
}

int
ewmh_window_icon_from_reply(xcb_get_property_reply_t *r)
{
    uint32_t *data;
    uint64_t len;

    if(!r || r->type != CARDINAL || r->format != 32 || r->length < 2)
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

    return image_new_from_argb32(data[0], data[1], data + 2);
}

/** Get NET_WM_ICON.
 * \param cookie The cookie.
 * \return The number of elements on stack.
 */
int
ewmh_window_icon_get_reply(xcb_get_property_cookie_t cookie)
{
    xcb_get_property_reply_t *r = xcb_get_property_reply(globalconf.connection, cookie, NULL);
    int ret = ewmh_window_icon_from_reply(r);
    p_delete(&r);
    return ret;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
