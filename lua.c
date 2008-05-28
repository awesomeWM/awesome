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

#include <stdio.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

/* XStringToKeysym() */
#include <X11/Xlib.h>

#include "config.h"
#include "structs.h"
#include "lua.h"
#include "window.h"
#include "tag.h"
#include "client.h"
#include "layouts/tile.h"

extern awesome_t globalconf;

extern bool running;
extern const name_func_link_t FloatingPlacementList[];

extern const struct luaL_reg awesome_mouse_lib[];
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

static void
__luaA_keystore(keybinding_t *key, const char *str)
{
    xcb_keycode_t kc;
    int ikc;

    if(!a_strlen(str))
        return;
    else if(a_strncmp(str, "#", 1))
        key->keysym = XStringToKeysym(str);
    else
    {
        ikc = atoi(str + 1);
        memcpy(&kc, &ikc, sizeof(KeyCode));
        key->keycode = kc;
    }
}

/** Define a global mouse binding. This binding will be available wehn you
 * click on root window.
 * \param A table with modifiers keys.
 * \param A mouse button number.
 * \param A function to execute.
 */
static int
luaA_mouse(lua_State *L)
{
    size_t i, len;
    int b;
    button_t *button;

    /* arg 1 is modkey table */
    luaA_checktable(L, 1);
    /* arg 2 is mouse button */
    b = luaL_checknumber(L, 2);
    /* arg 3 is cmd to run */
    luaA_checkfunction(L, 3);

    button = p_new(button_t, 1);
    button->button = xutil_button_fromint(b);
    button->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 1);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 1, i);
        button->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    button_list_push(&globalconf.buttons.root, button);

    window_root_grabbutton(button);

    return 0;
}

/** Define a global key binding. This key binding will always be available.
 * \param A table with modifier keys.
 * \param A key name.
 * \param A function to execute.
 */
static int
luaA_key(lua_State *L)
{
    size_t i, len;
    keybinding_t *k;
    const char *key;

    /* arg 1 is key mod table */
    luaA_checktable(L, 1);
    /* arg 2 is key */
    key = luaL_checkstring(L, 2);
    /* arg 3 is cmd to run */
    luaA_checkfunction(L, 3);

    /* get the last arg as function */
    k = p_new(keybinding_t, 1);
    __luaA_keystore(k, key);
    k->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 1);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 1, i);
        k->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    keybinding_list_push(&globalconf.keys, k);

    window_root_grabkey(k);

    return 0;
}

/** Set the floating placement algorithm. This will be used to compute the
 * initial floating position of floating windows.
 * \param An algorith name, either `none', `smart' or `mouse'.
 */
static int
luaA_floating_placement_set(lua_State *L)
{
    const char *pl = luaL_checkstring(L, 1);
    globalconf.floating_placement = name_func_lookup(pl, FloatingPlacementList);
    return 0;
}

/** Quit awesome.
 */
static int
luaA_quit(lua_State *L __attribute__ ((unused)))
{
    running = false;
    return 0;
}

/** Execute another application, probably a window manager, to replace
 * awesome.
 * \param The command line to execute.
 */
static int
luaA_exec(lua_State *L)
{
    client_t *c;
    const char *cmd = luaL_checkstring(L, 1);

    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);
    xcb_disconnect(globalconf.connection);

    a_exec(cmd);
    return 0;
}

/** Restart awesome.
 */
static int
luaA_restart(lua_State *L __attribute__ ((unused)))
{
    a_exec(globalconf.argv);
    return 0;
}

/** Set the screen padding. This can be used to define margin around the
 * screen. awesome will not use this area.
 * \param A table with a list of margin for `right', `left', `top' and
 * `bottom'.
 */
static int
luaA_padding_set(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;

    if(screen >= 0 && screen < globalconf.screens_info->nscreen)
    {
        luaA_checktable(L, 2);

        globalconf.screens[screen].padding.right = luaA_getopt_number(L, 2, "right", 0);
        globalconf.screens[screen].padding.left = luaA_getopt_number(L, 2, "left", 0);
        globalconf.screens[screen].padding.top = luaA_getopt_number(L, 2, "top", 0);
        globalconf.screens[screen].padding.bottom = luaA_getopt_number(L, 2, "bottom", 0);
    }

    return 0;
}

/** Define if awesome should respect applications size hints when resizing
 * windows in tiled mode. If you set this to true, you will experience gaps
 * between windows, but they will have the best size they can have.
 * \param A boolean value, true to enable, false to disable.
 */
static int
luaA_resizehints_set(lua_State *L)
{
    globalconf.resize_hints = luaA_checkboolean(L, 1);
    return 0;
}

/** Get the screen count.
 * \return The screen count, at least 1.
 */
static int
luaA_screen_count(lua_State *L)
{
    lua_pushnumber(L, globalconf.screens_info->nscreen);
    return 1;
}

/** Give the focus to a screen.
 * \param A screen number
 */
static int
luaA_screen_focus(lua_State *L)
{
    /* Our table begin at 0, Lua begins at 1 */
    int screen = luaL_checknumber(L, 1) - 1;
    luaA_checkscreen(screen);
    client_focus(NULL, screen);
    return 0;
}

static int
luaA_screen_coords_get(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;
    luaA_checkscreen(screen);
    lua_newtable(L);
    lua_pushnumber(L, globalconf.screens_info->geometry[screen].x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, globalconf.screens_info->geometry[screen].y);
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, globalconf.screens_info->geometry[screen].width);
    lua_setfield(L, -2, "width");
    lua_pushnumber(L, globalconf.screens_info->geometry[screen].height);
    lua_setfield(L, -2, "height");
    return 1;
}

/** Set the function called each time a client gets focus. This function is
 * called with the client object as argument.
 * \param A function to call each time a client gets focus.
 */
static int
luaA_hooks_focus(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.focus)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.focus);
    globalconf.hooks.focus = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called each time a client loses focus. This function is
 * called with the client object as argument.
 * \param A function to call each time a client loses focus.
 */
static int
luaA_hooks_unfocus(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.unfocus)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.unfocus);
    globalconf.hooks.unfocus = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called each time a new client appears. This function is
 * called with the client object as argument
 * \param A function to call on each new client.
 */
static int
luaA_hooks_newclient(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.newclient)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.newclient);
    globalconf.hooks.newclient = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called each time the mouse enter a new window. This
 * function is called with the client object as argument.
 * \param A function to call each time a client gets mouse over it.
 */
static int
luaA_hooks_mouseover(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.mouseover)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.mouseover);
    globalconf.hooks.mouseover = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called on each screen arrange. This function is called
 * with the screen number as argument.
 * \param A function to call on each screen arrange.
 */
static int
luaA_hooks_arrange(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.arrange)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.arrange);
    globalconf.hooks.arrange = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called on each title update. This function is called with
 * the client object as argument.
 * \param A function to call on each title update of each client.
 */
static int
luaA_hooks_titleupdate(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.titleupdate)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.titleupdate);
    globalconf.hooks.titleupdate = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set the function called when a client get urgency flag. This function is called with
 * the client object as argument.
 * \param A function to call when a client get the urgent flag.
 */
static int
luaA_hooks_urgent(lua_State *L)
{
    luaA_checkfunction(L, 1);
    if(globalconf.hooks.urgent)
        luaL_unref(L, LUA_REGISTRYINDEX, globalconf.hooks.urgent);
    globalconf.hooks.urgent = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

/** Set default font.
 * \param A string with a font name in Pango format.
 */
static int
luaA_font_set(lua_State *L)
{
    const char *font = luaL_checkstring(L, 1);
    draw_font_delete(&globalconf.font);
    globalconf.font = draw_font_new(globalconf.connection,
                                    globalconf.default_screen, font);
    return 0;
}

/** Set default colors.
 * \param A table with `fg' and `bg' elements, containing colors.
 */
static int
luaA_colors_set(lua_State *L)
{
    const char *fg, *bg;
    luaA_checktable(L, 1);
    if((fg = luaA_getopt_string(L, 1, "fg", NULL)))
        draw_color_new(globalconf.connection, globalconf.default_screen,
                       fg, &globalconf.colors.fg);
    if((bg = luaA_getopt_string(L, 1, "bg",NULL)))
        draw_color_new(globalconf.connection, globalconf.default_screen,
                       bg, &globalconf.colors.bg);
    return 0;
}

static void
luaA_openlib(lua_State *L, const char *name,
             const struct luaL_reg methods[],
             const struct luaL_reg meta[])
{
    luaL_newmetatable(L, name);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2); /* dup metatable*/
    lua_settable(L, -3);  /* metatable.__index = metatable */

    luaL_register(L, NULL, meta);
    luaL_register(L, name, methods);
}

bool
luaA_parserc(const char *rcfile)
{
    lua_State *L;
    int screen;

    static const struct luaL_reg awesome_lib[] =
    {
        { "quit", luaA_quit },
        { "exec", luaA_exec },
        { "restart", luaA_restart },
        { "floating_placement_set", luaA_floating_placement_set },
        { "padding_set", luaA_padding_set },
        { "key", luaA_key },
        { "mouse", luaA_mouse },
        { "resizehints_set", luaA_resizehints_set },
        { "font_set", luaA_font_set },
        { "colors_set", luaA_colors_set },
        { NULL, NULL }
    };
    static const struct luaL_reg awesome_screen_lib[] =
    {
        { "coords_get", luaA_screen_coords_get },
        { "count", luaA_screen_count },
        { "focus", luaA_screen_focus },
        { NULL, NULL }
    };
    static const struct luaL_reg awesome_hooks_lib[] =
    {
        { "focus", luaA_hooks_focus },
        { "unfocus", luaA_hooks_unfocus },
        { "newclient", luaA_hooks_newclient },
        { "mouseover", luaA_hooks_mouseover },
        { "arrange", luaA_hooks_arrange },
        { "titleupdate", luaA_hooks_titleupdate },
        { "urgent", luaA_hooks_urgent },
        { NULL, NULL }
    };

    L = globalconf.L = lua_open();

    luaL_openlibs(L);

    /* Export awesome lib */
    luaL_register(L, "awesome", awesome_lib);

    /* Export screen lib */
    luaL_register(L, "screen", awesome_screen_lib);

    /* Export hooks lib */
    luaL_register(L, "hooks", awesome_hooks_lib);

    /* Export hooks lib */
    luaL_register(L, "mouse", awesome_mouse_lib);

    /* Export tag */
    luaA_openlib(L, "tag", awesome_tag_methods, awesome_tag_meta);

    /* Export statusbar */
    luaA_openlib(L, "statusbar", awesome_statusbar_methods, awesome_statusbar_meta);

    /* Export widget */
    luaA_openlib(L, "widget", awesome_widget_methods, awesome_widget_meta);

    /* Export client */
    luaA_openlib(L, "client", awesome_client_methods, awesome_client_meta);

    /* Export titlebar */
    luaA_openlib(L, "titlebar", awesome_titlebar_methods, awesome_titlebar_meta);

    lua_pushliteral(L, "AWESOME_VERSION");
    lua_pushliteral(L, VERSION);
    lua_settable(L, LUA_GLOBALSINDEX);

    /* \todo move this */
    globalconf.font = draw_font_new(globalconf.connection, globalconf.default_screen, "sans 8");
    draw_color_new(globalconf.connection, globalconf.default_screen, "black", &globalconf.colors.fg);
    draw_color_new(globalconf.connection, globalconf.default_screen, "white", &globalconf.colors.bg);

    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?.lua\"");

    if(luaL_dofile(L, rcfile))
    {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        return false;
    }

    /* Assure there's at least one tag */
    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        if(!globalconf.screens[screen].tags)
            tag_append_to_screen(tag_new("default", layout_tile, 0.5, 1, 0), screen);

    return true;
}

/** Parse a command.
 * \param cmd The buffer to parse.
 * \return true on succes, false on failure.
 */
void
luaA_docmd(char *cmd)
{
    char *p, *curcmd = cmd;

    if(a_strlen(cmd))
        while((p = strchr(curcmd, '\n')))
        {
            *p = '\0';
            luaA_dostring(globalconf.L, curcmd);
            curcmd = p + 1;
        }
}
