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

/** awesome core API
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @release @AWESOME_VERSION@
 * @module awesome
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
#include "objects/screen.h"
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

#include <basedir_fs.h>

#include <xcb/xcb_atom.h>

#ifdef WITH_DBUS
extern const struct luaL_Reg awesome_dbus_lib[];
#endif
extern const struct luaL_Reg awesome_keygrabber_lib[];
extern const struct luaL_Reg awesome_mousegrabber_lib[];
extern const struct luaL_Reg awesome_root_lib[];
extern const struct luaL_Reg awesome_mouse_methods[];
extern const struct luaL_Reg awesome_mouse_meta[];

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
 * @function quit
 */
static int
luaA_quit(lua_State *L)
{
    if (globalconf.loop == NULL)
        globalconf.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_quit(globalconf.loop);
    return 0;
}

/** Execute another application, probably a window manager, to replace
 * awesome.
 *
 * @param cmd The command line to execute.
 * @function exec
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
 * @function restart
 */
static int
luaA_restart(lua_State *L)
{
    awesome_restart();
    return 0;
}

/** Load an image from a given path.
 *
 * @param name The file name.
 * @return[1] A cairo surface as light user datum.
 * @return[2] nil
 * @treturn[2] string Error message
 * @function load_image
 */
static int
luaA_load_image(lua_State *L)
{
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
 * @param size The size of the icons in pixels.
 * @function set_preferred_icon_size
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
    /* set selection */
    lua_pushcfunction(L, luaA_selection_get);
    lua_setglobal(L, "selection");
}

/** awesome global table.
 * @field version The version of awesome.
 * @field release The release name of awesome.
 * @field conffile The configuration file which has been loaded.
 * @field startup True if we are still in startup, false otherwise.
 * @field startup_errors Error message for errors that occured during
 *  startup.
 * @field composite_manager_running True if a composite manager is running.
 * @table awesome
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

    if(A_STREQ(buf, "startup"))
    {
        lua_pushboolean(L, globalconf.loop == NULL);
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

    return luaA_default_index(L);
}

/** Add a global signal.
 *
 * @param name A string with the event name.
 * @param func The function to call.
 * @function connect_signal
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
 * @param name A string with the event name.
 * @param func The function to call.
 * @function disconnect_signal
 */
static int
luaA_awesome_disconnect_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    const void *func = lua_topointer(L, 2);
    signal_disconnect(&global_signals, name, func);
    luaA_object_unref(L, (void *) func);
    return 0;
}

/** Emit a global signal.
 *
 * @param name A string with the event name.
 * @param ... The signal arguments.
 * @function emit_signal
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

/** Initialize the Lua VM
 * \param xdg An xdg handle to use to get XDG basedir.
 */
void
luaA_init(xdgHandle* xdg)
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

    /* Export root lib */
    luaA_registerlib(L, "root", awesome_root_lib);
    lua_pop(L, 1); /* luaA_registerlib() leaves the table on stack */

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

    /* Export keys */
    key_class_setup(L);

    /* add Lua search paths */
    lua_getglobal(L, "package");
    if (LUA_TTABLE != lua_type(L, 1))
    {
        warn("package is not a table");
        return;
    }
    lua_getfield(L, 1, "path");
    if (LUA_TSTRING != lua_type(L, 2))
    {
        warn("package.path is not a string");
        lua_pop(L, 1);
        return;
    }

    /* add XDG_CONFIG_DIR as include path */
    const char * const *xdgconfigdirs = xdgSearchableConfigDirectories(xdg);
    for(; *xdgconfigdirs; xdgconfigdirs++)
    {
        size_t len = a_strlen(*xdgconfigdirs);
        lua_pushliteral(L, ";");
        lua_pushlstring(L, *xdgconfigdirs, len);
        lua_pushliteral(L, "/awesome/?.lua");
        lua_concat(L, 3);

        lua_pushliteral(L, ";");
        lua_pushlstring(L, *xdgconfigdirs, len);
        lua_pushliteral(L, "/awesome/?/init.lua");
        lua_concat(L, 3);

        lua_concat(L, 3); /* concatenate with package.path */
    }

    /* add Lua lib path (/usr/share/awesome/lib by default) */
    lua_pushliteral(L, ";" AWESOME_LUA_LIB_PATH "/?.lua");
    lua_pushliteral(L, ";" AWESOME_LUA_LIB_PATH "/?/init.lua");
    lua_concat(L, 3); /* concatenate with package.path */
    lua_setfield(L, 1, "path"); /* package.path = "concatenated string" */

    lua_getfield(L, 1, "loaded");

    lua_pop(L, 2); /* pop "package" and "package.loaded" */

    /** A call into the lua code aborted with an error
     * @signal debug::error
     */
    signal_add(&global_signals, "debug::error");
    /** A deprecated lua function was called
     * @signal debug::deprecation
     */
    signal_add(&global_signals, "debug::deprecation");
    /** An invalid key was read from an object (e.g. c.foo)
     * @signal debug::index::miss
     */
    signal_add(&global_signals, "debug::index::miss");
    /** An invalid key was written to an object (e.g. c.foo = "bar")
     * @signal debug::newindex::miss
     */
    signal_add(&global_signals, "debug::newindex::miss");
    /**
     * @signal systray::update
     */
    signal_add(&global_signals, "systray::update");
    /**
     * @signal wallpaper_changed
     */
    signal_add(&global_signals, "wallpaper_changed");
    /**
     * @signal xkb::map_changed
     */
    signal_add(&global_signals, "xkb::map_changed");
    /**
     * @signal xkb::group_changed
     */
    signal_add(&global_signals, "xkb::group_changed");
    /**
     * @signal refresh
     */
    signal_add(&global_signals, "refresh");
    /**
     * @signal startup
     */
    signal_add(&global_signals, "startup");
    /**
     * @signal exit
     */
    signal_add(&global_signals, "exit");
    /**
     * @signal screen::change
     */
    signal_add(&global_signals, "screen::change");
}

static void
luaA_startup_error(const char *err)
{
    if (globalconf.startup_errors.len > 0)
        buffer_addsl(&globalconf.startup_errors, "\n\n");
    buffer_adds(&globalconf.startup_errors, err);
}

static bool
luaA_loadrc(const char *confpath, bool run)
{
    lua_State *L = globalconf_get_lua_State();
    if(luaL_loadfile(L, confpath))
    {
        const char *err = lua_tostring(L, -1);
        luaA_startup_error(err);
        fprintf(stderr, "%s\n", err);
        return false;
    }

    if(!run)
    {
        lua_pop(L, 1);
        return true;
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
luaA_parserc(xdgHandle* xdg, const char *confpatharg, bool run)
{
    char *confpath = NULL;
    bool ret = false;

    /* try to load, return if it's ok */
    if(confpatharg)
    {
        if(luaA_loadrc(confpatharg, run))
        {
            ret = true;
            goto bailout;
        }
        else if(!run)
            goto bailout;
    }

    confpath = xdgConfigFind("awesome/rc.lua", xdg);

    char *tmp = confpath;

    /* confpath is "string1\0string2\0string3\0\0" */
    while(*tmp)
    {
        if(luaA_loadrc(tmp, run))
        {
            ret = true;
            goto bailout;
        }
        else if(!run)
            goto bailout;
        tmp += a_strlen(tmp) + 1;
    }

bailout:

    p_delete(&confpath);

    return ret;
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
