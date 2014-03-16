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

#define _GNU_SOURCE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <basedir_fs.h>

#include <xcb/xcb_atom.h>

#include "awesome.h"
#include "config.h"
#include "objects/timer.h"
#include "awesome-version-internal.h"
#include "ewmh.h"
#include "luaa.h"
#include "spawn.h"
#include "objects/tag.h"
#include "objects/client.h"
#include "objects/drawin.h"
#include "objects/drawable.h"
#include "screen.h"
#include "event.h"
#include "property.h"
#include "selection.h"
#include "systray.h"
#include "common/xcursor.h"
#include "common/buffer.h"
#include "common/backtrace.h"

#ifdef WITH_DBUS
extern const struct luaL_Reg awesome_dbus_lib[];
#endif
extern const struct luaL_Reg awesome_keygrabber_lib[];
extern const struct luaL_Reg awesome_mousegrabber_lib[];
extern const struct luaL_Reg awesome_root_lib[];
extern const struct luaL_Reg awesome_mouse_methods[];
extern const struct luaL_Reg awesome_mouse_meta[];
extern const struct luaL_Reg awesome_screen_methods[];
extern const struct luaL_Reg awesome_screen_meta[];

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
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_quit(lua_State *L)
{
    g_main_loop_quit(globalconf.loop);
    return 0;
}

/** Execute another application, probably a window manager, to replace
 * awesome.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The command line to execute.
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
 */
static int
luaA_restart(lua_State *L)
{
    awesome_restart();
    return 0;
}

/** Load an image from a given path.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam The command line to execute.
 */
static int
luaA_load_image(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    cairo_surface_t *surface = draw_load_image(L, filename);
    if (!surface)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surface);
    return 1;
}

/** UTF-8 aware string length computing.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_mbstrlen(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);
    lua_pushnumber(L, (ssize_t) mbstowcs(NULL, NONULL(cmd), 0));
    return 1;
}

/** Overload standard Lua next function to use __next key on metatable.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaAe_next(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__next"))
    {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2);
    if(lua_next(L, 1))
        return 2;
    lua_pushnil(L);
    return 1;
}

/** Overload lua_next() function by using __next metatable field
 * to get next elements.
 * \param L The Lua VM stack.
 * \param idx The index number of elements in stack.
 * \return 1 if more elements to come, 0 otherwise.
 */
static int
luaA_next(lua_State *L, int idx)
{
    if(luaL_getmetafield(L, idx, "__next"))
    {
        /* if idx is relative, reduce it since we got __next */
        if(idx < 0) idx--;
        /* copy table and then move key */
        lua_pushvalue(L, idx);
        lua_pushvalue(L, -3);
        lua_remove(L, -4);
        lua_pcall(L, 2, 2, 0);
        /* next returned nil, it's the end */
        if(lua_isnil(L, -1))
        {
            /* remove nil */
            lua_pop(L, 2);
            return 0;
        }
        return 1;
    }
    else if(lua_istable(L, idx))
        return lua_next(L, idx);
    /* remove the key */
    lua_pop(L, 1);
    return 0;
}

/** Generic pairs function.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_generic_pairs(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));  /* return generator, */
    lua_pushvalue(L, 1);  /* state, */
    lua_pushnil(L);  /* and initial value */
    return 3;
}

/** Overload standard pairs function to use __pairs field of metatables.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaAe_pairs(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__pairs"))
    {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    return luaA_generic_pairs(L);
}

static int
luaA_ipairs_aux(lua_State *L)
{
    int i = luaL_checkint(L, 2) + 1;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushinteger(L, i);
    lua_rawgeti(L, 1, i);
    return (lua_isnil(L, -1)) ? 0 : 2;
}

/** Overload standard ipairs function to use __ipairs field of metatables.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaAe_ipairs(lua_State *L)
{
    if(luaL_getmetafield(L, 1, "__ipairs"))
    {
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
        return lua_gettop(L);
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);  /* and initial value */
    return 3;
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
    /* replace next */
    lua_pushcfunction(L, luaAe_next);
    lua_setglobal(L, "next");
    /* replace pairs */
    lua_pushcfunction(L, luaAe_next);
    lua_pushcclosure(L, luaAe_pairs, 1); /* pairs get next as upvalue */
    lua_setglobal(L, "pairs");
    /* replace ipairs */
    lua_pushcfunction(L, luaA_ipairs_aux);
    lua_pushcclosure(L, luaAe_ipairs, 1);
    lua_setglobal(L, "ipairs");
    /* replace type */
    lua_pushcfunction(L, luaAe_type);
    lua_setglobal(L, "type");
    /* set selection */
    lua_pushcfunction(L, luaA_selection_get);
    lua_setglobal(L, "selection");
}

/** Look for an item: table, function, etc.
 * \param L The Lua VM state.
 * \param item The pointer item.
 */
bool
luaA_hasitem(lua_State *L, const void *item)
{
    lua_pushnil(L);
    while(luaA_next(L, -2))
    {
        if(lua_topointer(L, -1) == item)
        {
            /* remove value and key */
            lua_pop(L, 2);
            return true;
        }
        if(lua_istable(L, -1))
            if(luaA_hasitem(L, item))
            {
                /* remove key and value */
                lua_pop(L, 2);
                return true;
            }
        /* remove value */
        lua_pop(L, 1);
    }
    return false;
}

/** Browse a table pushed on top of the index, and put all its table and
 * sub-table into an array.
 * \param L The Lua VM state.
 * \param elems The elements array.
 * \return False if we encounter an elements already in list.
 */
static bool
luaA_isloop_check(lua_State *L, cptr_array_t *elems)
{
    if(lua_istable(L, -1))
    {
        const void *object = lua_topointer(L, -1);

        /* Check that the object table is not already in the list */
        for(int i = 0; i < elems->len; i++)
            if(elems->tab[i] == object)
                return false;

        /* push the table in the elements list */
        cptr_array_append(elems, object);

        /* look every object in the "table" */
        lua_pushnil(L);
        while(luaA_next(L, -2))
        {
            if(!luaA_isloop_check(L, elems))
            {
                /* remove key and value */
                lua_pop(L, 2);
                return false;
            }
            /* remove value, keep key for next iteration */
            lua_pop(L, 1);
        }
    }
    return true;
}

/** Check if a table is a loop. When using tables as direct acyclic digram,
 * this is useful.
 * \param L The Lua VM state.
 * \param idx The index of the table in the stack
 * \return True if the table loops.
 */
bool
luaA_isloop(lua_State *L, int idx)
{
    /* elems is an elements array that we will fill with all array we
     * encounter while browsing the tables */
    cptr_array_t elems;

    cptr_array_init(&elems);

    /* push table on top */
    lua_pushvalue(L, idx);

    bool ret = luaA_isloop_check(L, &elems);

    /* remove pushed table */
    lua_pop(L, 1);

    cptr_array_wipe(&elems);

    return !ret;
}

/** awesome global table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield conffile The configuration file which has been loaded.
 * \lfield version The version of awesome.
 * \lfield release The release name of awesome.
 * \lfield startup True if we are still in startup, false otherwise.
 * \lfield startup_errors Error message for errors that occured during startup.
 * \lfield composite_manager_running True if a composite manager is running.
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
        lua_pushliteral(L, AWESOME_VERSION);
        return 1;
    }

    if(A_STREQ(buf, "release"))
    {
        lua_pushliteral(L, AWESOME_RELEASE);
        return 1;
    }

    if(A_STREQ(buf, "startup"))
    {
        lua_pushboolean(L, globalconf.loop == NULL);
        return 1;
    }

    if(A_STREQ(buf, "startup_errors") && globalconf.startup_errors.len != 0)
    {
        lua_pushstring(L, globalconf.startup_errors.s);
        return 1;
    }

    if(A_STREQ(buf, "composite_manager_running"))
    {
        lua_pushboolean(L, composite_manager_running());
        return 1;
    }

    if(A_STREQ(buf, "honor_ewmh_desktop")) {
        lua_pushboolean(L, globalconf.honor_ewmh_desktop);
        return 1;
    }

    return 0;
}

/** awesome global table setter.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield a variable from globalconf
 */
static int
luaA_awesome_new_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    const char *buf = luaL_checkstring(L, 2);

    if(A_STREQ(buf, "honor_ewmh_desktop") && lua_isboolean(L,3))
    {
        const int honor = lua_toboolean(L, 3);
        globalconf.honor_ewmh_desktop = honor;
    }
    return 0;
}

/** Add a global signal.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
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
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
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
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with the event name.
 * \lparam The function to call.
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

static int
luaA_dofunction_on_error(lua_State *L)
{
    /* duplicate string error */
    lua_pushvalue(L, -1);
    /* emit error signal */
    signal_object_emit(L, &global_signals, "debug::error", 1);

    if(!luaL_dostring(L, "return debug.traceback(\"error while running function\", 3)"))
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
        { "register_xproperty", luaA_register_xproperty },
        { "__index", luaA_awesome_index },
        { "__newindex", luaA_awesome_new_index },
        { NULL, NULL }
    };

    L = globalconf.L = luaL_newstate();

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

    /* Export screen */
    luaA_openlib(L, "screen", awesome_screen_methods, awesome_screen_meta);

    /* Export mouse */
    luaA_openlib(L, "mouse", awesome_mouse_methods, awesome_mouse_meta);

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

    /* Export timer */
    timer_class_setup(L);

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

    signal_add(&global_signals, "debug::error");
    signal_add(&global_signals, "debug::deprecation");
    signal_add(&global_signals, "debug::index::miss");
    signal_add(&global_signals, "debug::newindex::miss");
    signal_add(&global_signals, "systray::update");
    signal_add(&global_signals, "wallpaper_changed");
    signal_add(&global_signals, "refresh");
    signal_add(&global_signals, "exit");
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
    if(!luaL_loadfile(globalconf.L, confpath))
    {
        if(run)
        {
            /* Set the conffile right now so it can be used inside the
             * configuration file. */
            conffile = a_strdup(confpath);
            /* Move error handling function before function */
            lua_pushcfunction(globalconf.L, luaA_dofunction_on_error);
            lua_insert(globalconf.L, -2);
            if(lua_pcall(globalconf.L, 0, LUA_MULTRET, -2))
            {
                const char *err = lua_tostring(globalconf.L, -1);
                luaA_startup_error(err);
                fprintf(stderr, "%s\n", err);
                /* An error happened, so reset this. */
                conffile = NULL;
            }
            else
                return true;
        }
        else
        {
            lua_pop(globalconf.L, 1);
            return true;
        }
    }
    else
    {
        const char *err = lua_tostring(globalconf.L, -1);
        luaA_startup_error(err);
        fprintf(stderr, "%s\n", err);
    }

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
luaA_emit_refresh()
{
    signal_object_emit(globalconf.L, &global_signals, "refresh", 0);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
