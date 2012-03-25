/*
 * hooks.c - Lua hooks management
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

#include "globalconf.h"
#include "luaa.h"

#define HANDLE_HOOK(L, h) \
    do { \
        if(lua_gettop(L) == 1) \
            luaA_registerfct(L, 1, &h); \
        lua_rawgeti(L, LUA_REGISTRYINDEX, h); \
        return 1; \
    } while(0)

/** Set the function called each time a client gets focus. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call each time a client gets focus.
 */
static int
luaA_hooks_focus(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.focus);
}

/** Set the function called each time a client loses focus. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call each time a client loses focus.
 */
static int
luaA_hooks_unfocus(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.unfocus);
}

/** Set the function called each time a new client appears. This function is
 * called with the client object as argument, plus a boolean argument which is
 * true if the client is managed at startup.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on each new client.
 */
static int
luaA_hooks_manage(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.manage);
}

/** Set the function called each time a client goes away. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call when a client goes away.
 */
static int
luaA_hooks_unmanage(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.unmanage);
}

/** Set the function called each time the mouse enter a window. This
 * function is called with the client object as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call each time a client gets mouse over it.
 */
static int
luaA_hooks_mouse_enter(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.mouse_enter);
}

/** Set the function called each time the mouse leave a window. This
 * function is called with the client object as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call each time a client gets mouse over it.
 */
static int
luaA_hooks_mouse_leave(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.mouse_leave);
}

/** Set the function called on each client list change.
 * This function is called without any argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on each client list change.
 */
static int
luaA_hooks_clients(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.clients);
}

/** Set the function called on each screen tag list change.
 * This function is called with a screen number as first argument,
 * the tag object as second and the action (add, remove, select or unselect)
 * as third.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on each tag list change.
 */
static int
luaA_hooks_tags(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.tags);
}

/** Set the function called on each client's tags change.
 * This function is called with the client and the tag as argument.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on each client's tags change.
 */
static int
luaA_hooks_tagged(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.tagged);
}

/** Set the function called on each client's property change.
 * This function is called with the client object as argument and the
 * property name.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on each client property update.
 */
static int
luaA_hooks_property(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.property);
}

/** Set the function to be called every N seconds.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The number of seconds to run function every. Set 0 to disable.
 * \lparam A function to call every N seconds (optional).
 */
static int
luaA_hooks_timer(lua_State *L)
{
    if(lua_gettop(L) >= 1)
    {
        globalconf.timer.repeat = luaL_checknumber(L, 1);

        if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
            luaA_registerfct(L, 2, &globalconf.hooks.timer);

        ev_timer_again(globalconf.loop, &globalconf.timer);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, globalconf.hooks.timer);

    return 1;
}

/** Set the function called on awesome exit
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A function to call on awesome exit.
 */
static int
luaA_hooks_exit(lua_State *L)
{
    HANDLE_HOOK(L, globalconf.hooks.exit);
}

const struct luaL_reg awesome_hooks_lib[] =
{
    { "focus", luaA_hooks_focus },
    { "unfocus", luaA_hooks_unfocus },
    { "manage", luaA_hooks_manage },
    { "unmanage", luaA_hooks_unmanage },
    { "mouse_enter", luaA_hooks_mouse_enter },
    { "mouse_leave", luaA_hooks_mouse_leave },
    { "property", luaA_hooks_property },
    { "clients", luaA_hooks_clients },
    { "tags", luaA_hooks_tags },
    { "tagged", luaA_hooks_tagged },
    { "timer", luaA_hooks_timer },
    { "exit", luaA_hooks_exit },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
