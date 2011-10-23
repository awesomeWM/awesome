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

#include <ev.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <basedir_fs.h>

#include "awesome.h"
#include "config.h"
#include "timer.h"
#include "awesome-version-internal.h"
#include "ewmh.h"
#include "luaa.h"
#include "spawn.h"
#include "tag.h"
#include "client.h"
#include "screen.h"
#include "event.h"
#include "selection.h"
#include "window.h"
#include "common/xcursor.h"
#include "common/buffer.h"

#ifdef WITH_DBUS
extern const struct luaL_reg awesome_dbus_lib[];
#endif
extern const struct luaL_reg awesome_hooks_lib[];
extern const struct luaL_reg awesome_keygrabber_lib[];
extern const struct luaL_reg awesome_mousegrabber_lib[];
extern const struct luaL_reg awesome_root_lib[];
extern const struct luaL_reg awesome_mouse_methods[];
extern const struct luaL_reg awesome_mouse_meta[];
extern const struct luaL_reg awesome_screen_methods[];
extern const struct luaL_reg awesome_screen_meta[];

/** Quit awesome.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_quit(lua_State *L)
{
    ev_unloop(globalconf.loop, 1);
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
int
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

/** Modified version of os.execute() which use spawn code to reset signal
 * handlers correctly before exec()'ing the new program.
 * \param L The Lua VM state.
 * \return The number of arguments pushed on stack.
 */
static int
luaAe_os_execute(lua_State *L)
{
    lua_pushinteger(L, spawn_system(luaL_optstring(L, 1, NULL)));
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
    /* replace os.execute */
    lua_getglobal(L, "os");
    lua_pushcfunction(L, luaAe_os_execute);
    lua_setfield(L, -2, "execute");
    lua_pop(L, 1);
    /* replace next */
    lua_pushliteral(L, "next");
    lua_pushcfunction(L, luaAe_next);
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace pairs */
    lua_pushliteral(L, "pairs");
    lua_pushcfunction(L, luaAe_next);
    lua_pushcclosure(L, luaAe_pairs, 1); /* pairs get next as upvalue */
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace ipairs */
    lua_pushliteral(L, "ipairs");
    lua_pushcfunction(L, luaA_ipairs_aux);
    lua_pushcclosure(L, luaAe_ipairs, 1);
    lua_settable(L, LUA_GLOBALSINDEX);
    /* replace type */
    lua_pushliteral(L, "type");
    lua_pushcfunction(L, luaAe_type);
    lua_settable(L, LUA_GLOBALSINDEX);
    /* set selection */
    lua_pushliteral(L, "selection");
    lua_pushcfunction(L, luaA_selection_get);
    lua_settable(L, LUA_GLOBALSINDEX);
}

/** __next function for wtable objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wtable_next(lua_State *L)
{
    /* upvalue 1 is content table */
    if(lua_next(L, lua_upvalueindex(1)))
        return 2;
    lua_pushnil(L);
    return 1;
}

/** __ipairs function for wtable objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wtable_ipairs(lua_State *L)
{
    /* push ipairs_aux */
    lua_pushvalue(L, lua_upvalueindex(2));
    /* push content table */
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushinteger(L, 0);  /* and initial value */
    return 3;
}

/** Index function of wtable objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wtable_index(lua_State *L)
{
    size_t len;
    const char *buf;

    lua_pushvalue(L, 2);
    /* check for size, waiting lua 5.2 and __len on tables */
    if((buf = lua_tolstring(L, -1, &len)))
        if(a_tokenize(buf, len) == A_TK_LEN)
        {
            lua_pushnumber(L, lua_objlen(L, lua_upvalueindex(1)));
            return 1;
        }
    lua_pop(L, 1);

    /* upvalue 1 is content table */
    lua_rawget(L, lua_upvalueindex(1));
    return 1;
}

/** Newindex function of wtable objects.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wtable_newindex(lua_State *L)
{
    bool invalid = false;

    /* push key on top */
    lua_pushvalue(L, 2);
    /* get current key value in content table */
    lua_rawget(L, lua_upvalueindex(1));
    /* if value is a widget, notify change */
    if(lua_istable(L, -1) || luaA_toudata(L, -1, &widget_class))
        invalid = true;

    lua_pop(L, 1); /* remove value */

    /* if new value is a widget or a table */
    if(lua_istable(L, 3))
    {
        luaA_table2wtable(L);
        invalid = true;
    }
    else if(!invalid && luaA_toudata(L, 3, &widget_class))
        invalid = true;

    /* upvalue 1 is content table */
    lua_rawset(L, lua_upvalueindex(1));

    if(invalid)
        luaA_wibox_invalidate_byitem(L, lua_topointer(L, 1));

    return 0;
}

/** Convert the top element of the stack to a proxied wtable.
 * \param L The Lua VM state.
 */
void
luaA_table2wtable(lua_State *L)
{
    if(!lua_istable(L, -1))
        return;

    lua_newtable(L); /* create *real* content table */
    lua_createtable(L, 0, 5); /* metatable */
    lua_pushvalue(L, -2); /* copy content table */
    lua_pushcfunction(L, luaA_ipairs_aux); /* push ipairs aux */
    lua_pushcclosure(L, luaA_wtable_ipairs, 2);
    lua_pushvalue(L, -3); /* copy content table */
    lua_pushcclosure(L, luaA_wtable_next, 1); /* __next has the content table as upvalue */
    lua_pushvalue(L, -4); /* copy content table */
    lua_pushcclosure(L, luaA_wtable_index, 1); /* __index has the content table as upvalue */
    lua_pushvalue(L, -5); /* copy content table */
    lua_pushcclosure(L, luaA_wtable_newindex, 1); /* __newindex has the content table as upvalue */
    /* set metatable field with just pushed closure */
    lua_setfield(L, -5, "__newindex");
    lua_setfield(L, -4, "__index");
    lua_setfield(L, -3, "__next");
    lua_setfield(L, -2, "__ipairs");
    /* set metatable impossible to touch */
    lua_pushliteral(L, "wtable");
    lua_setfield(L, -2, "__metatable");
    /* set new metatable on original table */
    lua_setmetatable(L, -3);

    /* initial key */
    lua_pushnil(L);
    /* go through original table */
    while(lua_next(L, -3))
    {
        /* if convert value to wtable */
        luaA_table2wtable(L);
        /* copy key */
        lua_pushvalue(L, -2);
        /* move key before value */
        lua_insert(L, -2);
        /* set same value in content table */
        lua_rawset(L, -4);
        /* copy key */
        lua_pushvalue(L, -1);
        /* push the new value :-> */
        lua_pushnil(L);
        /* set orig[k] = nil */
        lua_rawset(L, -5);
    }
    /* remove content table */
    lua_pop(L, 1);
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
 * \lfield font The default font.
 * \lfield font_height The default font height.
 * \lfield conffile The configuration file which has been loaded.
 */
static int
luaA_awesome_index(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(buf, len))
    {
      case A_TK_FONT:
        {
            char *font = pango_font_description_to_string(globalconf.font->desc);
            lua_pushstring(L, font);
            g_free(font);
        }
        break;
      case A_TK_FONT_HEIGHT:
        lua_pushnumber(L, globalconf.font->height);
        break;
      case A_TK_CONFFILE:
        lua_pushstring(L, globalconf.conffile);
        break;
      case A_TK_FG:
        luaA_pushxcolor(L, globalconf.colors.fg);
        break;
      case A_TK_BG:
        luaA_pushxcolor(L, globalconf.colors.bg);
        break;
      case A_TK_VERSION:
        lua_pushliteral(L, AWESOME_VERSION);
        break;
      case A_TK_RELEASE:
        lua_pushliteral(L, AWESOME_RELEASE);
        break;
      case A_TK_STARTUP_ERRORS:
        if (globalconf.startup_errors.len == 0)
            return 0;
        lua_pushstring(L, globalconf.startup_errors.s);
      default:
        return 0;
    }

    return 1;
}

/** Newindex function for the awesome global table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_awesome_newindex(lua_State *L)
{
    if(luaA_usemetatable(L, 1, 2))
        return 1;

    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(buf, len))
    {
      case A_TK_FONT:
        {
            const char *newfont = luaL_checkstring(L, 3);
            draw_font_delete(&globalconf.font);
            globalconf.font = draw_font_new(newfont);
            /* refresh all wiboxes */
            foreach(wibox, globalconf.wiboxes)
                (*wibox)->need_update = true;
            foreach(c, globalconf.clients)
                if((*c)->titlebar)
                    (*c)->titlebar->need_update = true;
        }
        break;
      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
           xcolor_init_reply(xcolor_init_unchecked(&globalconf.colors.fg, buf, len));
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
           xcolor_init_reply(xcolor_init_unchecked(&globalconf.colors.bg, buf, len));
        break;
      default:
        return 0;
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
luaA_awesome_add_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    signal_add(&global_signals, name, luaA_object_ref(L, 2));
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
luaA_awesome_remove_signal(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaA_checkfunction(L, 2);
    const void *func = lua_topointer(L, 2);
    signal_remove(&global_signals, name, func);
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
    warn("unprotected error in call to Lua API (%s), restarting awesome",
         lua_tostring(L, -1));
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
    static const struct luaL_reg awesome_lib[] =
    {
        { "quit", luaA_quit },
        { "exec", luaA_exec },
        { "spawn", luaA_spawn },
        { "restart", luaA_restart },
        { "add_signal", luaA_awesome_add_signal },
        { "remove_signal", luaA_awesome_remove_signal },
        { "emit_signal", luaA_awesome_emit_signal },
        { "__index", luaA_awesome_index },
        { "__newindex", luaA_awesome_newindex },
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
    luaL_register(L, "root", awesome_root_lib);
    lua_pop(L, 1); /* luaL_register() leaves the table on stack */

    /* Export hooks lib */
    luaL_register(L, "hooks", awesome_hooks_lib);
    lua_pop(L, 1); /* luaL_register() leaves the table on stack */

#ifdef WITH_DBUS
    /* Export D-Bus lib */
    luaL_register(L, "dbus", awesome_dbus_lib);
    lua_pop(L, 1); /* luaL_register() leaves the table on stack */
#endif

    /* Export keygrabber lib */
    luaL_register(L, "keygrabber", awesome_keygrabber_lib);
    lua_pop(L, 1); /* luaL_register() leaves the table on stack */

    /* Export mousegrabber lib */
    luaL_register(L, "mousegrabber", awesome_mousegrabber_lib);
    lua_pop(L, 1); /* luaL_register() leaves the table on stack */

    /* Export screen */
    luaA_openlib(L, "screen", awesome_screen_methods, awesome_screen_meta);

    /* Export mouse */
    luaA_openlib(L, "mouse", awesome_mouse_methods, awesome_mouse_meta);

    /* Export button */
    button_class_setup(L);

    /* Export image */
    image_class_setup(L);

    /* Export tag */
    tag_class_setup(L);

    /* Export wibox */
    wibox_class_setup(L);

    /* Export widget */
    widget_class_setup(L);

    /* Export client */
    client_class_setup(L);

    /* Export keys */
    key_class_setup(L);

    /* Export timer */
    timer_class_setup(L);

    /* init hooks */
    globalconf.hooks.manage = LUA_REFNIL;
    globalconf.hooks.unmanage = LUA_REFNIL;
    globalconf.hooks.focus = LUA_REFNIL;
    globalconf.hooks.unfocus = LUA_REFNIL;
    globalconf.hooks.mouse_enter = LUA_REFNIL;
    globalconf.hooks.mouse_leave = LUA_REFNIL;
    globalconf.hooks.clients = LUA_REFNIL;
    globalconf.hooks.tags = LUA_REFNIL;
    globalconf.hooks.tagged = LUA_REFNIL;
    globalconf.hooks.property = LUA_REFNIL;
    globalconf.hooks.timer = LUA_REFNIL;
    globalconf.hooks.exit = LUA_REFNIL;

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
            globalconf.conffile = a_strdup(confpath);
            if(lua_pcall(globalconf.L, 0, LUA_MULTRET, 0))
            {
                const char *err = lua_tostring(globalconf.L, -1);
                luaA_startup_error(err);
                fprintf(stderr, "%s\n", err);
                /* An error happened, so reset this. */
                globalconf.conffile = NULL;
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

void
luaA_on_timer(EV_P_ ev_timer *w, int revents)
{
    if(globalconf.hooks.timer != LUA_REFNIL)
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.timer, 0, 0);
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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
