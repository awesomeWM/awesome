/*
 * button.c - button managing
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

#include "button.h"

#include "common/tokenize.h"

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with modifiers keys, or a button to clone.
 * \lparam A mouse button number, or 0 to match any button.
 * \lparam A function to execute on click events.
 * \lparam A function to execute on release events.
 * \lreturn A mouse button binding.
 */
static int
luaA_button_new(lua_State *L)
{
    xcb_button_t xbutton;
    button_t *button;

    lua_settop(L, 5);

    luaA_checktable(L, 2);
    /* arg 3 is mouse button */
    xbutton = luaL_checknumber(L, 3);

    /* arg 4 and 5 are callback functions, check they are functions... */
    if(!lua_isnil(L, 4))
        luaA_checkfunction(L, 4);
    if(!lua_isnil(L, 5))
        luaA_checkfunction(L, 5);

    button = button_new(L);

    button->press = luaA_object_ref_item(L, -1, 4);
    button->release = luaA_object_ref_item(L, -1, 4);
    button->button = xbutton;
    button->mod = luaA_tomodifiers(L, 2);

    return 1;
}

/** Button object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield press The function called when button press event is received.
 * \lfield release The function called when button release event is received.
 * \lfield button The mouse button number, or 0 for any button.
 * \lfield modifiers The modifier key table that should be pressed while the
 * button is pressed.
 */
static int
luaA_button_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    button_t *button = luaL_checkudata(L, 1, "button");
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_PRESS:
        return luaA_object_push_item(L, 1, button->press);
      case A_TK_RELEASE:
        return luaA_object_push_item(L, 1, button->release);
      case A_TK_BUTTON:
        lua_pushnumber(L, button->button);
        break;
      case A_TK_MODIFIERS:
        luaA_pushmodifiers(L, button->mod);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Button object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 */
static int
luaA_button_newindex(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    button_t *button = luaL_checkudata(L, 1, "button");
    const char *attr = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(attr, len))
    {
      case A_TK_PRESS:
        luaA_checkfunction(L, 3);
        luaA_object_unref_item(L, 1, button->press);
        button->press = luaA_object_ref_item(L, 1, 3);
        break;
      case A_TK_RELEASE:
        luaA_checkfunction(L, 3);
        luaA_object_unref_item(L, 1, button->release);
        button->release = luaA_object_ref_item(L, 1, 3);
        break;
      case A_TK_BUTTON:
        button->button = luaL_checknumber(L, 3);
        break;
      case A_TK_MODIFIERS:
        button->mod = luaA_tomodifiers(L, 3);
        break;
      default:
        break;
    }

    return 0;
}

const struct luaL_reg awesome_button_methods[] =
{
    LUA_CLASS_METHODS(button)
    { "__call", luaA_button_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_button_meta[] =
{
    LUA_OBJECT_META(button)
    { "__index", luaA_button_index },
    { "__newindex", luaA_button_newindex },
    { "__gc", luaA_object_gc },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
