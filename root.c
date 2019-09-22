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

/** awesome root window API
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @coreclassmod root
 */

#include <stdbool.h>

#include "root.h"
#include "globalconf.h"

#include "common/atoms.h"
#include "common/xcursor.h"
#include "common/xutil.h"
#include "objects/button.h"
#include "keygrabber.h"
#include "math.h"

#include <xcb/xtest.h>

struct root_impl root_impl;

static xcb_keycode_t
_string_to_key_code(const char *s)
{
    xcb_keysym_t keysym;
    xcb_keycode_t *keycodes;

    keysym   = XStringToKeysym(s);
    keycodes = xcb_key_symbols_get_keycode(globalconf.keysyms, keysym);

    if(keycodes) {
        return keycodes[0]; /* XXX only returning the first is probably not
                             * the best */
    } else {
        return 0;
    }
}

/** Send fake keyboard or mouse events.
 *
 * Usually the currently focused client or the keybindings will receive those
 * events. If a `keygrabber` or `mousegrabber` is running, then it will get them.
 *
 * Some keys have different names compared to the ones generally used in
 * Awesome. For example, Awesome uses "modifier keys" for keybindings using
 * their X11 names such as "Control" or "Mod1" (for "Alt"). These are not the
 * name of the key but is only the name of the modifier they represent. Some
 * modifiers are even present twice on some keyboard like the left and right
 * "Shift". Here is a list of the "real" key names matching the modifiers in
 * `fake_input`:
 *
 * <table class='widget_list' border=1>
 *  <tr style='font-weight: bold;'>
 *   <th align='center'>Modifier name </th>
 *   <th align='center'>Key name</th>
 *   <th align='center'>Other key name</th>
 *  </tr>
 *  <tr><td> Mod4</td><td align='center'> Super_L </td><td align='center'> Super_R </td></tr>
 *  <tr><td> Control </td><td align='center'> Control_L </td><td align='center'> Control_R </td></tr>
 *  <tr><td> Shift </td><td align='center'> Shift_L </td><td align='center'> Shift_R </td></tr>
 *  <tr><td> Mod1</td><td align='center'> Alt_L </td><td align='center'> Alt_R </td></tr>
 * </table>
 *
 * Note that this is valid for most of the modern "western" keyboard layouts.
 * Some older, custom or foreign layouts may break this convention.
 *
 * This function is very low level, to be more useful, it can be wrapped into
 * higher level constructs such as:
 *
 * **Sending strings:**
 *
 * @DOC_text_root_fake_string_EXAMPLE@
 *
 * Note that this example works for most ASCII inputs but may fail depending on
 * how the string is encoded. Some multi-byte characters may not represent
 * keys and some UTF-8 encoding format create characters by combining multiple
 * elements such as accent + base character or various escape sequences. If you
 * wish to use this example for "real world" i18n use cases, learning about
 * XKB event and UTF-8 encoding is a prerequisites.
 *
 * **Clicking:**
 *
 * ![Client geometry](../images/mouse.svg)
 *
 * @DOC_text_root_fake_click_EXAMPLE@
 *
 * @param event_type The event type: key\_press, key\_release, button\_press,
 *  button\_release or motion\_notify.
 * @param detail The detail: in case of a key event, this is the keycode
 *  to send, in case of a button event this is the number of the button. In
 *  case of a motion event, this is a boolean value which if true makes the
 *  coordinates relatives.
 * @param x In case of a motion event, this is the X coordinate.
 * @param y In case of a motion event, this is the Y coordinate.
 * @staticfct fake_input
 */
static int
luaA_root_fake_input(lua_State *L)
{
    if(!globalconf.have_xtest)
    {
        luaA_warn(L, "XTest extension is not available, cannot fake input.");
        return 0;
    }

    const char *stype = luaL_checkstring(L, 1);
    uint8_t type, detail;
    int x = 0, y = 0;

    if (A_STREQ(stype, "key_press"))
    {
        type = XCB_KEY_PRESS;
        if(lua_type(L, 2) == LUA_TSTRING) {
            detail = _string_to_key_code(lua_tostring(L, 2)); /* keysym */
        } else {
            detail = luaL_checkinteger(L, 2); /* keycode */
        }
    }
    else if(A_STREQ(stype, "key_release"))
    {
        type = XCB_KEY_RELEASE;
        if(lua_type(L, 2) == LUA_TSTRING) {
            detail = _string_to_key_code(lua_tostring(L, 2)); /* keysym */
        } else {
            detail = luaL_checkinteger(L, 2); /* keycode */
        }
    }
    else if(A_STREQ(stype, "button_press"))
    {
        type = XCB_BUTTON_PRESS;
        detail = luaL_checkinteger(L, 2); /* button number */
    }
    else if(A_STREQ(stype, "button_release"))
    {
        type = XCB_BUTTON_RELEASE;
        detail = luaL_checkinteger(L, 2); /* button number */
    }
    else if(A_STREQ(stype, "motion_notify"))
    {
        type = XCB_MOTION_NOTIFY;
        detail = luaA_checkboolean(L, 2); /* relative to the current position or not */
        x = round(luaA_checknumber_range(L, 3, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
        y = round(luaA_checknumber_range(L, 4, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
    }
    else
        return 0;

    xcb_test_fake_input(globalconf.connection,
                        type,
                        detail,
                        0, /* This is a delay, not a timestamp! */
                        XCB_NONE,
                        x, y,
                        0);
    return 0;
}

/** Get or set global key bindings.
 * These bindings will be available when you press keys on the root window.
 *
 * @tparam table|nil keys_array An array of key binding objects, or nothing.
 * @return The array of key bindings objects of this client.
 * @staticfct keys
 */
static int
luaA_root_keys(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        luaA_checktable(L, 1);

        foreach(key, globalconf.keys)
            luaA_object_unref(L, *key);

        key_array_wipe(&globalconf.keys);
        key_array_init(&globalconf.keys);

        lua_pushnil(L);
        while(lua_next(L, 1))
            key_array_append(&globalconf.keys, luaA_object_ref_class(L, -1, &key_class));

        root_impl.grab_keys();

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
 * This binding will be available when you click on the root window.
 *
 * @param button_table An array of mouse button bindings objects, or nothing.
 * @return The array of mouse button bindings objects.
 * @staticfct buttons
 */
static int
luaA_root_buttons(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        luaA_checktable(L, 1);

        foreach(button, globalconf.buttons)
            luaA_object_unref(L, *button);

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

/** Set the root cursor
 *
 * The possible values are:
 *
 *@DOC_cursor_c_COMMON@
 *
 * @param cursor_name A X cursor name.
 * @staticfct cursor
 */
static int
luaA_root_cursor(lua_State *L)
{
    const char *cursor_name = luaL_checkstring(L, 1);
    uint16_t cursor_font = xcursor_font_fromstr(cursor_name);

    if(cursor_font)
    {
        uint32_t change_win_vals[] = { xcursor_new(globalconf.cursor_ctx, cursor_font) };

        xcb_change_window_attributes(globalconf.connection,
                                     globalconf.screen->root,
                                     XCB_CW_CURSOR,
                                     change_win_vals);
    }
    else
        luaA_warn(L, "invalid cursor %s", cursor_name);

    return 0;
}

/** Get the drawins attached to a screen.
 *
 * @return A table with all drawins.
 * @staticfct drawins
 */
static int
luaA_root_drawins(lua_State *L)
{
    lua_createtable(L, globalconf.drawins.len, 0);

    for(int i = 0; i < globalconf.drawins.len; i++)
    {
        luaA_object_push(L, globalconf.drawins.tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

/** Get the wallpaper as a cairo surface or set it as a cairo pattern.
 *
 * @param pattern A cairo pattern as light userdata
 * @return A cairo surface or nothing.
 * @staticfct wallpaper
 */
static int
luaA_root_wallpaper(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        cairo_pattern_t *pattern = (cairo_pattern_t *)lua_touserdata(L, -1);
        lua_pushboolean(L, root_impl.set_wallpaper(pattern));
        /* Don't return the wallpaper, it's too easy to get memleaks */
        return 1;
    }

    if(globalconf.wallpaper == NULL)
        return 0;

    /* lua has to make sure this surface gets destroyed */
    lua_pushlightuserdata(L, cairo_surface_reference(globalconf.wallpaper));
    return 1;
}

/** Get the size of the root window.
 *
 * @return Width of the root window.
 * @return height of the root window.
 * @staticfct size
 */
static int
luaA_root_size(lua_State *L)
{
    lua_pushinteger(L, globalconf.screen->width_in_pixels);
    lua_pushinteger(L, globalconf.screen->height_in_pixels);
    return 2;
}

/** Get the physical size of the root window, in millimeter.
 *
 * @return Width of the root window, in millimeters.
 * @return height of the root window, in millimeters.
 * @staticfct size_mm
 */
static int
luaA_root_size_mm(lua_State *L)
{
    lua_pushinteger(L, globalconf.screen->width_in_millimeters);
    lua_pushinteger(L, globalconf.screen->height_in_millimeters);
    return 2;
}

/** Get the attached tags.
 * @return A table with all tags.
 * @staticfct tags
 */
static int
luaA_root_tags(lua_State *L)
{
    lua_createtable(L, globalconf.tags.len, 0);
    for(int i = 0; i < globalconf.tags.len; i++)
    {
        luaA_object_push(L, globalconf.tags.tab[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static bool
event_key_match(xcb_keycode_t keycode, uint16_t mods, keyb_t *k, xcb_keysym_t keysym)
{
    return (((k->keycode && keycode == k->keycode)
                    || (k->keysym && keysym == k->keysym))
            && (k->modifiers == XCB_BUTTON_MASK_ANY || k->modifiers == mods));
}

static void emit_key_signals(key_array_t *arr, bool pressed,
        xcb_keycode_t keycode, uint16_t mods,
        int oud, int nargs, xcb_keysym_t keysym)
{
    lua_State *L = globalconf_get_lua_State();

    int abs_oud = oud < 0 ? ((lua_gettop(L) + 1) + oud) : oud;
    int item_matching = 0;
    foreach(item, *arr)
    {
        if (event_key_match(keycode, mods, *item, keysym))
        {
            if (oud) luaA_object_push_item(L, abs_oud, *item);
            else luaA_object_push(L, *item);
            item_matching++;
        }
    }
    for(; item_matching > 0; item_matching--)
    {
        const char *event;
        if (pressed) event = "press";
        else event = "release";
        for(int i = 0; i < nargs; i++)
        {
            lua_pushvalue(L, - nargs - item_matching);
        }
        luaA_object_emit_signal(L, - nargs - 1, event, nargs);
        lua_pop(L, 1);
    }
    lua_pop(L, nargs);
}

void root_handle_key(key_array_t *keys, bool pushed_to_stack,
        uint32_t timestamp, uint32_t keycode, uint16_t state,
        bool pressed, xcb_keysym_t keysym, struct root_impl *root)
{
    lua_State *L = globalconf_get_lua_State();
    globalconf.timestamp = timestamp;

    if(globalconf.keygrabber != LUA_REFNIL)
    {
        if(keygrabber_handlekpress(keycode, pressed, state))
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, globalconf.keygrabber);

            if(!luaA_dofunction(L, 3, 0))
            {
                warn("Stopping keygrabber.");
                luaA_keygrabber_stop(L);
            }
        }
        if (pushed_to_stack)
        {
            lua_pop(L, pushed_to_stack);
        }
    }
    else
    {
        emit_key_signals(keys, pressed, keycode, state,
                -pushed_to_stack, pushed_to_stack, keysym);
    }
}

const struct luaL_Reg awesome_root_lib[] =
{
    { "buttons", luaA_root_buttons },
    { "keys", luaA_root_keys },
    { "cursor", luaA_root_cursor },
    { "fake_input", luaA_root_fake_input },
    { "drawins", luaA_root_drawins },
    { "wallpaper", luaA_root_wallpaper },
    { "size", luaA_root_size },
    { "size_mm", luaA_root_size_mm },
    { "tags", luaA_root_tags },
    { "__index", luaA_default_index },
    { "__newindex", luaA_default_newindex },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
