/*
 * mousegrabber.c - mouse pointer grabbing
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

/** awesome mousegrabber API
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @coreclassmod mousegrabber
 */

#include "mousegrabber.h"
#include "common/xcursor.h"
#include "mouse.h"
#include "globalconf.h"

#include <unistd.h>
#include <stdbool.h>

struct mousegrabber_impl mousegrabber_impl;

/** Handle mouse motion events.
 * \param L Lua stack to push the pointer motion.
 * \param x The received mouse event x component.
 * \param y The received mouse event y component.
 * \param mask The received mouse event bit mask.
 * \return True if the event was handled.
 */
bool
mousegrabber_handleevent(lua_State *L, int x, int y, uint16_t mask)
{
    if(globalconf.mousegrabber != LUA_REFNIL)
    {
        luaA_mouse_pushstatus(L, x, y, mask);
        lua_rawgeti(L, LUA_REGISTRYINDEX, globalconf.mousegrabber);
        if(!luaA_dofunction(L, 1, 1))
        {
            warn("Stopping mousegrabber.");
            luaA_mousegrabber_stop(L);
        }
        else
        {
            if(!lua_isboolean(L, -1) || !lua_toboolean(L, -1))
            {
                luaA_mousegrabber_stop(L);
            }
            lua_pop(L, 1);  /* pop returned value */
        }
        return true;
    }
    return false;
}

/** Grab the mouse pointer and list motions, calling callback function at each
 * motion. The callback function must return a boolean value: true to
 * continue grabbing, false to stop.
 * The function is called with one argument:
 * a table containing modifiers pointer coordinates.
 *
 * The list of valid cursors are:
 *
 *@DOC_cursor_c_COMMON@
 *
 *
 * @param func A callback function as described above.
 * @param cursor The name of a X cursor to use while grabbing.
 * @staticfct run
 */
static int
luaA_mousegrabber_run(lua_State *L)
{
    if(globalconf.mousegrabber != LUA_REFNIL)
        luaL_error(L, "mousegrabber already running");

    luaA_registerfct(L, 1, &globalconf.mousegrabber);

    const char *cursor_name = luaL_checkstring(L, 2);
    bool mouse_grabbed = mousegrabber_impl.grab_mouse(cursor_name);

    if(!mouse_grabbed)
    {
        luaA_unregister(L, &globalconf.mousegrabber);
        luaL_error(L, "unable to grab mouse pointer");
    }

    return 0;
}

/** Stop grabbing the mouse pointer.
 *
 * @staticfct stop
 */
int
luaA_mousegrabber_stop(lua_State *L)
{
    mousegrabber_impl.release_mouse();

    luaA_unregister(L, &globalconf.mousegrabber);

    return 0;
}

/** Check if mousegrabber is running.
 *
 * @treturn boolean True if running, false otherwise.
 * @staticfct isrunning
 */
static int
luaA_mousegrabber_isrunning(lua_State *L)
{
    lua_pushboolean(L, globalconf.mousegrabber != LUA_REFNIL);
    return 1;
}

const struct luaL_Reg awesome_mousegrabber_lib[] =
{
    { "run", luaA_mousegrabber_run },
    { "stop", luaA_mousegrabber_stop },
    { "isrunning", luaA_mousegrabber_isrunning },
    { "__index", luaA_default_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
