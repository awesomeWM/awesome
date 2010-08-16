/*
 * root.c - root window management
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

#include <xcb/xtest.h>

#include "globalconf.h"
#include "objects/button.h"
#include "objects/wibox.h"
#include "luaa.h"
#include "xwindow.h"
#include "common/xcursor.h"
#include "common/tokenize.h"
#include "common/xutil.h"

/** Send fake events. Usually the current focused client will get it.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lparam The event type: key_press, key_release, button_press, button_release
 * or motion_notify.
 * \lparam The detail: in case of a key event, this is the keycode to send, in
 * case of a button event this is the number of the button. In case of a motion
 * event, this is a boolean value which if true make the coordinates relatives.
 * \lparam In case of a motion event, this is the X coordinate.
 * \lparam In case of a motion event, this is the Y coordinate.
 * If not specified, the current one is used.
 */
static int
luaA_root_fake_input(lua_State *L)
{
    if(!globalconf.have_xtest)
    {
        luaA_warn(L, "XTest extension is not available, cannot fake input.");
        return 0;
    }

    size_t tlen;
    const char *stype = luaL_checklstring(L, 1, &tlen);
    uint8_t type, detail;
    int x = 0, y = 0;

    switch(a_tokenize(stype, tlen))
    {
      case A_TK_KEY_PRESS:
        type = XCB_KEY_PRESS;
        detail = luaL_checknumber(L, 2); /* keycode */
        break;
      case A_TK_KEY_RELEASE:
        type = XCB_KEY_RELEASE;
        detail = luaL_checknumber(L, 2); /* keycode */
        break;
      case A_TK_BUTTON_PRESS:
        type = XCB_BUTTON_PRESS;
        detail = luaL_checknumber(L, 2); /* button number */
        break;
      case A_TK_BUTTON_RELEASE:
        type = XCB_BUTTON_RELEASE;
        detail = luaL_checknumber(L, 2); /* button number */
        break;
      case A_TK_MOTION_NOTIFY:
        type = XCB_MOTION_NOTIFY;
        detail = luaA_checkboolean(L, 2); /* relative to the current position or not */
        x = luaL_checknumber(L, 3);
        y = luaL_checknumber(L, 4);
        break;
      default:
        return 0;
    }

    xcb_test_fake_input(globalconf.connection,
                        type,
                        detail,
                        XCB_CURRENT_TIME,
                        XCB_NONE,
                        x, y,
                        0);
    return 0;
}

/** Get or set global key bindings.
 * This binding will be available when you'll press keys on root window.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lparam An array of key bindings objects, or nothing.
 * \lreturn The array of key bindings objects of this client.
 */
static int
luaA_root_keys(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        luaA_checktable(L, 1);

        foreach(key, globalconf.keys)
            luaA_object_unref(globalconf.L, *key);

        key_array_wipe(&globalconf.keys);
        key_array_init(&globalconf.keys);

        lua_pushnil(L);
        while(lua_next(L, 1))
            key_array_append(&globalconf.keys, luaA_object_ref_class(L, -1, &key_class));

        xcb_screen_t *s = globalconf.screen;
        xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, s->root, XCB_BUTTON_MASK_ANY);
        xwindow_grabkeys(s->root, &globalconf.keys);

        return 1;
    }

    lua_createtable(L, globalconf.keys.len, 0);
    for(int i = 0; i < globalconf.keys.len; i++)
    {
        luaA_object_push(L, globalconf.keys.tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/** Get or set global mouse bindings.
 * This binding will be available when you'll click on root window.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lparam An array of mouse button bindings objects, or nothing.
 * \lreturn The array of mouse button bindings objects.
 */
static int
luaA_root_buttons(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        luaA_checktable(L, 1);

        foreach(button, globalconf.buttons)
            luaA_object_unref(globalconf.L, *button);

        button_array_wipe(&globalconf.buttons);
        button_array_init(&globalconf.buttons);

        lua_pushnil(L);
        while(lua_next(L, 1))
            button_array_append(&globalconf.buttons, luaA_object_ref(L, -1));

        return 1;
    }

    lua_createtable(L, globalconf.buttons.len, 0);
    for(int i = 0; i < globalconf.buttons.len; i++)
    {
        luaA_object_push(L, globalconf.buttons.tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/** Set the root cursor.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lparam A X cursor name.
 */
static int
luaA_root_cursor(lua_State *L)
{
    const char *cursor_name = luaL_checkstring(L, 1);
    uint16_t cursor_font = xcursor_font_fromstr(cursor_name);

    if(cursor_font)
    {
        uint32_t change_win_vals[] = { xcursor_new(globalconf.connection, cursor_font) };

        xcb_change_window_attributes(globalconf.connection,
                                     globalconf.screen->root,
                                     XCB_CW_CURSOR,
                                     change_win_vals);
    }
    else
        luaA_warn(L, "invalid cursor %s", cursor_name);

    return 0;
}

/** Get the wiboxes attached to a screen.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lreturn A table with all wiboxes.
 */
static int
luaA_root_wiboxes(lua_State *L)
{
    lua_createtable(L, globalconf.wiboxes.len, 0);

    for(int i = 0; i < globalconf.wiboxes.len; i++)
    {
        luaA_object_push(L, globalconf.wiboxes.tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

const struct luaL_reg awesome_root_lib[] =
{
    { "buttons", luaA_root_buttons },
    { "keys", luaA_root_keys },
    { "cursor", luaA_root_cursor },
    { "fake_input", luaA_root_fake_input },
    { "wiboxes", luaA_root_wiboxes },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
