/*
 * window.c - window object
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#include "luaa.h"
#include "xwindow.h"
#include "ewmh.h"
#include "screen.h"
#include "property.h"
#include "objects/window.h"
#include "common/atoms.h"
#include "common/luaobject.h"

LUA_CLASS_FUNCS(window, window_class)

static xcb_window_t
window_get(window_t *window)
{
    if (window->frame_window != XCB_NONE)
        return window->frame_window;
    return window->window;
}

static void
window_wipe(window_t *window)
{
    button_array_wipe(&window->buttons);
}

/** Get or set mouse buttons bindings on a window.
 * \param L The Lua VM state.
 * \return The number of elements pushed on the stack.
 */
static int
luaA_window_buttons(lua_State *L)
{
    window_t *window = luaA_checkudata(L, 1, &window_class);

    if(lua_gettop(L) == 2)
    {
        luaA_button_array_set(L, 1, 2, &window->buttons);
        luaA_object_emit_signal(L, 1, "property::buttons", 0);
        xwindow_buttons_grab(window_get(window), &window->buttons);
    }

    return luaA_button_array_get(L, 1, &window->buttons);
}

/** Return window struts (reserved space at the edge of the screen).
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_struts(lua_State *L)
{
    window_t *window = luaA_checkudata(L, 1, &window_class);

    if(lua_gettop(L) == 2)
    {
        luaA_tostrut(L, 2, &window->strut);
        ewmh_update_strut(window->window, &window->strut);
        luaA_object_emit_signal(L, 1, "property::struts", 0);
        /* FIXME: Only emit if the workarea actually changed
         * (= window is visible, only on the right screen)? */
        foreach(s, globalconf.screens)
            screen_emit_signal(L, s, "property::workarea", 0);
    }

    return luaA_pushstrut(L, window->strut);
}

/** Set a window opacity.
 * \param L The Lua VM state.
 * \param idx The index of the window on the stack.
 * \param opacity The opacity value.
 */
void
window_set_opacity(lua_State *L, int idx, double opacity)
{
    window_t *window = luaA_checkudata(L, idx, &window_class);

    if(window->opacity != opacity)
    {
        window->opacity = opacity;
        xwindow_set_opacity(window_get(window), opacity);
        luaA_object_emit_signal(L, idx, "property::opacity", 0);
    }
}

/** Set a window opacity.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_set_opacity(lua_State *L, window_t *window)
{
    if(lua_isnil(L, -1))
        window_set_opacity(L, -3, -1);
    else
    {
        double d = luaL_checknumber(L, -1);
        if(d >= 0 && d <= 1)
            window_set_opacity(L, -3, d);
    }
    return 0;
}

/** Get the window opacity.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_get_opacity(lua_State *L, window_t *window)
{
    if(window->opacity >= 0)
        lua_pushnumber(L, window->opacity);
    else
        /* Let's always return some "good" value */
        lua_pushnumber(L, 1);
    return 1;
}

/** Set the window border color.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_set_border_color(lua_State *L, window_t *window)
{
    size_t len;
    const char *color_name = luaL_checklstring(L, -1, &len);

    if(color_name &&
       color_init_reply(color_init_unchecked(&window->border_color, color_name, len)))
    {
        xwindow_set_border_color(window_get(window), &window->border_color);
        luaA_object_emit_signal(L, -3, "property::border_color", 0);
    }

    return 0;
}

/** Set a window border width.
 * \param L The Lua VM state.
 * \param idx The window index.
 * \param width The border width.
 */
void
window_set_border_width(lua_State *L, int idx, int width)
{
    window_t *window = luaA_checkudata(L, idx, &window_class);
    uint16_t old_width = window->border_width;

    if(width == window->border_width || width < 0)
        return;

    if(window->window)
        xcb_configure_window(globalconf.connection, window_get(window),
                             XCB_CONFIG_WINDOW_BORDER_WIDTH,
                             (uint32_t[]) { width });

    window->border_width = width;

    if(window->border_width_callback)
        (*window->border_width_callback)(window, old_width, width);

    luaA_object_emit_signal(L, idx, "property::border_width", 0);
}

/** Get the window type.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
int
luaA_window_get_type(lua_State *L, window_t *w)
{
    switch(w->type)
    {
      case WINDOW_TYPE_DESKTOP:
        lua_pushliteral(L, "desktop");
        break;
      case WINDOW_TYPE_DOCK:
        lua_pushliteral(L, "dock");
        break;
      case WINDOW_TYPE_SPLASH:
        lua_pushliteral(L, "splash");
        break;
      case WINDOW_TYPE_DIALOG:
        lua_pushliteral(L, "dialog");
        break;
      case WINDOW_TYPE_MENU:
        lua_pushliteral(L, "menu");
        break;
      case WINDOW_TYPE_TOOLBAR:
        lua_pushliteral(L, "toolbar");
        break;
      case WINDOW_TYPE_UTILITY:
        lua_pushliteral(L, "utility");
        break;
      case WINDOW_TYPE_DROPDOWN_MENU:
        lua_pushliteral(L, "dropdown_menu");
        break;
      case WINDOW_TYPE_POPUP_MENU:
        lua_pushliteral(L, "popup_menu");
        break;
      case WINDOW_TYPE_TOOLTIP:
        lua_pushliteral(L, "tooltip");
        break;
      case WINDOW_TYPE_NOTIFICATION:
        lua_pushliteral(L, "notification");
        break;
      case WINDOW_TYPE_COMBO:
        lua_pushliteral(L, "combo");
        break;
      case WINDOW_TYPE_DND:
        lua_pushliteral(L, "dnd");
        break;
      case WINDOW_TYPE_NORMAL:
        lua_pushliteral(L, "normal");
        break;
      default:
        return 0;
    }
    return 1;
}

/** Set the window type.
 * \param L The Lua VM state.
 * \param window The window object.
 * \return The number of elements pushed on stack.
 */
int
luaA_window_set_type(lua_State *L, window_t *w)
{
    window_type_t type;
    const char *buf = luaL_checkstring(L, -1);

    if (A_STREQ(buf, "desktop"))
        type = WINDOW_TYPE_DESKTOP;
    else if(A_STREQ(buf, "dock"))
        type = WINDOW_TYPE_DOCK;
    else if(A_STREQ(buf, "splash"))
        type = WINDOW_TYPE_SPLASH;
    else if(A_STREQ(buf, "dialog"))
        type = WINDOW_TYPE_DIALOG;
    else if(A_STREQ(buf, "menu"))
        type = WINDOW_TYPE_MENU;
    else if(A_STREQ(buf, "toolbar"))
        type = WINDOW_TYPE_TOOLBAR;
    else if(A_STREQ(buf, "utility"))
        type = WINDOW_TYPE_UTILITY;
    else if(A_STREQ(buf, "dropdown_menu"))
        type = WINDOW_TYPE_DROPDOWN_MENU;
    else if(A_STREQ(buf, "popup_menu"))
        type = WINDOW_TYPE_POPUP_MENU;
    else if(A_STREQ(buf, "tooltip"))
        type = WINDOW_TYPE_TOOLTIP;
    else if(A_STREQ(buf, "notification"))
        type = WINDOW_TYPE_NOTIFICATION;
    else if(A_STREQ(buf, "combo"))
        type = WINDOW_TYPE_COMBO;
    else if(A_STREQ(buf, "dnd"))
        type = WINDOW_TYPE_DND;
    else if(A_STREQ(buf, "normal"))
        type = WINDOW_TYPE_NORMAL;
    else
    {
        warn("Unknown window type '%s'", buf);
        return 0;
    }

    if(w->type != type)
    {
        w->type = type;
        if(w->window != XCB_WINDOW_NONE)
            ewmh_update_window_type(w->window, window_translate_type(w->type));
        luaA_object_emit_signal(globalconf.L, -3, "property::type", 0);
    }

    return 0;
}

static xproperty_t *
luaA_find_xproperty(lua_State *L, int idx)
{
    const char *name = luaL_checkstring(L, idx);
    foreach(prop, globalconf.xproperties)
        if (A_STREQ(prop->name, name))
            return prop;
    luaL_argerror(L, idx, "Unknown xproperty");
    return NULL;
}

int
window_set_xproperty(lua_State *L, xcb_window_t window, int prop_idx, int value_idx)
{
    xproperty_t *prop = luaA_find_xproperty(L, prop_idx);
    xcb_atom_t type;
    uint8_t format;
    size_t len;
    uint32_t number;
    const void *data;

    if(lua_isnil(L, value_idx))
    {
        xcb_delete_property(globalconf.connection, window, prop->atom);
    } else {
        if(prop->type == PROP_STRING)
        {
            data = luaL_checklstring(L, value_idx, &len);
            type = UTF8_STRING;
            format = 8;
        } else if(prop->type == PROP_NUMBER || prop->type == PROP_BOOLEAN)
        {
            if (prop->type == PROP_NUMBER)
                number = luaL_checkinteger(L, value_idx);
            else
                number = luaA_checkboolean(L, value_idx);
            data = &number;
            len = 1;
            type = XCB_ATOM_CARDINAL;
            format = 32;
        } else
            fatal("Got an xproperty with invalid type");

        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, window,
                            prop->atom, type, format, len, data);
    }
    return 0;
}

int
window_get_xproperty(lua_State *L, xcb_window_t window, int prop_idx)
{
    xproperty_t *prop = luaA_find_xproperty(L, prop_idx);
    xcb_atom_t type;
    void *data;
    xcb_get_property_reply_t *reply;
    uint32_t length;

    type = prop->type == PROP_STRING ? UTF8_STRING : XCB_ATOM_CARDINAL;
    length = prop->type == PROP_STRING ? UINT32_MAX : 1;
    reply = xcb_get_property_reply(globalconf.connection,
            xcb_get_property_unchecked(globalconf.connection, false, window,
                prop->atom, type, 0, length), NULL);
    if(!reply)
        return 0;

    data = xcb_get_property_value(reply);

    if(prop->type == PROP_STRING)
        lua_pushlstring(L, data, reply->value_len);
    else
    {
        if(reply->value_len <= 0)
        {
            p_delete(&reply);
            return 0;
        }
        if(prop->type == PROP_NUMBER)
            lua_pushinteger(L, *(uint32_t *) data);
        else
            lua_pushboolean(L, *(uint32_t *) data);
    }

    p_delete(&reply);
    return 1;
}

/** Set an xproperty.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_set_xproperty(lua_State *L)
{
    window_t *w = luaA_checkudata(L, 1, &window_class);
    return window_set_xproperty(L, w->window, 2, 3);
}

/** Get an xproperty.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_window_get_xproperty(lua_State *L)
{
    window_t *w = luaA_checkudata(L, 1, &window_class);
    return window_get_xproperty(L, w->window, 2);
}

/** Translate a window_type_t into the corresponding EWMH atom.
 * @param type The type to return.
 * @return The EWMH atom for this type.
 */
uint32_t
window_translate_type(window_type_t type)
{
    switch(type)
    {
    case WINDOW_TYPE_NORMAL:
        return _NET_WM_WINDOW_TYPE_NORMAL;
    case WINDOW_TYPE_DESKTOP:
        return _NET_WM_WINDOW_TYPE_DESKTOP;
    case WINDOW_TYPE_DOCK:
        return _NET_WM_WINDOW_TYPE_DOCK;
    case WINDOW_TYPE_SPLASH:
        return _NET_WM_WINDOW_TYPE_SPLASH;
    case WINDOW_TYPE_DIALOG:
        return _NET_WM_WINDOW_TYPE_DIALOG;
    case WINDOW_TYPE_MENU:
        return _NET_WM_WINDOW_TYPE_MENU;
    case WINDOW_TYPE_TOOLBAR:
        return _NET_WM_WINDOW_TYPE_TOOLBAR;
    case WINDOW_TYPE_UTILITY:
        return _NET_WM_WINDOW_TYPE_UTILITY;
    case WINDOW_TYPE_DROPDOWN_MENU:
        return _NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
    case WINDOW_TYPE_POPUP_MENU:
        return _NET_WM_WINDOW_TYPE_POPUP_MENU;
    case WINDOW_TYPE_TOOLTIP:
        return _NET_WM_WINDOW_TYPE_TOOLTIP;
    case WINDOW_TYPE_NOTIFICATION:
        return _NET_WM_WINDOW_TYPE_NOTIFICATION;
    case WINDOW_TYPE_COMBO:
        return _NET_WM_WINDOW_TYPE_COMBO;
    case WINDOW_TYPE_DND:
        return _NET_WM_WINDOW_TYPE_DND;
    }
    return _NET_WM_WINDOW_TYPE_NORMAL;
}

static int
luaA_window_set_border_width(lua_State *L, window_t *c)
{
    window_set_border_width(L, -3, luaL_checknumber(L, -1));
    return 0;
}

LUA_OBJECT_EXPORT_PROPERTY(window, window_t, window, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(window, window_t, border_color, luaA_pushcolor)
LUA_OBJECT_EXPORT_PROPERTY(window, window_t, border_width, lua_pushnumber)

void
window_class_setup(lua_State *L)
{
    static const struct luaL_Reg window_methods[] =
    {
        { NULL, NULL }
    };

    static const struct luaL_Reg window_meta[] =
    {
        { "struts", luaA_window_struts },
        { "buttons", luaA_window_buttons },
        { "set_xproperty", luaA_window_set_xproperty },
        { "get_xproperty", luaA_window_get_xproperty },
        { NULL, NULL }
    };

    luaA_class_setup(L, &window_class, "window", NULL,
                     NULL, (lua_class_collector_t) window_wipe, NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     window_methods, window_meta);

    luaA_class_add_property(&window_class, "window",
                            NULL,
                            (lua_class_propfunc_t) luaA_window_get_window,
                            NULL);
    luaA_class_add_property(&window_class, "opacity",
                            (lua_class_propfunc_t) luaA_window_set_opacity,
                            (lua_class_propfunc_t) luaA_window_get_opacity,
                            (lua_class_propfunc_t) luaA_window_set_opacity);
    luaA_class_add_property(&window_class, "border_color",
                            (lua_class_propfunc_t) luaA_window_set_border_color,
                            (lua_class_propfunc_t) luaA_window_get_border_color,
                            (lua_class_propfunc_t) luaA_window_set_border_color);
    luaA_class_add_property(&window_class, "border_width",
                            (lua_class_propfunc_t) luaA_window_set_border_width,
                            (lua_class_propfunc_t) luaA_window_get_border_width,
                            (lua_class_propfunc_t) luaA_window_set_border_width);

    signal_add(&window_class.signals, "property::border_color");
    signal_add(&window_class.signals, "property::border_width");
    signal_add(&window_class.signals, "property::buttons");
    signal_add(&window_class.signals, "property::opacity");
    signal_add(&window_class.signals, "property::struts");
    signal_add(&window_class.signals, "property::type");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
