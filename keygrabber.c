/*
 * keygrabber.c - key grabbing
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

/** awesome keygrabber API
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @release @AWESOME_VERSION@
 * @module keygrabber
 */

#include <unistd.h>

#include "keygrabber.h"
#include "globalconf.h"
#include "keyresolv.h"

/** Grab the keyboard.
 * \return True if keyboard was grabbed.
 */
static bool
keygrabber_grab(void)
{
    int i;
    xcb_grab_keyboard_reply_t *xgb;

    for(i = 1000; i; i--)
    {
        if((xgb = xcb_grab_keyboard_reply(globalconf.connection,
                                          xcb_grab_keyboard(globalconf.connection, true,
                                                            globalconf.screen->root,
                                                            XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC,
                                                            XCB_GRAB_MODE_ASYNC),
                                          NULL)))
        {
            p_delete(&xgb);
            return true;
        }
        usleep(1000);
    }
    return false;
}

/** Handle keypress event.
 * \param L Lua stack to push the key pressed.
 * \param e Received XKeyEvent.
 * \return True if a key was successfully get, false otherwise.
 */
bool
keygrabber_handlekpress(lua_State *L, xcb_key_press_event_t *e)
{
    /* transfer event (keycode + modifiers) to keysym */
    xcb_keysym_t ksym = keyresolv_get_keysym(e->detail, e->state);

    /* convert keysym to string */
    char buf[MAX(MB_LEN_MAX, 32)];
    if(!keyresolv_keysym_to_string(ksym, buf, countof(buf)))
        return false;

    luaA_pushmodifiers(L, e->state);

    lua_pushstring(L, buf);

    switch(e->response_type)
    {
      case XCB_KEY_PRESS:
        lua_pushliteral(L, "press");
        break;
      case XCB_KEY_RELEASE:
        lua_pushliteral(L, "release");
        break;
    }

    return true;
}

/** Grab keyboard input and read pressed keys, calling a callback function at
 * each keypress, until `keygrabber.stop` is called.
 * The callback function receives three arguments:
 *
 * * a table containing modifiers keys
 * * a string with the pressed key
 * * a string with either "press" or "release" to indicate the event type.
 *
 * @param callback A callback function as described above.
 * @function run
 * @usage The following function can be bound to a key, and will be used to
 *        resize a client using keyboard.
 *
 *     function resize(c)
 *       keygrabber.run(function(mod, key, event)
 *         if event == "release" then return end
 *
 *         if     key == 'Up'   then awful.client.moveresize(0, 0, 0, 5, c)
 *         elseif key == 'Down' then awful.client.moveresize(0, 0, 0, -5, c)
 *         elseif key == 'Right' then awful.client.moveresize(0, 0, 5, 0, c)
 *         elseif key == 'Left'  then awful.client.moveresize(0, 0, -5, 0, c)
 *         else   keygrabber.stop()
 *         end
 *       end)
 *     end
 */
static int
luaA_keygrabber_run(lua_State *L)
{
    if(globalconf.keygrabber != LUA_REFNIL)
        luaL_error(L, "keygrabber already running");

    luaA_registerfct(L, 1, &globalconf.keygrabber);

    if(!keygrabber_grab())
    {
        luaA_unregister(L, &globalconf.keygrabber);
        luaL_error(L, "unable to grab keyboard");
    }

    return 0;
}

/** Stop grabbing the keyboard.
 * @function stop
 */
int
luaA_keygrabber_stop(lua_State *L)
{
    xcb_ungrab_keyboard(globalconf.connection, XCB_CURRENT_TIME);
    luaA_unregister(L, &globalconf.keygrabber);
    return 0;
}

/** Check if keygrabber is running.
 * @function isrunning
 * @treturn bool A boolean value, true if keygrabber is running, false otherwise.
 */
static int
luaA_keygrabber_isrunning(lua_State *L)
{
    lua_pushboolean(L, globalconf.keygrabber != LUA_REFNIL);
    return 1;
}

const struct luaL_Reg awesome_keygrabber_lib[] =
{
    { "run", luaA_keygrabber_run },
    { "stop", luaA_keygrabber_stop },
    { "isrunning", luaA_keygrabber_isrunning },
    { "__index", luaA_default_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
