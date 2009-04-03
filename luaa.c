/*
 * lua.c - Lua configuration management
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

#include "common/util.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <ev.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <xcb/xcb.h>

#include "awesome.h"
#include "awesome-version-internal.h"
#include "ewmh.h"
#include "config.h"
#include "luaa.h"
#include "tag.h"
#include "client.h"
#include "screen.h"
#include "event.h"
#include "mouse.h"
#include "selection.h"
#include "window.h"
#include "common/socket.h"
#include "common/buffer.h"

extern awesome_t globalconf;

extern const struct luaL_reg awesome_hooks_lib[];
extern const struct luaL_reg awesome_dbus_lib[];
extern const struct luaL_reg awesome_keygrabber_lib[];
extern const struct luaL_reg awesome_mousegrabber_lib[];
extern const struct luaL_reg awesome_button_methods[];
extern const struct luaL_reg awesome_button_meta[];
extern const struct luaL_reg awesome_image_methods[];
extern const struct luaL_reg awesome_image_meta[];
extern const struct luaL_reg awesome_mouse_methods[];
extern const struct luaL_reg awesome_mouse_meta[];
extern const struct luaL_reg awesome_screen_methods[];
extern const struct luaL_reg awesome_screen_meta[];
extern const struct luaL_reg awesome_client_methods[];
extern const struct luaL_reg awesome_client_meta[];
extern const struct luaL_reg awesome_tag_methods[];
extern const struct luaL_reg awesome_tag_meta[];
extern const struct luaL_reg awesome_widget_methods[];
extern const struct luaL_reg awesome_widget_meta[];
extern const struct luaL_reg awesome_wibox_methods[];
extern const struct luaL_reg awesome_wibox_meta[];
extern const struct luaL_reg awesome_key_methods[];
extern const struct luaL_reg awesome_key_meta[];

static struct sockaddr_un *addr;
static ev_io csio = { .fd = -1 };

#define XDG_CONFIG_HOME_DEFAULT "/.config"

/** Get or set global key bindings.
 * This binding will be available when you'll press keys on root window.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of key bindings objects, or nothing.
 * \return The array of key bindings objects of this client.
 */
static int
luaA_root_keys(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        luaA_key_array_set(L, 1, &globalconf.keys);

        int nscreen = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));

        for(int phys_screen = 0; phys_screen < nscreen; phys_screen++)
        {
            xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);
            xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, s->root, XCB_BUTTON_MASK_ANY);
            window_grabkeys(s->root, &globalconf.keys);
        }
    }

    return luaA_key_array_get(L, &globalconf.keys);
}

/** Get or set global mouse bindings.
 * This binding will be available when you'll click on root window.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects.
 */
static int
luaA_root_buttons(lua_State *L)
{
    if(lua_gettop(L) == 1)
        luaA_button_array_set(L, 1, &globalconf.buttons);

    return luaA_button_array_get(L, &globalconf.buttons);
}

/** Quit awesome.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_quit(lua_State *L __attribute__ ((unused)))
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

    awesome_atexit();

    a_exec(cmd);
    return 0;
}

/** Restart awesome.
 */
static int
luaA_restart(lua_State *L __attribute__ ((unused)))
{
    awesome_restart();
    return 0;
}

static void
luaA_openlib(lua_State *L, const char *name,
             const struct luaL_reg methods[],
             const struct luaL_reg meta[])
{
    luaL_newmetatable(L, name);                                        /* 1 */
    lua_pushvalue(L, -1);           /* dup metatable                      2 */
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable      1 */

    luaL_register(L, NULL, meta);                                      /* 1 */
    luaL_register(L, name, methods);                                   /* 2 */
    lua_pushvalue(L, -1);           /* dup self as metatable              3 */
    lua_setmetatable(L, -2);        /* set self as metatable              2 */
    lua_pop(L, 2);
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
 * \param The number of elements pushed on stack.
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
#define CHECK_TYPE(type) \
    do { \
        if(luaA_toudata(L, 1, #type)) \
        { \
            lua_pushliteral(L, #type); \
            return 1; \
        } \
    } while(0)
CHECK_TYPE(wibox);
CHECK_TYPE(client);
CHECK_TYPE(image);
CHECK_TYPE(key);
CHECK_TYPE(button);
CHECK_TYPE(tag);
CHECK_TYPE(widget);
#undef CHECK_TYPE
    lua_pushstring(L, luaL_typename(L, 1));
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

/** Newndex function of wtable objects.
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
    if(lua_istable(L, -1) || luaA_toudata(L, -1, "widget"))
        invalid = true;

    lua_pop(L, 1); /* remove value */

    /* if new value is a widget or a table */
    if(lua_istable(L, 3))
    {
        luaA_table2wtable(L);
        invalid = true;
    }
    else if(!invalid && luaA_toudata(L, 3, "widget"))
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
    lua_newtable(L); /* metatable */
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
luaA_isloop_check(lua_State *L, void_array_t *elems)
{
    if(lua_istable(L, -1))
    {
        const void *object = lua_topointer(L, -1);

        /* Check that the object table is not already in the list */
        for(int i = 0; i < elems->len; i++)
            if(elems->tab[i] == object)
                return false;

        /* push the table in the elements list */
        void_array_append(elems, object);

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
    void_array_t elems;

    void_array_init(&elems);

    /* push table on top */
    lua_pushvalue(L, idx);

    bool ret = luaA_isloop_check(L, &elems);

    /* remove pushed table */
    lua_pop(L, 1);

    void_array_wipe(&elems);

    return !ret;
}

/** Spawn a program.
 * This function is multi-head (Zaphod) aware and will set display to
 * the right screen according to mouse position.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack
 * \luastack
 * \lparam The command to launch.
 * \lparam The optional screen number to spawn the command on.
 */
static int
luaA_spawn(lua_State *L)
{
    char *host, newdisplay[128];
    const char *cmd;
    int screen = 0, screenp, displayp;

    if(lua_gettop(L) == 2)
    {
        screen = luaL_checknumber(L, 2) - 1;
        luaA_checkscreen(screen);
    }

    cmd = luaL_checkstring(L, 1);

    if(!globalconf.xinerama_is_active)
    {
        xcb_parse_display(NULL, &host, &displayp, &screenp);
        snprintf(newdisplay, sizeof(newdisplay), "%s:%d.%d", host, displayp, screen);
        setenv("DISPLAY", newdisplay, 1);
        p_delete(&host);
    }

    /* The double-fork construct avoids zombie processes and keeps the code
     * clean from stupid signal handlers. */
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(globalconf.connection)
                xcb_disconnect(globalconf.connection);
            setsid();
            a_exec(cmd);
            warn("execl '%s' failed: %s\n", cmd, strerror(errno));
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);

    return 0;
}

/** awesome global table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield font The default font.
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
      case A_TK_CONFFILE:
        lua_pushstring(L, globalconf.conffile);
        break;
      case A_TK_FG:
        luaA_pushcolor(L, &globalconf.colors.fg);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &globalconf.colors.bg);
        break;
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

/** Initialize the Lua VM
 */
void
luaA_init(void)
{
    lua_State *L;
    static const struct luaL_reg awesome_lib[] =
    {
        { "quit", luaA_quit },
        { "exec", luaA_exec },
        { "spawn", luaA_spawn },
        { "restart", luaA_restart },
        { "__index", luaA_awesome_index },
        { "__newindex", luaA_awesome_newindex },
        { NULL, NULL }
    };
    static const struct luaL_reg root_lib[] =
    {
        { "buttons", luaA_root_buttons },
        { "keys", luaA_root_keys },
        { NULL, NULL }
    };

    L = globalconf.L = luaL_newstate();

    luaL_openlibs(L);

    luaA_fixups(L);

    /* Export awesome lib */
    luaA_openlib(L, "awesome", awesome_lib, awesome_lib);

    /* Export root lib */
    luaA_openlib(L, "root", root_lib, root_lib);

    /* Export hooks lib */
    luaL_register(L, "hooks", awesome_hooks_lib);

    /* Export D-Bus lib */
    luaL_register(L, "dbus", awesome_dbus_lib);

    /* Export keygrabber lib */
    luaL_register(L, "keygrabber", awesome_keygrabber_lib);

    /* Export mousegrabber lib */
    luaL_register(L, "mousegrabber", awesome_mousegrabber_lib);

    /* Export screen */
    luaA_openlib(L, "screen", awesome_screen_methods, awesome_screen_meta);

    /* Export mouse */
    luaA_openlib(L, "mouse", awesome_mouse_methods, awesome_mouse_meta);

    /* Export button */
    luaA_openlib(L, "button", awesome_button_methods, awesome_button_meta);

    /* Export image */
    luaA_openlib(L, "image", awesome_image_methods, awesome_image_meta);

    /* Export tag */
    luaA_openlib(L, "tag", awesome_tag_methods, awesome_tag_meta);

    /* Export wibox */
    luaA_openlib(L, "wibox", awesome_wibox_methods, awesome_wibox_meta);

    /* Export widget */
    luaA_openlib(L, "widget", awesome_widget_methods, awesome_widget_meta);

    /* Export client */
    luaA_openlib(L, "client", awesome_client_methods, awesome_client_meta);

    /* Export keys */
    luaA_openlib(L, "key", awesome_key_methods, awesome_key_meta);

    lua_pushliteral(L, "AWESOME_VERSION");
    lua_pushstring(L, AWESOME_VERSION);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushliteral(L, "AWESOME_RELEASE");
    lua_pushstring(L, AWESOME_RELEASE);
    lua_settable(L, LUA_GLOBALSINDEX);

    /* init hooks */
    globalconf.hooks.manage = LUA_REFNIL;
    globalconf.hooks.unmanage = LUA_REFNIL;
    globalconf.hooks.focus = LUA_REFNIL;
    globalconf.hooks.unfocus = LUA_REFNIL;
    globalconf.hooks.mouse_enter = LUA_REFNIL;
    globalconf.hooks.mouse_leave = LUA_REFNIL;
    globalconf.hooks.arrange = LUA_REFNIL;
    globalconf.hooks.clients = LUA_REFNIL;
    globalconf.hooks.tags = LUA_REFNIL;
    globalconf.hooks.tagged = LUA_REFNIL;
    globalconf.hooks.property = LUA_REFNIL;
    globalconf.hooks.timer = LUA_REFNIL;
#ifdef WITH_DBUS
    globalconf.hooks.dbus = LUA_REFNIL;
#endif

    /* add Lua lib path (/usr/share/awesome/lib by default) */
    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?.lua\"");
    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?/init.lua\"");

    /* add XDG_CONFIG_DIR (/etc/xdg/awesome by default) as include path */
    luaA_dostring(L, "package.path = package.path .. \";" XDG_CONFIG_DIR "/awesome/?.lua\"");
    luaA_dostring(L, "package.path = package.path .. \";" XDG_CONFIG_DIR "/awesome/?/init.lua\"");

    /* add either XDG_CONFIG_HOME/awesome or HOME/.config/awesome to path */
    char *dir;
    if((dir = getenv("XDG_CONFIG_HOME")))
    {
        char *path;
        a_asprintf(&path, "package.path = package.path .. \";%s/awesome/?.lua;%s/awesome/?/init.lua\"", dir, dir);
        luaA_dostring(globalconf.L, path);
        p_delete(&path);
    }
    else
    {
        char *path, *homedir = getenv("HOME");
        a_asprintf(&path,
                   "package.path = package.path .. \";%s" XDG_CONFIG_HOME_DEFAULT "/awesome/?.lua;%s" XDG_CONFIG_HOME_DEFAULT "/awesome/?/init.lua\"",
                   homedir, homedir);
        luaA_dostring(globalconf.L, path);
        p_delete(&path);
    }

    /* add XDG_CONFIG_DIRS to include paths */
    char *xdg_config_dirs = getenv("XDG_CONFIG_DIRS");
    ssize_t len;

    if((len = a_strlen(xdg_config_dirs)))
    {
        char **buf, **xdg_files = a_strsplit(xdg_config_dirs, len, ':');

        for(buf = xdg_files; *buf; buf++)
        {
            char *confpath;
            a_asprintf(&confpath, "package.path = package.path .. \";%s/awesome/?.lua;%s/awesome/?/init.lua\"",
                       *buf, *buf);
            luaA_dostring(globalconf.L, confpath);
            p_delete(&confpath);
        }

        for(buf = xdg_files; *buf; buf++)
            p_delete(buf);
        p_delete(&xdg_files);
    }
}

#define AWESOME_CONFIG_FILE "/awesome/rc.lua"

static bool
luaA_loadrc(const char *confpath, bool run)
{
    if(!luaL_loadfile(globalconf.L, confpath))
    {
        if(run)
        {
            if(lua_pcall(globalconf.L, 0, LUA_MULTRET, 0))
                fprintf(stderr, "%s\n", lua_tostring(globalconf.L, -1));
            else
            {
                globalconf.conffile = a_strdup(confpath);
                return true;
            }
        }
        else
            lua_pop(globalconf.L, 1);
        return true;
    }
    else
        fprintf(stderr, "%s\n", lua_tostring(globalconf.L, -1));

    return false;
}

/** Load a configuration file.
 * \param confpatharg The configuration file to load.
 * \param run Run the configuration file.
 */
bool
luaA_parserc(const char *confpatharg, bool run)
{
    int screen;
    const char *confdir, *xdg_config_dirs;
    char *confpath = NULL, **xdg_files = NULL, **buf;
    ssize_t len;
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

    if((confdir = getenv("XDG_CONFIG_HOME")))
        a_asprintf(&confpath, "%s" AWESOME_CONFIG_FILE, confdir);
    else
        a_asprintf(&confpath, "%s" XDG_CONFIG_HOME_DEFAULT AWESOME_CONFIG_FILE, getenv("HOME"));

    /* try to run XDG_CONFIG_HOME/awesome/rc.lua */
    if(luaA_loadrc(confpath, run))
    {
        ret = true;
        goto bailout;
    }
    else if(!run)
        goto bailout;

    p_delete(&confpath);

    xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    if(!(len = a_strlen(xdg_config_dirs)))
    {
        xdg_config_dirs = XDG_CONFIG_DIR;
        len = sizeof(XDG_CONFIG_DIR) - 1;
    }

    xdg_files = a_strsplit(xdg_config_dirs, len, ':');

    for(buf = xdg_files; *buf && !ret; buf++)
    {
        a_asprintf(&confpath, "%s" AWESOME_CONFIG_FILE, *buf);
        if(luaA_loadrc(confpath, run))
        {
            ret = true;
            goto bailout;
        }
        else if(!run)
            goto bailout;
        p_delete(&confpath);
    }

bailout:

    p_delete(&confpath);

    if(xdg_files)
    {
        for(buf = xdg_files; *buf; buf++)
            p_delete(buf);
        p_delete(&xdg_files);
    }

    /* Assure there's at least one tag */
    for(screen = 0; screen < globalconf.nscreen; screen++)
        if(!globalconf.screens[screen].tags.len)
            tag_append_to_screen(tag_new("default", sizeof("default") - 1),
                                 &globalconf.screens[screen]);
    return ret;
}

/** Parse a command.
 * \param cmd The buffer to parse.
 * \return the number of elements pushed on the stack by the last statement in cmd.
 * If there's an error, the message is pushed onto the stack and this returns 1.
 */
static int
luaA_docmd(const char *cmd)
{
    char *p;
    int newtop, oldtop = lua_gettop(globalconf.L);

    while((p = strchr(cmd, '\n')))
    {
        newtop = lua_gettop(globalconf.L);
        lua_pop(globalconf.L, newtop - oldtop);
        oldtop = newtop;

        *p = '\0';
        if(luaL_dostring(globalconf.L, cmd))
        {
            warn("error executing Lua code: %s", lua_tostring(globalconf.L, -1));
            return 1;
        }
        cmd = p + 1;
    }
    return lua_gettop(globalconf.L) - oldtop;
}

/** Pushes a Lua array containing the top n elements of the stack.
 * \param n The number of elements to put in the array.
 */
static void
luaA_array(int n)
{
    int i;
    lua_createtable(globalconf.L, n, 0);
    lua_insert(globalconf.L, -n - 1);

    for (i = n; i > 0; i--)
        lua_rawseti(globalconf.L, -i - 1, i);
}

/** Maps the top n elements of the stack to the result of
 * applying a function to that element.
 * \param n The number of elements to map.
 * \luastack
 * \lparam The function to map the elements by. This should be
 * at position -(n + 1).
 */
static void
luaA_map(int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        lua_pushvalue(globalconf.L, -n - 1); /* copy of the function */
        lua_pushvalue(globalconf.L, -n - 1); /* value to map */
        lua_pcall(globalconf.L, 1, 1, 0);    /* call function */
        lua_remove(globalconf.L, -n - 1);    /* remove old value */
    }
    lua_remove(globalconf.L, -n - 1); /* remove function */
}

static void luaA_conn_cleanup(EV_P_ ev_io *w)
{
    ev_ref(EV_DEFAULT_UC);
    ev_io_stop(EV_DEFAULT_UC_ w);
    if(close(w->fd))
        warn("error closing UNIX domain socket: %s", strerror(errno));
    p_delete(&w);
}

static void
luaA_cb(EV_P_ ev_io *w, int revents)
{
    char buf[1024];
    int r, els;
    const char *s;
    size_t len;

    switch(r = recv(w->fd, buf, sizeof(buf)-1, MSG_TRUNC))
    {
      case -1:
        warn("error reading UNIX domain socket: %s", strerror(errno));
      case 0: /* 0 bytes are only transferred when the connection is closed */
        luaA_conn_cleanup(EV_DEFAULT_UC_ w);
        break;
      default:
        if(r >= ssizeof(buf))
            break;
        buf[r] = '\0';
        lua_getglobal(globalconf.L, "table");
        lua_getfield(globalconf.L, -1, "concat");
        lua_remove(globalconf.L, -2); /* remove table */

        lua_getglobal(globalconf.L, "tostring");
        els = luaA_docmd(buf);
        luaA_map(els); /* map results to strings */
        luaA_array(els); /* put strings in an array */

        lua_pushstring(globalconf.L, "\t");
        lua_pcall(globalconf.L, 2, 1, 0); /* concatenate results with tabs */

        s = lua_tolstring(globalconf.L, -1, &len);

        /* ignore ENOENT because the client may not read */
        if(send(w->fd, s, len, MSG_DONTWAIT) == -1)
           switch(errno)
           {
             case ENOENT:
             case EAGAIN:
               break;
             default:
                warn("can't send back to client via domain socket: %s", strerror(errno));
                break;
           }

        lua_pop(globalconf.L, 1); /* pop the string */
    }
    awesome_refresh(globalconf.connection);
}


static void
luaA_conn_cb(EV_P_ ev_io *w, int revents)
{
    ev_io *csio_conn = p_new(ev_io, 1);
    int csfd = accept(w->fd, NULL, NULL);

    if(csfd < 0)
        return;

    fd_set_close_on_exec(csfd);

    ev_io_init(csio_conn, &luaA_cb, csfd, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ csio_conn);
    ev_unref(EV_DEFAULT_UC);
}

void
luaA_cs_init(void)
{
    int csfd = socket_getclient();

    if(csfd < 0 || fcntl(csfd, F_SETFD, FD_CLOEXEC) == -1)
        return;

    if(!(addr = socket_open(csfd, SOCKET_MODE_BIND)))
    {
        warn("error binding UNIX domain socket: %s", strerror(errno));
        return;
    }

    listen(csfd, 10);
    ev_io_init(&csio, &luaA_conn_cb, csfd, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ &csio);
    ev_unref(EV_DEFAULT_UC);
}

void
luaA_cs_cleanup(void)
{
    if(csio.fd < 0)
        return;
    ev_ref(EV_DEFAULT_UC);
    ev_io_stop(EV_DEFAULT_UC_ &csio);
    if(close(csio.fd))
        warn("error closing UNIX domain socket: %s", strerror(errno));
    if(unlink(addr->sun_path))
        warn("error unlinking UNIX domain socket: %s", strerror(errno));
    p_delete(&addr);
    csio.fd = -1;
}

void
luaA_on_timer(EV_P_ ev_timer *w, int revents)
{
    if(globalconf.hooks.timer != LUA_REFNIL)
        luaA_dofunction(globalconf.L, globalconf.hooks.timer, 0, 0);
    awesome_refresh(globalconf.connection);
}

/** Push a color as a string onto the stack
 * \param L The Lua VM state.
 * \param c The color to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushcolor(lua_State *L, const xcolor_t *c)
{
    uint8_t r = (unsigned)c->red   * 0xff / 0xffff;
    uint8_t g = (unsigned)c->green * 0xff / 0xffff;
    uint8_t b = (unsigned)c->blue  * 0xff / 0xffff;
    uint8_t a = (unsigned)c->alpha * 0xff / 0xffff;
    char s[10];
    int len;
    /* do not print alpha if it's full */
    if(a == 0xff)
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
    else
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", r, g, b, a);
    lua_pushlstring(L, s, len);
    return 1;
}
