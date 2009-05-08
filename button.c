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

DO_LUA_TOSTRING(button_t, button, "button")
LUA_OBJECT_FUNCS(button_t, button, "button")

void
button_unref_simplified(button_t **b)
{
    button_unref(globalconf.L, *b);
}

/** Collect a button.
 * \param L The Lua VM state.
 * \return 0.
 */
static int
luaA_button_gc(lua_State *L)
{
    button_t *button = luaL_checkudata(L, 1, "button");
    luaA_ref_array_wipe(&button->refs);
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, button->press);
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, button->release);
    return 0;
}

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
    luaA_ref press = LUA_REFNIL, release = LUA_REFNIL;

    luaA_checktable(L, 2);
    /* arg 3 is mouse button */
    xbutton = luaL_checknumber(L, 3);

    /* arg 4 and 5 are callback functions, check they are functions... */
    if(!lua_isnil(L, 4))
        luaA_checkfunction(L, 4);
    if(lua_gettop(L) == 5 && !lua_isnil(L, 5))
        luaA_checkfunction(L, 5);

    /* ... then register (can't register before since 5 maybe not nil but not a
     * function */
    if(!lua_isnil(L, 4))
        luaA_registerfct(L, 4, &press);
    if(lua_gettop(L) == 5 && !lua_isnil(L, 5))
        luaA_registerfct(L, 5, &release);

    button = button_new(L);
    button->press = press;
    button->release = release;
    button->button = xbutton;

    luaA_setmodifiers(L, 2, &button->mod);

    return 1;
}

/** Set a button array with a Lua table.
 * \param L The Lua VM state.
 * \param idx The index of the Lua table.
 * \param buttons The array button to fill.
 */
void
luaA_button_array_set(lua_State *L, int idx, button_array_t *buttons)
{
    luaA_checktable(L, idx);
    button_array_wipe(buttons);
    button_array_init(buttons);
    lua_pushnil(L);
    while(lua_next(L, idx))
        button_array_append(buttons, button_ref(L));
}

/** Push an array of button as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param buttons The button array to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_button_array_get(lua_State *L, button_array_t *buttons)
{
    lua_createtable(L, buttons->len, 0);
    for(int i = 0; i < buttons->len; i++)
    {
        button_push(L, buttons->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
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
        if(button->press != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, button->press);
        else
            lua_pushnil(L);
        break;
      case A_TK_RELEASE:
        if(button->release != LUA_REFNIL)
            lua_rawgeti(L, LUA_REGISTRYINDEX, button->release);
        else
            lua_pushnil(L);
        break;
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
        luaA_registerfct(L, 3, &button->press);
        break;
      case A_TK_RELEASE:
        luaA_registerfct(L, 3, &button->release);
        break;
      case A_TK_BUTTON:
        button->button = luaL_checknumber(L, 3);
        break;
      case A_TK_MODIFIERS:
        luaA_setmodifiers(L, 3, &button->mod);
      default:
        break;
    }

    return 0;
}

const struct luaL_reg awesome_button_methods[] =
{
    { "__call", luaA_button_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_button_meta[] =
{
    { "__index", luaA_button_index },
    { "__newindex", luaA_button_newindex },
    { "__gc", luaA_button_gc },
    { "__tostring", luaA_button_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
