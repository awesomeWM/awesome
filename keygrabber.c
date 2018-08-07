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

/*
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @module keygrabber
 */

#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

#include "keygrabber.h"
#include "globalconf.h"

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

/** Returns, whether the \0-terminated char in UTF8 is control char.
 * Control characters are either characters without UTF8 representation like XF86MonBrightnessUp
 * or backspace and the other characters in ASCII table before space
 *
 * \param buf input buffer
 * \return True if the input buffer is control character.
 */
static bool
is_control(char *buf)
{
    return (buf[0] >= 0 && buf[0] < 0x20) || buf[0] == 0x7f;
}

/** Handle keypress event.
 * \param L Lua stack to push the key pressed.
 * \param e Received XKeyEvent.
 * \return True if a key was successfully retrieved, false otherwise.
 */
bool
keygrabber_handlekpress(lua_State *L, xcb_key_press_event_t *e)
{
    /* convert keysym to string */
    char buf[MAX(MB_LEN_MAX, 32)];

    /* snprintf-like return value could be used here, but that should not be
     * necessary, as we have buffer big enough */
    xkb_state_key_get_utf8(globalconf.xkb_state, e->detail, buf, countof(buf) );

    if (is_control(buf))
    {
        /* Use text names for control characters, ignoring all modifiers. */
        xcb_keysym_t keysym = xcb_key_symbols_get_keysym(globalconf.keysyms,
                                                         e->detail, 0);
        xkb_keysym_get_name(keysym, buf, countof(buf));
    }

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

/* Grab keyboard input and read pressed keys, calling a callback function at
 * each keypress, until `keygrabber.stop` is called.
 * The callback function receives three arguments:
 *
 * @param callback A callback function as described above.
 * @deprecated keygrabber.run
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
 * @deprecated keygrabber.stop
 */
int
luaA_keygrabber_stop(lua_State *L)
{
    xcb_ungrab_keyboard(globalconf.connection, XCB_CURRENT_TIME);
    luaA_unregister(L, &globalconf.keygrabber);
    return 0;
}

/** Check if keygrabber is running.
 * @deprecated keygrabber.isrunning
 * @treturn bool A boolean value, true if keygrabber is running, false otherwise.
 * @see keygrabber.is_running
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
