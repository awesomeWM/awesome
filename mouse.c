/*
 * mouse.c - mouse managing
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include <math.h>

#include "screen.h"
#include "tag.h"
#include "common/xcursor.h"
#include "common/xutil.h"

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
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, button->press);
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, button->release);
    return 0;
}

/** Get the pointer position.
 * \param window The window to get position on.
 * \param x will be set to the Pointer-x-coordinate relative to window
 * \param y will be set to the Pointer-y-coordinate relative to window
 * \param child Will be set to the window under the pointer.
 * \param mask will be set to the current buttons state
 * \return true on success, false if an error occured
 **/
bool
mouse_query_pointer(xcb_window_t window, int16_t *x, int16_t *y, xcb_window_t *child, uint16_t *mask)
{
    xcb_query_pointer_cookie_t query_ptr_c;
    xcb_query_pointer_reply_t *query_ptr_r;

    query_ptr_c = xcb_query_pointer_unchecked(globalconf.connection, window);
    query_ptr_r = xcb_query_pointer_reply(globalconf.connection, query_ptr_c, NULL);

    if(!query_ptr_r || !query_ptr_r->same_screen)
        return false;

    *x = query_ptr_r->win_x;
    *y = query_ptr_r->win_y;
    if(mask)
        *mask = query_ptr_r->mask;
    if(child)
        *child = query_ptr_r->child;

    p_delete(&query_ptr_r);

    return true;
}

/** Get the pointer position on the screen.
 * \param screen This will be set to the screen the mouse is on.
 * \param x This will be set to the Pointer-x-coordinate relative to window.
 * \param y This will be set to the Pointer-y-coordinate relative to window.
 * \param child This will be set to the window under the pointer.
 * \param mask This will be set to the current buttons state.
 * \return True on success, false if an error occured.
 */
static bool
mouse_query_pointer_root(screen_t **s, int16_t *x, int16_t *y, xcb_window_t *child, uint16_t *mask)
{
    for(int screen = 0;
        screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen++)
    {
        xcb_window_t root = xutil_screen_get(globalconf.connection, screen)->root;

        if(mouse_query_pointer(root, x, y, child, mask))
        {
            *s = &globalconf.screens.tab[screen];
            return true;
        }
    }
    return false;
}

/** Set the pointer position.
 * \param window The destination window.
 * \param x X-coordinate inside window.
 * \param y Y-coordinate inside window.
 */
static inline void
mouse_warp_pointer(xcb_window_t window, int x, int y)
{
    xcb_warp_pointer(globalconf.connection, XCB_NONE, window,
                     0, 0, 0, 0, x, y );
}

/** Create a new mouse button bindings.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with modifiers keys, or a button to clone.
 * \lparam A mouse button number.
 * \lparam A function to execute on click events.
 * \lparam A function to execute on release events.
 * \lreturn A mouse button binding.
 */
static int
luaA_button_new(lua_State *L)
{
    int i, len;
    button_t *button, *orig;
    luaA_ref press = LUA_REFNIL, release = LUA_REFNIL;

    if((orig = luaA_toudata(L, 2, "button")))
    {
        button_t *copy = button_new(L);
        copy->mod = orig->mod;
        copy->button = orig->button;
        if(orig->press != LUA_REFNIL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, orig->press);
            luaA_registerfct(L, -1, &copy->press);
            lua_pop(L, 1);
        }
        else
            copy->press = LUA_REFNIL;
        if(orig->release != LUA_REFNIL)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, orig->release);
            luaA_registerfct(L, -1, &copy->release);
            lua_pop(L, 1);
        }
        else
            copy->release = LUA_REFNIL;
        return 1;
    }

    luaA_checktable(L, 2);
    /* arg 3 is mouse button */
    i = luaL_checknumber(L, 3);

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
    button->button = xutil_button_fromint(i);

    len = lua_objlen(L, 2);
    for(i = 1; i <= len; i++)
    {
        size_t blen;
        const char *buf;
        lua_rawgeti(L, 2, i);
        buf = luaL_checklstring(L, -1, &blen);
        button->mod |= xutil_key_mask_fromstr(buf, blen);
        lua_pop(L, 1);
    }

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
        /* works fine, but not *really* neat */
        lua_pushnumber(L, button->button);
        break;
      default:
        break;
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
        button->button = xutil_button_fromint(luaL_checknumber(L, 3));
        break;
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

/** Mouse library.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield coords Mouse coordinates.
 * \lfield screen Mouse screen number.
 */
static int
luaA_mouse_index(lua_State *L)
{
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);
    int16_t mouse_x, mouse_y;
    screen_t *screen;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, NULL, NULL))
            return 0;

        screen  = screen_getbycoord(screen, mouse_x, mouse_y);

        lua_pushnumber(L, screen_array_indexof(&globalconf.screens, screen) + 1);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Newindex for mouse.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_mouse_newindex(lua_State *L)
{
    size_t len;
    const char *attr = luaL_checklstring(L, 2, &len);
    int x, y = 0;
    xcb_window_t root;
    int screen, phys_screen;

    switch(a_tokenize(attr, len))
    {
      case A_TK_SCREEN:
        screen = luaL_checknumber(L, 3) - 1;
        luaA_checkscreen(screen);

        /* we need the physical one to get the root window */
        phys_screen = screen_virttophys(screen);
        root = xutil_screen_get(globalconf.connection, phys_screen)->root;

        x = globalconf.screens.tab[screen].geometry.x;
        y = globalconf.screens.tab[screen].geometry.y;

        mouse_warp_pointer(root, x, y);
        break;
      default:
        return 0;
    }

    return 0;
}

/** Push a table with mouse status.
 * \param L The Lua VM state.
 * \param x The x coordinate.
 * \param y The y coordinate.
 * \param mask The button mask.
 */
int
luaA_mouse_pushstatus(lua_State *L, int x, int y, uint16_t mask)
{
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");

    lua_createtable(L, 5, 0);

    int i = 1;

    for(uint16_t maski = XCB_BUTTON_MASK_1; maski <= XCB_BUTTON_MASK_5; maski <<= 1)
    {
        if(mask & maski)
            lua_pushboolean(L, true);
        else
            lua_pushboolean(L, false);
        lua_rawseti(L, -2, i++);
    }
    lua_setfield(L, -2, "buttons");
    return 1;
}

/** Get or set the mouse coords.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None or a table with x and y keys as mouse coordinates.
 * \lreturn A table with mouse coordinates.
 */
static int
luaA_mouse_coords(lua_State *L)
{
    uint16_t mask;
    int x, y;
    int16_t mouse_x, mouse_y;
    screen_t *screen;

    if(lua_gettop(L) == 1)
    {
        xcb_window_t root;

        luaA_checktable(L, 1);

        if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, NULL, &mask))
            return 0;

        x = luaA_getopt_number(L, 1, "x", mouse_x);
        y = luaA_getopt_number(L, 1, "y", mouse_y);

        root = xutil_screen_get(globalconf.connection,
                                screen_array_indexof(&globalconf.screens, screen))->root;
        mouse_warp_pointer(root, x, y);
        lua_pop(L, 1);
    }

    if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, NULL, &mask))
        return 0;

    return luaA_mouse_pushstatus(L, mouse_x, mouse_y, mask);
}

/** Get the client which is under the pointer.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn A client or nil.
 */
static int
luaA_mouse_object_under_pointer(lua_State *L)
{
    screen_t *screen;
    int16_t mouse_x, mouse_y;
    xcb_window_t child;

    if(!mouse_query_pointer_root(&screen, &mouse_x, &mouse_y, &child, NULL))
        return 0;

    wibox_t *wibox;
    client_t *client;
    if((wibox = wibox_getbywin(child)))
    {
        wibox_push(L, wibox);

        int16_t x = mouse_x - wibox->sw.geometry.x;
        int16_t y = mouse_y - wibox->sw.geometry.y;

        widget_t *widget = widget_getbycoords(wibox->position, &wibox->widgets,
                                              wibox->sw.geometry.width,
                                              wibox->sw.geometry.height,
                                              &x, &y);

        if(widget)
        {
            widget_push(L, widget);
            return 2;
        }
        return 1;
    }
    else if((client = client_getbywin(child)))
        return client_push(globalconf.L, client);

    return 0;
}

const struct luaL_reg awesome_mouse_methods[] =
{
    { "__index", luaA_mouse_index },
    { "__newindex", luaA_mouse_newindex },
    { "coords", luaA_mouse_coords },
    { "object_under_pointer", luaA_mouse_object_under_pointer },
    { NULL, NULL }
};
const struct luaL_reg awesome_mouse_meta[] =
{
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
