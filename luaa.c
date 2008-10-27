/*
 * lua.c - Lua configuration management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <errno.h>
#include <stdio.h>
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
#include "titlebar.h"
#include "mouse.h"
#include "layouts/tile.h"
#include "common/socket.h"
#include "common/buffer.h"

extern awesome_t globalconf;

extern const struct luaL_reg awesome_hooks_lib[];
extern const struct luaL_reg awesome_keygrabber_lib[];
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
extern const struct luaL_reg awesome_titlebar_methods[];
extern const struct luaL_reg awesome_titlebar_meta[];
extern const struct luaL_reg awesome_tag_methods[];
extern const struct luaL_reg awesome_tag_meta[];
extern const struct luaL_reg awesome_widget_methods[];
extern const struct luaL_reg awesome_widget_meta[];
extern const struct luaL_reg awesome_statusbar_methods[];
extern const struct luaL_reg awesome_statusbar_meta[];
extern const struct luaL_reg awesome_wibox_methods[];
extern const struct luaL_reg awesome_wibox_meta[];
extern const struct luaL_reg awesome_keybinding_methods[];
extern const struct luaL_reg awesome_keybinding_meta[];

static struct sockaddr_un *addr;
static ev_io csio = { .fd = -1 };

/** Get or set global mouse bindings.
 * This binding will be available when you'll click on root window.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects of this client.
 */
static int
luaA_buttons(lua_State *L)
{
    button_array_t *buttons = &globalconf.buttons;

    if(lua_gettop(L) == 1)
        luaA_button_array_set(L, 1, buttons);

    return luaA_button_array_get(L, buttons);
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

/** Set or get default font. (DEPRECATED)
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam An optional string with a font name in Pango format.
 * \lreturn The font used, in Pango format.
 */
static int
luaA_font(lua_State *L)
{
    char *font;

    if(lua_gettop(L) == 1)
    {
        const char *newfont = luaL_checkstring(L, 1);
        draw_font_delete(&globalconf.font);
        globalconf.font = draw_font_new(globalconf.default_screen, newfont);
    }

    font = pango_font_description_to_string(globalconf.font->desc);
    lua_pushstring(L, font);
    g_free(font);

    return 1;
}

/** Set default font. (DEPRECATED)
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A string with a font name in Pango format.
 */
static int
luaA_font_set(lua_State *L)
{
    deprecate();
    return luaA_font(L);
}

/** Get configuration file path used by awesome.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn The awesome configuration file path.
 */
static int
luaA_conffile(lua_State *L)
{
    lua_pushstring(L, globalconf.conffile);
    return 1;
}

/** Set default colors.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with `fg' and `bg' elements, containing colors.
 * \lreturn A table with `fg' and `bg' elements, containing colors.
 */
static int
luaA_colors(lua_State *L)
{
    if(lua_gettop(L) == 1)
    {
        const char *buf;
        size_t len;
        int8_t colors_nbr = -1, i;
        xcolor_init_request_t reqs[2];

        luaA_checktable(L, 1);

        if((buf = luaA_getopt_lstring(L, 1, "fg", NULL, &len)))
           reqs[++colors_nbr] = xcolor_init_unchecked(&globalconf.colors.fg, buf, len);

        if((buf = luaA_getopt_lstring(L, 1, "bg", NULL, &len)))
           reqs[++colors_nbr] = xcolor_init_unchecked(&globalconf.colors.bg, buf, len);

        for(i = 0; i <= colors_nbr; i++)
           xcolor_init_reply(reqs[i]);
    }

    lua_newtable(L);
    luaA_pushcolor(L, &globalconf.colors.fg);
    lua_setfield(L, -2, "fg");
    luaA_pushcolor(L, &globalconf.colors.bg);
    lua_setfield(L, -2, "bg");

    return 1;
}

/** Set default colors. (DEPRECATED)
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 *
 * \luastack
 * \lparam A table with `fg' and `bg' elements, containing colors.
 */
static int
luaA_colors_set(lua_State *L)
{
    deprecate();
    return luaA_colors(L);
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
    lua_pushnumber(L, mbstowcs(NULL, NONULL(cmd), 0));
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

    return lua_next(L, idx);
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

/** Replace various standards Lua functions with our own.
 * \param L The Lua VM state.
 */
static void
luaA_fixups(lua_State *L)
{
    lua_getglobal(L, "string");
    lua_pushcfunction(L, luaA_mbstrlen);
    lua_setfield(L, -2, "len");
    lua_pop(L, 1);
    lua_pushliteral(L, "next");
    lua_pushcfunction(L, luaAe_next);
    lua_settable(L, LUA_GLOBALSINDEX);
    lua_pushliteral(L, "pairs");
    lua_pushcfunction(L, luaAe_next);
    lua_pushcclosure(L, luaAe_pairs, 1); /* pairs get next as upvalue */
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
    lua_pushcclosure(L, luaA_wtable_next, 1); /* __next has the content table as upvalue */
    lua_pushvalue(L, -3); /* copy content table */
    lua_pushcclosure(L, luaA_wtable_index, 1); /* __index has the content table as upvalue */
    lua_pushvalue(L, -4); /* copy content table */
    lua_pushcclosure(L, luaA_wtable_newindex, 1); /* __newindex has the content table as upvalue */
    /* set metatable field with just pushed closure */
    lua_setfield(L, -4, "__newindex");
    lua_setfield(L, -3, "__index");
    lua_setfield(L, -2, "__next");
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

/** Check if a table is a loop. When using table as direct acyclic digram,
 * this is useful.
 * \param L The Lua VM state.
 * \param idx The index of the table in the stack
 * \return True if the table loops.
 */
bool
luaA_isloop(lua_State *L, int idx)
{
    if(lua_istable(L, idx))
    {
        lua_pushvalue(L, idx); /* push table on top */
        if(luaA_hasitem(L, lua_topointer(L, -1)))
        {
            lua_pop(L, 1); /* remove pushed table */
            return true;
        }
        lua_pushnil(L);
        while(luaA_next(L, -2))
        {
            /* check for recursivity */
            if(luaA_isloop(L, -1))
            {
                lua_pop(L, 2); /* remove key and value */
                return true;
            }
            lua_pop(L, 1); /* remove value */
        }
        lua_pop(L, 1); /* remove pushed table */
    }
    return false;
}

/** Object table.
 * This table can use safely object as key.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
int
luaA_otable_index(lua_State *L)
{
    void **obj, **v;

    if((obj = lua_touserdata(L, 2)))
    {
        /* begins at nil */
        lua_pushnil(L);
        while(lua_next(L, 1))
        {
            /* check the key against the key if the key is a userdata,
             * otherwise check it again the value. */
            if((v = lua_touserdata(L, -2))
                && *v == *obj)
                /* return value */
                return 1;
            else if((v = lua_touserdata(L, -1))
                    && *v == *obj)
            {
                /* return key */
                lua_pop(L, 1);
                return 1;
            }
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
        return 0;
    }

    lua_rawget(L, 1);
    return 1;
}

/** Object table.
 * This table can use safely object as key.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_otable_newindex(lua_State *L)
{
    void **obj, **v;

    if((obj = lua_touserdata(L, 2)))
    {
        /* begins at nil */
        lua_pushnil(L);
        while(lua_next(L, 1))
        {
            if((v = lua_touserdata(L, -2))
               && *v == *obj)
            {
                /* remove value */
                lua_pop(L, 1);
                /* push new value on top */
                lua_pushvalue(L, 3);
                /* set in table key = value */
                lua_rawset(L, 1);
                return 0;
            }
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
    }

    lua_rawset(L, 1);

    return 0;
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

/** Initialize the Lua VM
 */
void
luaA_init(void)
{
    lua_State *L;

    static const struct luaL_reg otable_methods[] =
    {
        { "__call", luaA_otable_new },
        { NULL, NULL }
    };
    static const struct luaL_reg otable_meta[] =
    {
        { "__index", luaA_otable_index },
        { "__newindex", luaA_otable_newindex },
        { NULL, NULL }
    };
    static const struct luaL_reg awesome_lib[] =
    {
        { "quit", luaA_quit },
        { "exec", luaA_exec },
        { "spawn", luaA_spawn },
        { "restart", luaA_restart },
        { "buttons", luaA_buttons },
        { "font_set", luaA_font_set },
        { "colors_set", luaA_colors_set },
        { "font", luaA_font },
        { "colors", luaA_colors },
        { "conffile", luaA_conffile },
        { NULL, NULL }
    };

    L = globalconf.L = luaL_newstate();

    luaL_openlibs(L);

    luaA_fixups(L);

    /* Export awesome lib */
    luaL_register(L, "awesome", awesome_lib);

    /* Export hooks lib */
    luaL_register(L, "hooks", awesome_hooks_lib);

    /* Export keygrabber lib */
    luaL_register(L, "keygrabber", awesome_keygrabber_lib);

    /* Export otable lib */
    luaA_openlib(L, "otable", otable_methods, otable_meta);

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

    /* Export statusbar */
    luaA_openlib(L, "statusbar", awesome_statusbar_methods, awesome_statusbar_meta);

    /* Export wibox */
    luaA_openlib(L, "wibox", awesome_wibox_methods, awesome_wibox_meta);

    /* Export widget */
    luaA_openlib(L, "widget", awesome_widget_methods, awesome_widget_meta);

    /* Export client */
    luaA_openlib(L, "client", awesome_client_methods, awesome_client_meta);

    /* Export titlebar */
    luaA_openlib(L, "titlebar", awesome_titlebar_methods, awesome_titlebar_meta);

    /* Export keys */
    luaA_openlib(L, "keybinding", awesome_keybinding_methods, awesome_keybinding_meta);

    lua_pushliteral(L, "AWESOME_VERSION");
    lua_pushstring(L, AWESOME_VERSION);
    lua_settable(L, LUA_GLOBALSINDEX);

    lua_pushliteral(L, "AWESOME_RELEASE");
    lua_pushstring(L, AWESOME_RELEASE);
    lua_settable(L, LUA_GLOBALSINDEX);

    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?.lua\"");
    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?/init.lua\"");

    /* init hooks */
    globalconf.hooks.manage = LUA_REFNIL;
    globalconf.hooks.unmanage = LUA_REFNIL;
    globalconf.hooks.focus = LUA_REFNIL;
    globalconf.hooks.unfocus = LUA_REFNIL;
    globalconf.hooks.mouse_enter = LUA_REFNIL;
    globalconf.hooks.arrange = LUA_REFNIL;
    globalconf.hooks.clients = LUA_REFNIL;
    globalconf.hooks.tags = LUA_REFNIL;
    globalconf.hooks.tagged = LUA_REFNIL;
    globalconf.hooks.property = LUA_REFNIL;
    globalconf.hooks.timer = LUA_REFNIL;
}

#define XDG_CONFIG_HOME_DEFAULT "/.config"

#define AWESOME_CONFIG_FILE "/awesome/rc.lua"

static bool
luaA_loadrc(const char *confpath, bool run)
{
    if(confpath)
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
    }
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
    char *confpath = NULL, **xdg_files, **buf, path[1024];
    ssize_t len;
    bool ret;

    ret = luaA_loadrc(confpatharg, run);

    confdir = getenv("XDG_CONFIG_HOME");

    if((len = a_strlen(confdir)))
    {
        len += sizeof(AWESOME_CONFIG_FILE);
        confpath = p_new(char, len);
        a_strcpy(confpath, len, confdir);
        /* update package.path */
        snprintf(path, sizeof(path), "package.path = package.path .. \";%s/awesome/?.lua\"", confdir);
        luaA_dostring(globalconf.L, path);
    }
    else
    {
        confdir = getenv("HOME");
        len = a_strlen(confdir) + sizeof(AWESOME_CONFIG_FILE)-1 + sizeof(XDG_CONFIG_HOME_DEFAULT);
        confpath = p_new(char, len);
        a_strcpy(confpath, len, confdir);
        a_strcat(confpath, len, XDG_CONFIG_HOME_DEFAULT);
        /* update package.path */
        snprintf(path, sizeof(path), "package.path = package.path .. \";%s" XDG_CONFIG_HOME_DEFAULT "/awesome/?.lua\"", confdir);
        luaA_dostring(globalconf.L, path);
    }
    a_strcat(confpath, len, AWESOME_CONFIG_FILE);

    if(!ret)
        ret = luaA_loadrc(confpath, run);

    xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    if(!(len = a_strlen(xdg_config_dirs)))
    {
        xdg_config_dirs = XDG_CONFIG_DIR;
        len = sizeof(XDG_CONFIG_DIR) - 1;
    }

    xdg_files = a_strsplit(xdg_config_dirs, len, ':');

    for(buf = xdg_files; *buf; buf++)
    {
        p_delete(&confpath);
        len = a_strlen(*buf) + sizeof("AWESOME_CONFIG_FILE");
        confpath = p_new(char, len);
        a_strcpy(confpath, len, *buf);
        a_strcat(confpath, len, AWESOME_CONFIG_FILE);
        snprintf(path, sizeof(path), "package.path = package.path .. \";%s/awesome/?.lua\"", *buf);
        luaA_dostring(globalconf.L, path);
        if(!ret)
            ret = luaA_loadrc(confpath, run);
    }

    for(buf = xdg_files; *buf; buf++)
        p_delete(buf);
    p_delete(&xdg_files);

    /* Assure there's at least one tag */
    for(screen = 0; screen < globalconf.nscreen; screen++)
        if(!globalconf.screens[screen].tags.len)
            tag_append_to_screen(tag_new("default", sizeof("default")-1, layout_tile, 0.5, 1, 0),
                                 &globalconf.screens[screen]);

    p_delete(&confpath);

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
        if (luaL_dostring(globalconf.L, cmd))
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
    if (close(w->fd))
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
        if (send(w->fd, s, len, MSG_DONTWAIT) == -1)
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

    ev_io_init(csio_conn, &luaA_cb, csfd, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ csio_conn);
    ev_unref(EV_DEFAULT_UC);
}

void
luaA_cs_init(void)
{
    int csfd = socket_getclient();

    if (csfd < 0 || fcntl(csfd, F_SETFD, FD_CLOEXEC) == -1)
        return;

    addr = socket_getaddr(getenv("DISPLAY"));

    if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
    {
        if(errno == EADDRINUSE)
        {
            if(unlink(addr->sun_path))
                warn("error unlinking existing file: %s", strerror(errno));
            if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                return warn("error binding UNIX domain socket: %s", strerror(errno));
        }
        else
            return warn("error binding UNIX domain socket: %s", strerror(errno));
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
    if (close(csio.fd))
        warn("error closing UNIX domain socket: %s", strerror(errno));
    if(unlink(addr->sun_path))
        warn("error unlinking UNIX domain socket: %s", strerror(errno));
    p_delete(&addr);
    csio.fd = -1;
}

void
luaA_on_timer(EV_P_ ev_timer *w, int revents)
{
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
    /* do not print alpha if it's full */
    if(a == 0xff)
        snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
    else
        snprintf(s, sizeof(s), "#%02x%02x%02x%02x", r, g, b, a);
    lua_pushlstring(L, s, sizeof(s));
    return 1;
}
