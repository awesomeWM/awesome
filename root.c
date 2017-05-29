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
 * @module root
 */

#include "globalconf.h"

#include "common/atoms.h"
#include "common/xcursor.h"
#include "common/xutil.h"
#include "objects/button.h"
#include "xwindow.h"

#include "math.h"

#include <xcb/xtest.h>
#include <xcb/xcb_aux.h>
#include <cairo-xcb.h>

static void
root_set_wallpaper_pixmap(xcb_connection_t *c, xcb_pixmap_t p)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    const xcb_screen_t *screen = globalconf.screen;

    /* We now have the pattern painted to the pixmap p. Now turn p into the root
     * window's background pixmap.
     */
    xcb_change_window_attributes(c, screen->root, XCB_CW_BACK_PIXMAP, &p);
    xcb_clear_area(c, 0, screen->root, 0, 0, 0, 0);

    prop_c = xcb_get_property_unchecked(c, false,
            screen->root, ESETROOT_PMAP_ID, XCB_ATOM_PIXMAP, 0, 1);

    /* Theoretically, this should be enough to set the wallpaper. However, to
     * make pseudo-transparency work, clients need a way to get the wallpaper.
     * You can't query a window's back pixmap, so properties are (ab)used.
     */
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, screen->root, _XROOTPMAP_ID, XCB_ATOM_PIXMAP, 32, 1, &p);
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, screen->root, ESETROOT_PMAP_ID, XCB_ATOM_PIXMAP, 32, 1, &p);

    /* Now make sure that the old wallpaper is freed (but only do this for ESETROOT_PMAP_ID) */
    prop_r = xcb_get_property_reply(c, prop_c, NULL);
    if (prop_r && prop_r->value_len)
    {
        xcb_pixmap_t *rootpix = xcb_get_property_value(prop_r);
        if (rootpix)
            xcb_kill_client(c, *rootpix);
    }
    p_delete(&prop_r);
}

static bool
root_set_wallpaper(cairo_pattern_t *pattern)
{
    lua_State *L = globalconf_get_lua_State();
    xcb_connection_t *c = xcb_connect(NULL, NULL);
    xcb_pixmap_t p = xcb_generate_id(c);
    /* globalconf.connection should be connected to the same X11 server, so we
     * can just use the info from that other connection.
     */
    const xcb_screen_t *screen = globalconf.screen;
    uint16_t width = screen->width_in_pixels;
    uint16_t height = screen->height_in_pixels;
    bool result = false;
    cairo_surface_t *surface;
    cairo_t *cr;

    if (xcb_connection_has_error(c))
        goto disconnect;

    /* Create a pixmap and make sure it is already created, because we are going
     * to use it from the other X11 connection (Juggling with X11 connections
     * is a really, really bad idea).
     */
    xcb_create_pixmap(c, screen->root_depth, p, screen->root, width, height);
    xcb_aux_sync(c);

    /* Now paint to the picture from the main connection so that cairo sees that
     * it can tell the X server to copy between the (possible) old pixmap and
     * the new one directly and doesn't need GetImage and PutImage.
     */
    surface = cairo_xcb_surface_create(globalconf.connection, p, draw_default_visual(screen), width, height);
    cr = cairo_create(surface);
    /* Paint the pattern to the surface */
    cairo_set_source(cr, pattern);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_flush(surface);
    xcb_aux_sync(globalconf.connection);

    /* Change the wallpaper, without sending us a PropertyNotify event */
    xcb_grab_server(globalconf.connection);
    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 (uint32_t[]) { 0 });
    root_set_wallpaper_pixmap(globalconf.connection, p);
    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 ROOT_WINDOW_EVENT_MASK);
    xcb_ungrab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* Make sure our pixmap is not destroyed when we disconnect. */
    xcb_set_close_down_mode(c, XCB_CLOSE_DOWN_RETAIN_PERMANENT);

    /* Tell Lua that the wallpaper changed */
    cairo_surface_destroy(globalconf.wallpaper);
    globalconf.wallpaper = surface;
    signal_object_emit(L, &global_signals, "wallpaper_changed", 0);

    result = true;
disconnect:
    xcb_aux_sync(c);
    xcb_disconnect(c);
    return result;
}

void
root_update_wallpaper(void)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    xcb_get_geometry_cookie_t geom_c;
    xcb_get_geometry_reply_t *geom_r;
    xcb_pixmap_t *rootpix;

    cairo_surface_destroy(globalconf.wallpaper);
    globalconf.wallpaper = NULL;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false,
            globalconf.screen->root, _XROOTPMAP_ID, XCB_ATOM_PIXMAP, 0, 1);
    prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL);

    if (!prop_r || !prop_r->value_len)
    {
        p_delete(&prop_r);
        return;
    }

    rootpix = xcb_get_property_value(prop_r);
    if (!rootpix)
    {
        p_delete(&prop_r);
        return;
    }

    geom_c = xcb_get_geometry_unchecked(globalconf.connection, *rootpix);
    geom_r = xcb_get_geometry_reply(globalconf.connection, geom_c, NULL);
    if (!geom_r)
    {
        p_delete(&prop_r);
        return;
    }

    /* Only the default visual makes sense, so just the default depth */
    if (geom_r->depth != draw_visual_depth(globalconf.screen, globalconf.default_visual->visual_id))
        warn("Got a pixmap with depth %d, but the default depth is %d, continuing anyway",
                geom_r->depth, draw_visual_depth(globalconf.screen, globalconf.default_visual->visual_id));

    globalconf.wallpaper = cairo_xcb_surface_create(globalconf.connection,
                                                    *rootpix,
                                                    globalconf.default_visual,
                                                    geom_r->width,
                                                    geom_r->height);

    p_delete(&prop_r);
    p_delete(&geom_r);
}

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

/** Send fake events. Usually the currently focused client will get it.
 *
 * @param event_type The event type: key_press, key_release, button_press,
 *  button_release or motion_notify.
 * @param detail The detail: in case of a key event, this is the keycode
 *  to send, in case of a button event this is the number of the button. In
 *  case of a motion event, this is a boolean value which if true makes the
 *  coordinates relatives.
 * @param x In case of a motion event, this is the X coordinate.
 * @param y In case of a motion event, this is the Y coordinate.
 * @function fake_input
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
 * @function keys
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

        xcb_screen_t *s = globalconf.screen;
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
 * This binding will be available when you click on the root window.
 *
 * @param button_table An array of mouse button bindings objects, or nothing.
 * @return The array of mouse button bindings objects.
 * @function buttons
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
 * @function cursor
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
 * @function drawins
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
 * @function wallpaper
 */
static int
luaA_root_wallpaper(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        cairo_pattern_t *pattern = (cairo_pattern_t *)lua_touserdata(L, -1);
        lua_pushboolean(L, root_set_wallpaper(pattern));
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
 * @function size
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
 * @function size_mm
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
 * @function tags
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
