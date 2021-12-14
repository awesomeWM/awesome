/*
 * luaa.c - Lua configuration management
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

/** AwesomeWM lifecycle API.
 *
 * This module contains the functions and signal to manage the lifecycle of the
 * AwesomeWM process. It allows to execute code at specific point from the early
 * initialization all the way to the last events before exiting or restarting.
 *
 * Additionally it handles signals for spawn and keyboard related events.
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @coreclassmod awesome
 */

/** Register a new xproperty.
 *
 * @tparam string name The name of the X11 property.
 * @tparam string type One of "string", "number" or "boolean".
 * @staticfct register_xproperty
 */

#define _GNU_SOURCE

#include "luaa.h"
#include "globalconf.h"
#include "awesome.h"
#include "common/backtrace.h"
#include "common/version.h"
#include "config.h"
#include "event.h"
#include "objects/client.h"
#include "objects/drawable.h"
#include "objects/drawin.h"
#include "objects/selection_getter.h"
#include "objects/screen.h"
#include "objects/selection_acquire.h"
#include "objects/selection_transfer.h"
#include "objects/selection_watcher.h"
#include "objects/tag.h"
#include "property.h"
#include "selection.h"
#include "spawn.h"
#include "systray.h"
#include "xkb.h"
#include "xrdb.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* for strings and Unicode handling */
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <basedir_fs.h>

#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include "xkb_utf32_to_keysym_compat.c"

#include <unistd.h> /* for gethostname() */

#ifdef WITH_DBUS
extern const struct luaL_Reg awesome_dbus_lib[];
#endif
extern const struct luaL_Reg awesome_keygrabber_lib[];
extern const struct luaL_Reg awesome_mousegrabber_lib[];
extern const struct luaL_Reg awesome_mouse_methods[];
extern const struct luaL_Reg awesome_mouse_meta[];
extern const struct luaL_Reg awesome_root_methods[];
extern const struct luaL_Reg awesome_root_meta[];

signal_array_t global_signals;

/** A call into the Lua code aborted with an error.
 *
 * This signal is used in the example configuration, @{05-awesomerc.md},
 * to let a notification box pop up.
 * @tparam table err Table with the error object, can be converted to a string with
 * `tostring(err)`.
 * @signal debug::error
 */

/** A deprecated Lua function was called.
 *
 * @tparam string hint String with a hint on what to use instead of the
 * deprecated functionality.
 * @tparam[opt=nil] string|nil see The name of the newer API
 * @tparam[opt=nil] table|nil args The name of the newer API
 * @signal debug::deprecation
 */

/** An invalid key was read from an object.
 *
 * This can happen if `foo` in an `c.foo` access does not exist.
 * @param unknown1 Class?
 * @param unknown2 Key?
 * @signal debug::index::miss
 */

/** An invalid key was written to an object.
 *
 * This can happen if `foo` in an `c.foo = "bar"` assignment doesn't exist.
 * @param unknown1 Class?
 * @param unknown2 Key?
 * @param unknown3 Value?
 * @signal debug::newindex::miss
 */

/** The systray should be updated.
 *
 * This signal is used in `wibox.widget.systray`.
 * @signal systray::update
 */

/** The wallpaper has changed.
 *
 * This signal is used for pseudo-transparency in `wibox.drawable` if no
 * composite manager is running.
 * @signal wallpaper_changed
 */

/** Keyboard map has changed.
 *
 * This signal is sent after the new keymap has been loaded. It is used in
 * `awful.widget.keyboardlayout` to redraw the layout.
 * @signal xkb::map_changed
 */

/** Keyboard group has changed.
 *
 * It's used in `awful.widget.keyboardlayout` to redraw the layout.
 * @tparam number group Integer containing the changed group
 * @signal xkb::group_changed.
 */

/** Refresh.
 *
 * This signal is emitted as a kind of idle signal in the event loop.
 * One example usage is in `gears.timer` to executed delayed calls.
 * @signal refresh
 */

/** AwesomeWM is about to enter the event loop.
 *
 * This means all initialization has been done.
 * @signal startup
 */

/** AwesomeWM is exiting / about to restart.
 *
 * This signal is emitted in the `atexit` handler as well when awesome
 * restarts.
 * @tparam boolean reason_restart Boolean value is true if the signal was sent
 *  because of a restart.
 * @signal exit
 */

/** The output status of a screen has changed.
 *
 * @tparam string output String containing which output has changed.
 * @tparam string connection_state String containing the connection status of
 * the output: It will be either "Connected", "Disconnected" or
 * "Unknown".
 * @signal screen::change
 */

/** Path to config file */
static char *conffile;

/** Check whether a composite manager is running.
 * \return True if such a manager is running.
 */
static bool
composite_manager_running(void)
{
    xcb_intern_atom_reply_t *atom_r;
    xcb_get_selection_owner_reply_t *selection_r;
    char *atom_name;
    bool result;

    if(!(atom_name = xcb_atom_name_by_screen("_NET_WM_CM", globalconf.default_screen)))
    {
        warn("error getting composite manager atom");
        return false;
    }

    atom_r = xcb_intern_atom_reply(globalconf.connection,
                                   xcb_intern_atom_unchecked(globalconf.connection, false,
                                                             a_strlen(atom_name), atom_name),
                                   NULL);
    p_delete(&atom_name);
    if(!atom_r)
        return false;

    selection_r = xcb_get_selection_owner_reply(globalconf.connection,
                                                xcb_get_selection_owner_unchecked(globalconf.connection,
                                                                                  atom_r->atom),
                                                NULL);
    p_delete(&atom_r);

    result = selection_r != NULL && selection_r->owner != XCB_NONE;
    p_delete(&selection_r);

    return result;
}

/** Quit awesome.
 * @tparam[opt=0] integer code The exit code to use when exiting.
 * @staticfct quit
 */
static int
luaA_quit(lua_State *L)
{
    if (!lua_isnoneornil(L, 1))
        globalconf.exit_code = luaL_checkinteger(L, 1);
    if (globalconf.loop == NULL)
        globalconf.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_quit(globalconf.loop);
    return 0;
}

/** Execute another application, probably a window manager, to replace
 * awesome.
 *
 * @tparam string cmd The command line to execute.
 * @staticfct exec
 */
static int
luaA_exec(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);

    awesome_atexit(false);

    a_exec(cmd);
    return 0;
}

/** Restart awesome.
 * @staticfct restart
 */
static int
luaA_restart(lua_State *L)
{
    awesome_restart();
    return 0;
}

/** Send a signal to a process.
 * @tparam integer pid Process identifier.  0 and negative values have special
 *   meaning.  See `man 3 kill`.
 * @tparam integer sig Signal number.
 *   See `awesome.unix_signal` for a list of signals.
 * @treturn boolean true if the signal was successfully sent, else false
 * @staticfct kill
 */
static int
luaA_kill(lua_State *L)
{
    int pid = luaL_checknumber(L, 1);
    int sig = luaA_checknumber_range(L, 2, 0, INT_MAX);

    int result = kill(pid, sig);
    lua_pushboolean(L, result == 0);
    return 1;
}

/** Synchronize with the X11 server. This is needed in the test suite to avoid
 * some race conditions. You should never need to use this function.
 * @staticfct sync
 */
static int
luaA_sync(lua_State *L)
{
    xcb_aux_sync(globalconf.connection);
    return 0;
}

/** Translate a GdkPixbuf to a cairo image surface..
 *
 * @param pixbuf The pixbuf as a light user datum.
 * @param path The pixbuf origin path
 * @treturn gears.surface A cairo surface as light user datum.
 * @staticfct pixbuf_to_surface
 */
static int
luaA_pixbuf_to_surface(lua_State *L)
{
    GdkPixbuf *pixbuf = (GdkPixbuf *) lua_touserdata(L, 1);
    cairo_surface_t *surface = draw_surface_from_pixbuf(pixbuf);

    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surface);
    return 1;
}

/** Load an image from a given path.
 *
 * @tparam string name The file name.
 * @treturn gears.surface A cairo surface as light user datum.
 * @treturn nil|string The error message, if any.
 * @staticfct load_image
 */
static int
luaA_load_image(lua_State *L)
{
    /* TODO: Deprecate this function, Lua can use GdkPixbuf directly plus
     * awesome.pixbuf_to_surface
     */

    GError *error = NULL;
    const char *filename = luaL_checkstring(L, 1);
    cairo_surface_t *surface = draw_load_image(L, filename, &error);
    if (!surface) {
        lua_pushnil(L);
        lua_pushstring(L, error->message);
        g_error_free(error);
        return 2;
    }

    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surface);
    return 1;
}

/** Set the preferred size for client icons.
 *
 * The closest equal or bigger size is picked if present, otherwise the closest
 * smaller size is picked. The default is 0 pixels, ie. the smallest icon.
 *
 * @tparam integer size The size of the icons in pixels.
 * @staticfct set_preferred_icon_size
 */
static int
luaA_set_preferred_icon_size(lua_State *L)
{
    globalconf.preferred_icon_size = luaA_checkinteger_range(L, 1, 0, UINT32_MAX);
    return 0;
}

/** UTF-8 aware string length computing.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_mbstrlen(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);
    lua_pushinteger(L, (ssize_t) mbstowcs(NULL, NONULL(cmd), 0));
    return 1;
}

/** Enhanced type() function which recognize awesome objects.
 * \param L The Lua VM state.
 * \return The number of arguments pushed on the stack.
 */
static int
luaAe_type(lua_State *L)
{
    luaL_checkany(L, 1);
    lua_pushstring(L, luaA_typename(L, 1));
    return 1;
}

/** Replace various standards Lua functions with our own.
 * \param L The Lua VM state.
 */
static void
luaA_fixups(lua_State *L)
{
    /* export string.wlen */
    lua_getglobal(L, "string");
    lua_pushcfunction(L, luaA_mbstrlen);
    lua_setfield(L, -2, "wlen");
    lua_pop(L, 1);
    /* replace type */
    lua_pushcfunction(L, luaAe_type);
    lua_setglobal(L, "type");
}

static const char *
get_modifier_name(int map_index)
{
    switch (map_index) {
        case XCB_MAP_INDEX_SHIFT:   return "Shift";
        case XCB_MAP_INDEX_LOCK:    return "Lock";
        case XCB_MAP_INDEX_CONTROL: return "Control";
        case XCB_MAP_INDEX_1:       return "Mod1"; /* Alt */
        case XCB_MAP_INDEX_2:       return "Mod2";
        case XCB_MAP_INDEX_3:       return "Mod3";
        case XCB_MAP_INDEX_4:       return "Mod4";
        case XCB_MAP_INDEX_5:       return "Mod5";

    }

    return 0; /* \0 */
}

/* Helper function for luaA_get_key_name() below.
 * Will return the UTF-32 codepoint IF AND ONLY IF the input is exactly one
 * valid UTF-8 character. Otherwise, it will return zero.
 */
static uint32_t
one_utf8_to_utf32(const char* input, const size_t length) {
    gunichar character = g_utf8_get_char_validated(input, length);
    // Return 0 if there is more than one UTF-8 character:
    if (g_unichar_to_utf8(character, NULL) != (gint)length)
        return 0;
    // Return 0 if the character is invalid.
    if (character == (gunichar)-1 || character == (gunichar)-2)
        return 0;
    return character;
}

/* Get X11 keysym and a one-character representation from an Awesome keycode.
 *
 * A "one-character representation" is a single UTF-8 representing the typical
 * output from that keysym in a text editor (e.g. " " for space, "ñ" for
 * n_tilde, "Ā" for A_macron). It usually matches the main engraving of the key
 * for level-0 symbols (but lowercase).
 *
 * Keycodes may be given in a string in any valid format for `awful.key`:
 * "#" + keycode, the symkey name and the UTF-8 representation will all work.
 *
 * If no suitable keysym is found, or a malformed keycode is given as an
 * argument, this function will return (nil, nil)
 *
 * @treturn[1] string keysym The keysym name
 * @treturn[1] nil keysym If no valid keysym is found
 * @treturn[2] string printsymbol The xkb_keysym_to_utf8 result
 * @treturn[2] nil printsymbol If the keysym has no printable representation.
 * @staticfct awful.keyboard.get_key_name
 */

static int
luaA_get_key_name(lua_State *L)
{
    // check if argument is valid
    if (lua_gettop(L) > 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        return 0;
    }

    const char* input = luaL_checkstring(L, 1);
    const xkb_keysym_t *keysyms;
    xkb_keysym_t keysym = XKB_KEY_NoSymbol;
    const size_t length = strlen(input);
    uint32_t ucs;

    /* Checking for the three possible syntaxes awful.key uses:
     * 1: #keycode (#8 to #255, any other is invalid)
     * 2: the symbol itself (the result of xkb_keysym_to_utf8, e.g. @ for at).
     * 3: the keysym
     */
    if (input[0] == '#' && input[1] != '\0' && length < 5) // syntax #1
    {
        int keycode_from_hash = atoi(input+1);
        // We discard keycodes with invalid values:
        const xcb_setup_t *setup = xcb_get_setup(globalconf.connection);
        if (keycode_from_hash < setup->min_keycode ||
            keycode_from_hash > setup->max_keycode)
            return 0;
        xkb_keycode_t keycode = (xkb_keycode_t) keycode_from_hash;
        struct xkb_keymap *keymap = xkb_state_get_keymap(globalconf.xkb_state);
        xkb_keymap_key_get_syms_by_level(keymap, keycode, 0, 0, &keysyms);
        keysym = keysyms[0];
    }
    else if ((ucs = one_utf8_to_utf32(input, length)) > 0) //syntax #2
        keysym = xkb_utf32_to_keysym_compat(ucs);
    else //syntax #3
        keysym = xkb_keysym_from_name(input, XKB_KEYSYM_NO_FLAGS);

    if (keysym == XKB_KEY_NoSymbol)
        return 0;
    else
    {
        char *name = key_get_keysym_name(keysym);
        lua_pushstring(L, name);
        char utfname[8];
        if (xkb_keysym_to_utf8(keysym, utfname, 8) > 0)
            lua_pushstring(L, utfname);
        else
            return 1; // this will make the second returned value a nil
    }
    return 2;
}

/* Undocumented */
/*
 * The table of keybindings modifiers.
 *
 * Each modifier has zero to many entries depending on the keyboard layout.
 * For example, `Shift` usually both has a left and right variant. Each
 * modifier entry has a `keysym` and `keycode` entry. For the US PC 105
 * keyboard, it looks like:
 *
 *    awesome.modifiers = {
 *         Shift = {
 *              {keycode = 50 , keysym = 'Shift_L'    },
 *              {keycode = 62 , keysym = 'Shift_R'    },
 *         },
 *         Lock = {},
 *         Control = {
 *              {keycode = 37 , keysym = 'Control_L'  },
 *              {keycode = 105, keysym = 'Control_R'  },
 *         },
 *         Mod1 = {
 *              {keycode = 64 , keysym = 'Alt_L'      },
 *              {keycode = 108, keysym = 'Alt_R'      },
 *         },
 *         Mod2 = {
 *              {keycode = 77 , keysym = 'Num_Lock'   },
 *         },
 *         Mod3 = {},
 *         Mod4 = {
 *              {keycode = 133, keysym = 'Super_L'    },
 *              {keycode = 134, keysym = 'Super_R'    },
 *         },
 *         Mod5 = {
 *              {keycode = 203, keysym = 'Mode_switch'},
 *         },
 *    };
 *
 * @tfield table modifiers
 * @tparam table modifiers.Shift The Shift modifiers.
 * @tparam table modifiers.Lock The Lock modifiers.
 * @tparam table modifiers.Control The Control modifiers.
 * @tparam table modifiers.Mod1 The Mod1 (Alt) modifiers.
 * @tparam table modifiers.Mod2 The Mod2 modifiers.
 * @tparam table modifiers.Mod3 The Mod3 modifiers.
 * @tparam table modifiers.Mod4 The Mod4 modifiers.
 * @tparam table modifiers.Mod5 The Mod5 modifiers.
 */

/*
 * Modifiers can change over time, given they are currently not tracked, just
 * query them each time. Use with moderation.
 */
static int luaA_get_modifiers(lua_State *L)
{
    xcb_get_modifier_mapping_reply_t *mods = xcb_get_modifier_mapping_reply(globalconf.connection,
            xcb_get_modifier_mapping(globalconf.connection), NULL);
    if (!mods)
        return 0;

    xcb_keycode_t *mappings = xcb_get_modifier_mapping_keycodes(mods);
    struct xkb_keymap *keymap = xkb_state_get_keymap(globalconf.xkb_state);

    lua_newtable(L);

    /* This get the MAPPED modifiers, not all of them are */
    for (int i = XCB_MAP_INDEX_SHIFT; i <= XCB_MAP_INDEX_5; i++) {
        lua_pushstring(L, get_modifier_name(i));
        lua_newtable(L);

        for (int j = 0; j < mods->keycodes_per_modifier; j++) {
            const xkb_keysym_t *keysyms;
            const xcb_keycode_t key_code = mappings[i * mods->keycodes_per_modifier + j];
            xkb_keymap_key_get_syms_by_level(keymap, key_code, 0, 0, &keysyms);
            if (keysyms != NULL) {
                /* The +1 because j starts at zero and Lua at 1 */
                lua_pushinteger(L, j+1);

                lua_newtable(L);

                lua_pushstring(L, "keycode");
                lua_pushinteger(L, key_code);
                lua_settable(L, -3);

                /* Technically it is possible to get multiple keysyms here,
                 * but... we just use the first one.
                 */
                lua_pushstring(L, "keysym");
                char *string = key_get_keysym_name(keysyms[0]);
                lua_pushstring(L, string);
                p_delete(&string);
                lua_settable(L, -3);

                lua_settable(L, -3);
            }
        }
        lua_settable(L, -3);
    }

    free(mods);

    return 0;
}

/* Undocumented */
/*
 * A table with the currently active modifier names.
 *
 * @tfield table _active_modifiers
 */

static int luaA_get_active_modifiers(lua_State *L)
{
    int count = 1;
    lua_newtable(L);

    for (int i = XCB_MAP_INDEX_SHIFT; i <= XCB_MAP_INDEX_5; i++) {
        const int active = xkb_state_mod_index_is_active (globalconf.xkb_state, i,
            XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE
        );

        if (active) {
            lua_pushstring(L, get_modifier_name(i));
            lua_rawseti(L,-2, count++);
        }
    }

    return 0;
}

/**
 * The AwesomeWM version.
 * @tfield string version
 */

/**
 * The AwesomeWM release name.
 * @tfield string release
 */

/**
 * The AwesomeWM API level.
 *
 * By default, this matches the major version (first component of the version).
 *
 * API levels are used to allow newer version of AwesomeWM to alter the behavior
 * and subset deprecated APIs. Using an older API level than the current major
 * version allows to use legacy `rc.lua` with little porting. However, they won't
 * be able to use all the new features. Attempting to use a newer feature along
 * with an older API level is not and will not be supported, even if it almost
 * works. Keeping up to date with the newer API levels is highly recommended.
 *
 * Going the other direction, setting an higher API level allows to take
 * advantage of experimental feature. It will also be much harsher when it comes
 * to deprecation. Setting the API level value beyond `current+3` will treat
 * using APIs currently pending deprecation as fatal errors. All new code
 * submitted to the upstream AwesomeWM codebase is forbidden to use deprecated
 * APIs. Testing your patches with mode and the default config is recommended
 * before submitting a patch.
 *
 * You can use the `-l` command line option or `api-level` modeline key to set
 * the API level for your `rc.lua`. This setting is global and read only,
 * individual modules cannot set their own API level.
 *
 * @tfield string api_level
 */

/**
 * The configuration file which has been loaded.
 * @tfield string conffile
 */

/**
 * True if we are still in startup, false otherwise.
 * @tfield boolean startup
 */

/**
 * Error message for errors that occurred during
 *  startup.
 * @tfield string startup_errors
 */

/**
 * True if a composite manager is running.
 * @tfield boolean composite_manager_running
 */

/**
 * Table mapping between signal numbers and signal identifiers.
 * @tfield table unix_signal
 */

/**
 * The hostname of the computer on which we are running.
 * @tfield string hostname
 */

/**
 * The path where themes were installed to.
 * @tfield string themes_path
 */

/**
 * The path where icons were installed to.
 * @tfield string icon_path
 */

static int
luaA_awesome_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    const char *buf = luaL_checkstring(L, 2);

    if(A_STREQ(buf, "conffile"))
    {
        lua_pushstring(L, conffile);
        return 1;
    }

    if(A_STREQ(buf, "version"))
    {
        lua_pushstring(L, awesome_version_string());
        return 1;
    }

    if(A_STREQ(buf, "release"))
    {
        lua_pushstring(L, awesome_release_string());
        return 1;
    }

    if(A_STREQ(buf, "api_level"))
    {
        lua_pushinteger(L, globalconf.api_level);
        return 1;
    }

    if(A_STREQ(buf, "startup"))
    {
        lua_pushboolean(L, globalconf.loop == NULL);
        return 1;
    }

    if(A_STREQ(buf, "_modifiers"))
    {
        luaA_get_modifiers(L);
        return 1;
    }

    if(A_STREQ(buf, "_active_modifiers"))
    {
        luaA_get_active_modifiers(L);
        return 1;
    }

    if(A_STREQ(buf, "startup_errors"))
    {
        if (globalconf.startup_errors.len == 0)
            return 0;
        lua_pushstring(L, globalconf.startup_errors.s);
        return 1;
    }

    if(A_STREQ(buf, "composite_manager_running"))
    {
        lua_pushboolean(L, composite_manager_running());
        return 1;
    }

    if(A_STREQ(buf, "hostname"))
    {
        /* No good way to handle failures... */
        char hostname[256] = "";
        gethostname(&hostname[0], countof(hostname));
        hostname[countof(hostname) - 1] = '\0';

        lua_pushstring(L, hostname);
        return 1;
    }

    if(A_STREQ(buf, "themes_path"))
    {
        lua_pushliteral(L, AWESOME_THEMES_PATH);
        return 1;
    }

    if(A_STREQ(buf, "icon_path"))
    {
        lua_pushliteral(L, AWESOME_ICON_PATH);
        return 1;
    }

    return luaA_default_index(L);
}

/** Add a global signal.
 *
 * @tparam string name A string with the event name.
 * @tparam function func The function to call.
 * @staticfct connect_signal
 */
static int
luaA_awesome_connect_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    signal_connect(&global_signals, name, luaA_object_ref(L, 2));
    return 0;
}

/** Remove a global signal.
 *
 * @tparam string name A string with the event name.
 * @tparam function func The function to call.
 * @staticfct disconnect_signal
 */
static int
luaA_awesome_disconnect_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    const void *func = lua_topointer(L, 2);
    if (signal_disconnect(&global_signals, name, func))
        luaA_object_unref(L, (void *) func);
    return 0;
}

/** Emit a global signal.
 *
 * @tparam function name A string with the event name.
 * @param ... The signal arguments.
 * @staticfct emit_signal
 */
static int
luaA_awesome_emit_signal(lua_State *L)
{
    signal_object_emit(L, &global_signals, luaL_checkstring(L, 1), lua_gettop(L) - 1);
    return 0;
}

static int
luaA_panic(lua_State *L)
{
    warn("unprotected error in call to Lua API (%s)",
         lua_tostring(L, -1));
    buffer_t buf;
    backtrace_get(&buf);
    warn("dumping backtrace\n%s", buf.s);
    warn("restarting awesome");
    awesome_restart();
    return 0;
}

#if LUA_VERSION_NUM >= 502
static const char *
luaA_tolstring(lua_State *L, int idx, size_t *len)
{
    return luaL_tolstring(L, idx, len);
}
#else
static const char *
luaA_tolstring(lua_State *L, int idx, size_t *len)
{
    /* Try using the metatable. If that fails, push the value itself onto
     * the stack.
     */
    if (!luaL_callmeta(L, idx, "__tostring"))
        lua_pushvalue(L, idx);

    switch (lua_type(L, -1)) {
    case LUA_TSTRING:
        lua_pushvalue(L, -1);
        break;
    case LUA_TBOOLEAN:
        if (lua_toboolean(L, -1))
            lua_pushliteral(L, "true");
        else
            lua_pushliteral(L, "false");
        break;
    case LUA_TNUMBER:
        lua_pushfstring(L, "%f", lua_tonumber(L, -1));
        break;
    case LUA_TNIL:
        lua_pushstring(L, "nil");
        break;
    default:
        lua_pushfstring(L, "%s: %p",
                lua_typename(L, lua_type(L, -1)),
                lua_topointer(L, -1));
        break;
    }
    lua_remove(L, -2);
    return lua_tolstring(L, -1, len);
}
#endif

static int
luaA_dofunction_on_error(lua_State *L)
{
    /* Convert error to string, to prevent a follow-up error with lua_concat. */
    luaA_tolstring(L, -1, NULL);

    /* duplicate string error */
    lua_pushvalue(L, -1);
    /* emit error signal */
    signal_object_emit(L, &global_signals, "debug::error", 1);

    if(!luaL_dostring(L, "return debug.traceback(\"error while running function!\", 3)"))
    {
        /* Move traceback before error */
        lua_insert(L, -2);
        /* Insert sentence */
        lua_pushliteral(L, "\nerror: ");
        /* Move it before error */
        lua_insert(L, -2);
        lua_concat(L, 3);
    }
    return 1;
}

static void
setup_awesome_signals(lua_State *L)
{
    lua_getglobal(L, "awesome");
    lua_pushstring(L, "unix_signal");
    lua_newtable(L);

#define SETUP_SIGNAL(sig)                             \
    do {                                              \
        /* Set awesome.unix_signal["SIGSTOP"] = 42 */ \
        lua_pushinteger(L, sig);                      \
        lua_setfield(L, -2, #sig);                    \
        /* Set awesome.unix_signal[42] = "SIGSTOP" */ \
        lua_pushinteger(L, sig);                      \
        lua_pushstring(L, #sig);                      \
        lua_settable(L, -3);                          \
    } while (0)

    /* Non-standard signals. These are first so that e.g. (on my system)
     * signals[29] is SIGPOLL and not SIGIO (the value gets overwritten).
     */
#ifdef SIGIOT
    SETUP_SIGNAL(SIGIOT);
#endif
#ifdef SIGEMT
    SETUP_SIGNAL(SIGEMT);
#endif
#ifdef SIGSTKFLT
    SETUP_SIGNAL(SIGSTKFLT);
#endif
#ifdef SIGIO
    SETUP_SIGNAL(SIGIO);
#endif
#ifdef SIGCLD
    SETUP_SIGNAL(SIGCLD);
#endif
#ifdef SIGPWR
    SETUP_SIGNAL(SIGPWR);
#endif
#ifdef SIGINFO
    SETUP_SIGNAL(SIGINFO);
#endif
#ifdef SIGLOST
    SETUP_SIGNAL(SIGLOST);
#endif
#ifdef SIGWINCH
    SETUP_SIGNAL(SIGWINCH);
#endif
#ifdef SIGUNUSED
    SETUP_SIGNAL(SIGUNUSED);
#endif

    /* POSIX.1-1990, according to man 7 signal */
    SETUP_SIGNAL(SIGHUP);
    SETUP_SIGNAL(SIGINT);
    SETUP_SIGNAL(SIGQUIT);
    SETUP_SIGNAL(SIGILL);
    SETUP_SIGNAL(SIGABRT);
    SETUP_SIGNAL(SIGFPE);
    SETUP_SIGNAL(SIGKILL);
    SETUP_SIGNAL(SIGSEGV);
    SETUP_SIGNAL(SIGPIPE);
    SETUP_SIGNAL(SIGALRM);
    SETUP_SIGNAL(SIGTERM);
    SETUP_SIGNAL(SIGUSR1);
    SETUP_SIGNAL(SIGUSR2);
    SETUP_SIGNAL(SIGCHLD);
    SETUP_SIGNAL(SIGCONT);
    SETUP_SIGNAL(SIGSTOP);
    SETUP_SIGNAL(SIGTSTP);
    SETUP_SIGNAL(SIGTTIN);
    SETUP_SIGNAL(SIGTTOU);

    /* POSIX.1-2001, according to man 7 signal */
    SETUP_SIGNAL(SIGBUS);
    /* Some Operating Systems doesn't have SIGPOLL (e.g. FreeBSD) */
#ifdef SIGPOLL
    SETUP_SIGNAL(SIGPOLL);
#endif
    SETUP_SIGNAL(SIGPROF);
    SETUP_SIGNAL(SIGSYS);
    SETUP_SIGNAL(SIGTRAP);
    SETUP_SIGNAL(SIGURG);
    SETUP_SIGNAL(SIGVTALRM);
    SETUP_SIGNAL(SIGXCPU);
    SETUP_SIGNAL(SIGXFSZ);

#undef SETUP_SIGNAL

    /* Set awesome.signal to the table we just created, key was already pushed */
    lua_rawset(L, -3);

    /* Pop "awesome" */
    lua_pop(L, 1);
}

/* Add things to the string on top of the stack */
static void
add_to_search_path(lua_State *L, string_array_t *searchpath, bool for_lua)
{
    if (LUA_TSTRING != lua_type(L, -1))
    {
        warn("package.path is not a string");
        return;
    }

    foreach(entry, *searchpath)
    {
        int components;
        size_t len = a_strlen(*entry);
        lua_pushliteral(L, ";");
        lua_pushlstring(L, *entry, len);
        if (for_lua)
            lua_pushliteral(L, "/?.lua");
        else
            lua_pushliteral(L, "/?.so");
        lua_concat(L, 3);

        if (for_lua)
        {
            lua_pushliteral(L, ";");
            lua_pushlstring(L, *entry, len);
            lua_pushliteral(L, "/?/init.lua");
            lua_concat(L, 3);

            components = 2;
        } else {
            components = 1;
        }
        lua_concat(L, components + 1); /* concatenate with string on top of the stack */
    }

    /* add Lua lib path (/usr/share/awesome/lib by default) */
    if (for_lua)
    {
        lua_pushliteral(L, ";" AWESOME_LUA_LIB_PATH "/?.lua");
        lua_pushliteral(L, ";" AWESOME_LUA_LIB_PATH "/?/init.lua");
        lua_concat(L, 3); /* concatenate with thing on top of the stack when we were called */
    } else {
        lua_pushliteral(L, ";" AWESOME_LUA_LIB_PATH "/?.so");
        lua_concat(L, 2); /* concatenate with thing on top of the stack when we were called */
    }
}

/** Initialize the Lua VM
 * \param xdg An xdg handle to use to get XDG basedir.
 */
void
luaA_init(xdgHandle* xdg, string_array_t *searchpath)
{
    lua_State *L;
    static const struct luaL_Reg awesome_lib[] =
    {
        { "quit", luaA_quit },
        { "exec", luaA_exec },
        { "spawn", luaA_spawn },
        { "restart", luaA_restart },
        { "connect_signal", luaA_awesome_connect_signal },
        { "disconnect_signal", luaA_awesome_disconnect_signal },
        { "emit_signal", luaA_awesome_emit_signal },
        { "systray", luaA_systray },
        { "load_image", luaA_load_image },
        { "pixbuf_to_surface", luaA_pixbuf_to_surface },
        { "set_preferred_icon_size", luaA_set_preferred_icon_size },
        { "register_xproperty", luaA_register_xproperty },
        { "set_xproperty", luaA_set_xproperty },
        { "get_xproperty", luaA_get_xproperty },
        { "__index", luaA_awesome_index },
        { "__newindex", luaA_default_newindex },
        { "xkb_set_layout_group", luaA_xkb_set_layout_group},
        { "xkb_get_layout_group", luaA_xkb_get_layout_group},
        { "xkb_get_group_names", luaA_xkb_get_group_names},
        { "xrdb_get_value", luaA_xrdb_get_value},
        { "kill", luaA_kill},
        { "sync", luaA_sync},
        { "_get_key_name", luaA_get_key_name},
        { NULL, NULL }
    };

    L = globalconf.L.real_L_dont_use_directly = luaL_newstate();

    /* Set panic function */
    lua_atpanic(L, luaA_panic);

    /* Set error handling function */
    lualib_dofunction_on_error = luaA_dofunction_on_error;

    luaL_openlibs(L);

    luaA_fixups(L);

    luaA_object_setup(L);

    /* Export awesome lib */
    luaA_openlib(L, "awesome", awesome_lib, awesome_lib);
    setup_awesome_signals(L);

    /* Export root lib */
    luaA_openlib(L, "root", awesome_root_methods, awesome_root_meta);

#ifdef WITH_DBUS
    /* Export D-Bus lib */
    luaA_registerlib(L, "dbus", awesome_dbus_lib);
    lua_pop(L, 1); /* luaA_registerlib() leaves the table on stack */
#endif

    /* Export keygrabber lib */
    luaA_registerlib(L, "keygrabber", awesome_keygrabber_lib);
    lua_pop(L, 1); /* luaA_registerlib() leaves the table on stack */

    /* Export mousegrabber lib */
    luaA_registerlib(L, "mousegrabber", awesome_mousegrabber_lib);
    lua_pop(L, 1); /* luaA_registerlib() leaves the table on stack */

    /* Export mouse */
    luaA_openlib(L, "mouse", awesome_mouse_methods, awesome_mouse_meta);

    /* Export screen */
    screen_class_setup(L);

    /* Export button */
    button_class_setup(L);

    /* Export tag */
    tag_class_setup(L);

    /* Export window */
    window_class_setup(L);

    /* Export drawable */
    drawable_class_setup(L);

    /* Export drawin */
    drawin_class_setup(L);

    /* Export client */
    client_class_setup(L);

    /* Export selection getter */
    selection_getter_class_setup(L);

    /* Export keys */
    key_class_setup(L);

    /* Export selection acquire */
    selection_acquire_class_setup(L);

    /* Export selection transfer */
    selection_transfer_class_setup(L);

    /* Export selection watcher */
    selection_watcher_class_setup(L);

    /* Setup the selection interface */
    selection_setup(L);

    /* add Lua search paths */
    lua_getglobal(L, "package");
    if (LUA_TTABLE != lua_type(L, 1))
    {
        warn("package is not a table");
        return;
    }
    lua_getfield(L, 1, "path");
    add_to_search_path(L, searchpath, true);
    lua_setfield(L, 1, "path"); /* package.path = "concatenated string" */

    lua_getfield(L, 1, "cpath");
    add_to_search_path(L, searchpath, false);
    lua_setfield(L, 1, "cpath"); /* package.cpath = "concatenated string" */

    lua_pop(L, 1); /* pop "package" */
}

static void
luaA_startup_error(const char *err)
{
    if (globalconf.startup_errors.len > 0)
        buffer_addsl(&globalconf.startup_errors, "\n\n");
    buffer_adds(&globalconf.startup_errors, err);
}

static bool
luaA_loadrc(const char *confpath)
{
    lua_State *L = globalconf_get_lua_State();
    if(luaL_loadfile(L, confpath))
    {
        const char *err = lua_tostring(L, -1);
        luaA_startup_error(err);
        fprintf(stderr, "%s\n", err);
        lua_pop(L, 1);
        return false;
    }

    /* Set the conffile right now so it can be used inside the
     * configuration file. */
    conffile = a_strdup(confpath);
    /* Move error handling function before function */
    lua_pushcfunction(L, luaA_dofunction_on_error);
    lua_insert(L, -2);
    if(!lua_pcall(L, 0, 0, -2))
    {
        /* Pop luaA_dofunction_on_error */
        lua_pop(L, 1);
        return true;
    }

    const char *err = lua_tostring(L, -1);
    luaA_startup_error(err);
    fprintf(stderr, "%s\n", err);
    /* An error happened, so reset this. */
    conffile = NULL;
    /* Pop luaA_dofunction_on_error() and the error message */
    lua_pop(L, 2);

    return false;
}

/** Load a configuration file.
 * \param xdg An xdg handle to use to get XDG basedir.
 * \param confpatharg The configuration file to load.
 * \param run Run the configuration file.
 */
bool
luaA_parserc(xdgHandle* xdg, const char *confpatharg)
{
    const char *confpath = luaA_find_config(xdg, confpatharg, luaA_loadrc);
    bool ret = confpath != NULL;
    p_delete(&confpath);

    return ret;
}

/** Find a config file for which the given callback returns true.
 * \param xdg An xdg handle to use to get XDG basedir.
 * \param confpatharg The configuration file to load.
 * \param callback The callback to call.
 */
const char *
luaA_find_config(xdgHandle* xdg, const char *confpatharg, luaA_config_callback *callback)
{
    char *confpath = NULL;

    if(confpatharg && callback(confpatharg))
    {
        return a_strdup(confpatharg);
    }

    confpath = xdgConfigFind("awesome/rc.lua", xdg);

    char *tmp = confpath;

    /* confpath is "string1\0string2\0string3\0\0" */
    while(*tmp)
    {
        if(callback(tmp))
        {
            const char *ret = a_strdup(tmp);
            p_delete(&confpath);
            return ret;
        }
        tmp += a_strlen(tmp) + 1;
    }
    p_delete(&confpath);

    if(callback(AWESOME_DEFAULT_CONF))
    {
        return a_strdup(AWESOME_DEFAULT_CONF);
    }

    return NULL;
}

int
luaA_class_index_miss_property(lua_State *L, lua_object_t *obj)
{
    signal_object_emit(L, &global_signals, "debug::index::miss", 2);
    return 0;
}

int
luaA_class_newindex_miss_property(lua_State *L, lua_object_t *obj)
{
    signal_object_emit(L, &global_signals, "debug::newindex::miss", 3);
    return 0;
}

void
luaA_emit_startup()
{
    lua_State *L = globalconf_get_lua_State();
    signal_object_emit(L, &global_signals, "startup", 0);
}

void
luaA_emit_refresh()
{
    lua_State *L = globalconf_get_lua_State();
    signal_object_emit(L, &global_signals, "refresh", 0);
}

int
luaA_default_index(lua_State *L)
{
    return luaA_class_index_miss_property(L, NULL);
}

int
luaA_default_newindex(lua_State *L)
{
    return luaA_class_newindex_miss_property(L, NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
