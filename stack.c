/*
 * stack.c - client stack management
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

#include "stack.h"
#include "ewmh.h"

void
stack_client_remove(lua_State *L, client_t *c, bool silent, const char *context)
{
    foreach(client, globalconf.stack)
        if(*client == c)
        {
            client_array_remove(&globalconf.stack, client);
            break;
        }
    ewmh_update_net_client_list_stacking();

    if (!silent)
        stack_windows(L, context, c, NULL);
}

/** Push the client at the beginning of the client stack.
 * \param L The Lua context.
 * \param c The client to push.
 * \param context An human readable reason of why this was done.
 */
void
stack_client_push(lua_State *L, client_t *c, const char *context)
{
    stack_client_remove(L, c, true, "");
    client_array_push(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking();
    stack_windows(L, context, c, NULL);
}

/** Push the client at the end of the client stack.
 * \param L The Lua context.
 * \param c The client to push.
 * \param context An human readable reason of why this was done.
 */
void
stack_client_append(lua_State *L, client_t *c, const char *context)
{
    stack_client_remove(L, c, true, "");
    client_array_append(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking();
    stack_windows(L, context, c, NULL);
}

void
stack_windows(lua_State *L, const char *context, client_t *c, drawin_t *d)
{
    /* Context */
    lua_pushstring(L, context);

    /* Create hints table */
    lua_newtable(L);

    lua_pushstring(L, "client");
    if (c)
        luaA_object_push(L, c);
    else
        lua_pushnil(L);

    lua_settable(L, -3);

    lua_pushstring(L, "drawin");
    if (d)
        luaA_object_push(L, d);
    else
        lua_pushnil(L);

    lua_settable(L, -3);

    luaA_class_emit_signal(L, &client_class, "request::restack", 2);
}

/** Stack a window above another window, without causing errors.
 * \param w The window.
 * \param previous The window which should be below this window.
 */
static void
stack_window_above(xcb_window_t w, xcb_window_t previous)
{
    if (previous == XCB_NONE)
        /* This would cause an error from the X server. Also, if we really
         * changed the stacking order of all windows, they'd all have to redraw
         * themselves. Doing it like this is better. */
        return;

    xcb_configure_window(globalconf.connection, w,
                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                         (uint32_t[]) { previous, XCB_STACK_MODE_ABOVE });
}

/** Stack a client above.
 * \param c The client.
 * \param previous The previous client on the stack.
 * \return The next-previous!
 */
static xcb_window_t
stack_client_above(client_t *c, xcb_window_t previous)
{
    stack_window_above(c->frame_window, previous);

    previous = c->frame_window;

    /* stack transient window on top of their parents */
    foreach(node, globalconf.stack)
        if((*node)->transient_for == c)
            previous = stack_client_above(*node, previous);

    return previous;
}

/**
 * Allow Lua to define the stacking order of clients and wiboxes.
 *
 * The table must contain `client` and `wibox` object. Index `1` is the closest
 * to the root (wallpaper) and the last index is the closest to the top.
 *
 * @staticfct root.set_stacking_order
 * @tparam table stacking_order
 */
int
luaA_set_stacking_order(lua_State *L) {
    xcb_window_t next = XCB_NONE;

    if(lua_gettop(L) == 1)
    {
        luaA_checktable(L, 1);

        lua_pushnil(L);

        while(lua_next(L, 1))
        {
            if (luaA_class_get(L, -1) == &client_class)
            {
                client_t *c = luaA_object_ref_class(L, -1, &client_class);
                next = stack_client_above(c, next);
                luaA_object_unref(L, c);
            }
            else if (luaA_class_get(L, -1) == &drawin_class)
            {
                drawin_t *d = luaA_object_ref_class(L, -1, &drawin_class);
                stack_window_above(d->window, next);
                next = d->window;
                luaA_object_unref(L, d);
            }
            else
                return luaL_error(L, "set_stacking_order only works on clients and drawins");
        }

        lua_pop(L, 1);
    }

    return 0;
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
