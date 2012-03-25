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
#include "luaa.h"
#include "key.h"
#include "common/luaobject.h"

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_button_new(lua_State *L)
{
    xcb_button_t xbutton;
    button_t *button;

    /* compat code */
    if(lua_istable(L, 2) && lua_isnumber(L, 3))
    {
        luaA_deprecate(L, "new syntax");

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
        button->button = xbutton;
        button->modifiers = luaA_tomodifiers(L, 2);

        if(!lua_isnil(L, 4))
        {
            lua_pushvalue(L, 4);
            luaA_object_add_signal(L, -2, "press", -1);
        }

        if(!lua_isnil(L, 5))
        {
            lua_pushvalue(L, 5);
            luaA_object_add_signal(L, -2, "release", -1);
        }

        return 1;
    }

    return luaA_class_new(L, &button_class);
}

/** Set a button array with a Lua table.
 * \param L The Lua VM state.
 * \param oidx The index of the object to store items into.
 * \param idx The index of the Lua table.
 * \param buttons The array button to fill.
 */
void
luaA_button_array_set(lua_State *L, int oidx, int idx, button_array_t *buttons)
{
    luaA_checktable(L, idx);

    foreach(button, *buttons)
        luaA_object_unref_item(L, oidx, *button);

    button_array_wipe(buttons);
    button_array_init(buttons);

    lua_pushnil(L);
    while(lua_next(L, idx))
        if(luaA_toudata(L, -1, &button_class))
            button_array_append(buttons, luaA_object_ref_item(L, oidx, -1));
        else
            lua_pop(L, 1);
}

/** Push an array of button as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param oidx The index of the object to get items from.
 * \param buttons The button array to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_button_array_get(lua_State *L, int oidx, button_array_t *buttons)
{
    lua_createtable(L, buttons->len, 0);
    for(int i = 0; i < buttons->len; i++)
    {
        luaA_object_push_item(L, oidx, buttons->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

LUA_OBJECT_EXPORT_PROPERTY(button, button_t, button, lua_pushnumber);
LUA_OBJECT_EXPORT_PROPERTY(button, button_t, modifiers, luaA_pushmodifiers);

static int
luaA_button_set_modifiers(lua_State *L, button_t *b)
{
    b->modifiers = luaA_tomodifiers(L, -1);
    luaA_object_emit_signal(L, -3, "property::modifiers", 0);
    return 0;
}

static int
luaA_button_set_button(lua_State *L, button_t *b)
{
    b->button = luaL_checknumber(L, -1);
    luaA_object_emit_signal(L, -3, "property::button", 0);
    return 0;
}

void
button_class_setup(lua_State *L)
{
    static const struct luaL_reg button_methods[] =
    {
        LUA_CLASS_METHODS(button)
        { "__call", luaA_button_new },
        { NULL, NULL }
    };

    static const struct luaL_reg button_meta[] =
    {
        LUA_OBJECT_META(button)
        LUA_CLASS_META
        { "__gc", luaA_object_gc },
        { NULL, NULL }
    };

    luaA_class_setup(L, &button_class, "button", (lua_class_allocator_t) button_new,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     button_methods, button_meta);
    luaA_class_add_property(&button_class, A_TK_BUTTON,
                            (lua_class_propfunc_t) luaA_button_set_button,
                            (lua_class_propfunc_t) luaA_button_get_button,
                            (lua_class_propfunc_t) luaA_button_set_button);
    luaA_class_add_property(&button_class, A_TK_MODIFIERS,
                            (lua_class_propfunc_t) luaA_button_set_modifiers,
                            (lua_class_propfunc_t) luaA_button_get_modifiers,
                            (lua_class_propfunc_t) luaA_button_set_modifiers);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
