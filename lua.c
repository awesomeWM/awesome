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

#include <ev.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "ewmh.h"
#include "config.h"
#include "lua.h"
#include "tag.h"
#include "client.h"
#include "statusbar.h"
#include "titlebar.h"
#include "screen.h"
#include "layouts/tile.h"
#include "common/socket.h"
#include "common/version.h"

extern awesome_t globalconf;

extern const struct luaL_reg awesome_keygrabber_lib[];
extern const struct luaL_reg awesome_mouse_methods[];
extern const struct luaL_reg awesome_mouse_meta[];
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
extern const struct luaL_reg awesome_keybinding_methods[];
extern const struct luaL_reg awesome_keybinding_meta[];

static struct sockaddr_un *addr;
static ev_io csio = { .fd = -1 };

/** Add a global mouse binding. This binding will be available when you'll
 * click on root window.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A mouse button binding.
 */
static int
luaA_mouse_add(lua_State *L)
{
    button_t **button = luaA_checkudata(L, 1, "mouse");

    button_list_push(&globalconf.buttons.root, *button);
    button_ref(button);

    return 0;
}

/** Quit awesome.
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
 *
 * \luastack
 * \lparam The command line to execute.
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
    ewmh_restart();
    return 0;
}

/** Set the screen padding. This can be used to define margin around the
 * screen. awesome will not use this area.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A screen number.
 * \lparam A table with a list of margin for `right', `left', `top' and
 * `bottom'.
 */
static int
luaA_screen_padding_set(lua_State *L)
{
    int screen = luaL_checknumber(L, 1) - 1;

    luaA_checkscreen(screen);

    luaA_checktable(L, 2);

    globalconf.screens[screen].padding.right = luaA_getopt_number(L, 2, "right", 0);
    globalconf.screens[screen].padding.left = luaA_getopt_number(L, 2, "left", 0);
    globalconf.screens[screen].padding.top = luaA_getopt_number(L, 2, "top", 0);
    globalconf.screens[screen].padding.bottom = luaA_getopt_number(L, 2, "bottom", 0);

    ewmh_update_workarea(screen_virttophys(screen));

    return 0;
}

/** Get the screen count.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lreturn The screen count, at least 1.
 */
static int
luaA_screen_count(lua_State *L)
{
    lua_pushnumber(L, globalconf.screens_info->nscreen);
    return 1;
}

/** Give the focus to a screen.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A screen number
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
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call each time a client gets focus.
 */
static int
luaA_hooks_focus(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.focus);
}

/** Set the function called each time a client loses focus. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call each time a client loses focus.
 */
static int
luaA_hooks_unfocus(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.unfocus);
}

/** Set the function called each time a new client appears. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call on each new client.
 */
static int
luaA_hooks_manage(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.manage);
}

/** Set the function called each time a client goes away. This function is
 * called with the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call when a client goes away.
 */
static int
luaA_hooks_unmanage(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.unmanage);
}

/** Set the function called each time the mouse enter a new window. This
 * function is called with the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call each time a client gets mouse over it.
 */
static int
luaA_hooks_mouseover(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.mouseover);
}

/** Set the function called on each screen arrange. This function is called
 * with the screen number as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call on each screen arrange.
 */
static int
luaA_hooks_arrange(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.arrange);
}

/** Set the function called on each title update. This function is called with
 * the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call on each title update of each client.
 */
static int
luaA_hooks_titleupdate(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.titleupdate);
}

/** Set the function called when a client get urgency flag. This function is called with
 * the client object as argument.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A function to call when a client get the urgent flag.
 */
static int
luaA_hooks_urgent(lua_State *L)
{
    return luaA_registerfct(L, &globalconf.hooks.urgent);
}

/** Set the function to be called every N seconds.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam The number of seconds to run function every. Set 0 to disable.
 * \lparam A function to call every N seconds (optional).
 */
static int
luaA_hooks_timer(lua_State *L)
{
    globalconf.timer.repeat = luaL_checknumber(L, 1);

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
        luaA_registerfct(L, &globalconf.hooks.timer);

    ev_timer_again(globalconf.loop, &globalconf.timer);
    return 0;
}

/** Set default font.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A string with a font name in Pango format.
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
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with `fg' and `bg' elements, containing colors.
 */
static int
luaA_colors_set(lua_State *L)
{
    const char *buf;
    size_t len;

    luaA_checktable(L, 1);

    if((buf = luaA_getopt_lstring(L, 1, "fg", NULL, &len)))
       xcolor_init(&globalconf.colors.fg, globalconf.connection,
                   globalconf.default_screen, buf, len);

    if((buf = luaA_getopt_lstring(L, 1, "bg", NULL, &len)))
       xcolor_init(&globalconf.colors.bg, globalconf.connection,
                   globalconf.default_screen, buf, len);
    return 0;
}

/** Push a pointer onto the stack according to its type.
 * \param p The pointer.
 * \param type Its type.
 */
void
luaA_pushpointer(lua_State *L, void *p, awesome_type_t type)
{
    switch(type)
    {
      case AWESOME_TYPE_STATUSBAR:
        luaA_statusbar_userdata_new(L, p);
        break;
      case AWESOME_TYPE_TITLEBAR:
        luaA_titlebar_userdata_new(L, p);
        break;
    }
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

static int
luaA_mbstrlen(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);
    lua_pushnumber(L, mbstowcs(NULL, NONULL(cmd), 0));
    return 1;
}

static void
luaA_fixups(lua_State *L)
{
    lua_getglobal(L, "string");
    lua_pushcfunction(L, luaA_mbstrlen);
    lua_setfield(L, -2, "len");
    lua_pop(L, 1);
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
        { "restart", luaA_restart },
        { "mouse_add", luaA_mouse_add },
        { "font_set", luaA_font_set },
        { "colors_set", luaA_colors_set },
        { NULL, NULL }
    };
    static const struct luaL_reg awesome_screen_lib[] =
    {
        { "padding_set", luaA_screen_padding_set },
        { "coords_get", luaA_screen_coords_get },
        { "count", luaA_screen_count },
        { "focus", luaA_screen_focus },
        { NULL, NULL }
    };
    static const struct luaL_reg awesome_hooks_lib[] =
    {
        { "focus", luaA_hooks_focus },
        { "unfocus", luaA_hooks_unfocus },
        { "manage", luaA_hooks_manage },
        { "unmanage", luaA_hooks_unmanage },
        { "mouseover", luaA_hooks_mouseover },
        { "arrange", luaA_hooks_arrange },
        { "titleupdate", luaA_hooks_titleupdate },
        { "urgent", luaA_hooks_urgent },
        { "timer", luaA_hooks_timer },
        { NULL, NULL }
    };

    L = globalconf.L = lua_open();

    luaL_openlibs(L);

    luaA_fixups(L);

    /* Export awesome lib */
    luaL_register(L, "awesome", awesome_lib);

    /* Export screen lib */
    luaL_register(L, "screen", awesome_screen_lib);

    /* Export hooks lib */
    luaL_register(L, "hooks", awesome_hooks_lib);

    /* Export keygrabber lib */
    luaL_register(L, "keygrabber", awesome_keygrabber_lib);

    /* Export mouse */
    luaA_openlib(L, "mouse", awesome_mouse_methods, awesome_mouse_meta);

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

    /* Export keys */
    luaA_openlib(L, "keybinding", awesome_keybinding_methods, awesome_keybinding_meta);

    lua_pushliteral(L, "AWESOME_VERSION");
    lua_pushstring(L, version_string());
    lua_settable(L, LUA_GLOBALSINDEX);

    luaA_dostring(L, "package.path = package.path .. \";" AWESOME_LUA_LIB_PATH  "/?.lua\"");

    /* init hooks */
    globalconf.hooks.manage = LUA_REFNIL;
    globalconf.hooks.unmanage = LUA_REFNIL;
    globalconf.hooks.focus = LUA_REFNIL;
    globalconf.hooks.unfocus = LUA_REFNIL;
    globalconf.hooks.mouseover = LUA_REFNIL;
    globalconf.hooks.arrange = LUA_REFNIL;
    globalconf.hooks.titleupdate = LUA_REFNIL;
    globalconf.hooks.urgent = LUA_REFNIL;
    globalconf.hooks.timer = LUA_REFNIL;
}

#define XDG_CONFIG_HOME_DEFAULT "/.config"

#define AWESOME_CONFIG_FILE "/awesome/rc.lua"

/** Load a configuration file.
 * \param rcfile The configuration file to load.
 */
void
luaA_parserc(const char *confpatharg)
{
    int screen;
    const char *confdir, *xdg_config_dirs;
    char *confpath = NULL, **xdg_files, **buf;
    ssize_t len;
    
    if(confpatharg)
    {
        if(luaL_dofile(globalconf.L, confpatharg))
            fprintf(stderr, "%s\n", lua_tostring(globalconf.L, -1));
        else
            goto bailout;
    }

    confdir = getenv("XDG_CONFIG_HOME");

    if(a_strlen(confdir))
    {
        len = a_strlen(confdir) + sizeof(AWESOME_CONFIG_FILE) + 1;
        confpath = p_new(char, len);
        a_strcpy(confpath, len, confdir);
    }
    else
    {
        confdir = getenv("HOME");
        len = a_strlen(confdir) + sizeof(AWESOME_CONFIG_FILE) + sizeof(XDG_CONFIG_HOME_DEFAULT) + 1;
        confpath = p_new(char, len);
        a_strcpy(confpath, len, confdir);
        a_strcat(confpath, len, XDG_CONFIG_HOME_DEFAULT);
    }
    a_strcat(confpath, len, AWESOME_CONFIG_FILE);

    if(luaL_dofile(globalconf.L, confpath))
        fprintf(stderr, "%s\n", lua_tostring(globalconf.L, -1));
    else
        goto bailout;

    xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    if(!(len = a_strlen(xdg_config_dirs)))
    {
        xdg_config_dirs = SYSCONFDIR;
        len = sizeof(SYSCONFDIR);
    }

    xdg_files = a_strsplit(xdg_config_dirs, len, ':');

    for(buf = xdg_files; *buf; buf++)
    {
        p_delete(&confpath);
        len = a_strlen(*buf) + sizeof("AWESOME_CONFIG_FILE") + 1;
        confpath = p_new(char, len);
        a_strcpy(confpath, len, *buf);
        a_strcat(confpath, len, AWESOME_CONFIG_FILE);
        if(luaL_dofile(globalconf.L, confpath))
            fprintf(stderr, "%s\n", lua_tostring(globalconf.L, -1));
        else
            break;
    }

    for(buf = xdg_files; *buf; buf++)
        p_delete(buf);
    p_delete(&xdg_files);
    
  bailout:
    /* Assure there's at least one tag */
    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        if(!globalconf.screens[screen].tags.len)
            tag_append_to_screen(tag_new("default", sizeof("default"), layout_tile, 0.5, 1, 0), screen);

    p_delete(&confpath);
}

/** Parse a command.
 * \param cmd The buffer to parse.
 * \return true on succes, false on failure.
 */
static void
luaA_docmd(const char *cmd)
{
    char *p;

    while((p = strchr(cmd, '\n')))
    {
        *p = '\0';
        luaA_dostring(globalconf.L, cmd);
        cmd = p + 1;
    }
}

static void
luaA_cb(EV_P_ ev_io *w, int revents)
{
    char buf[1024];
    int r;

    switch(r = recv(w->fd, buf, sizeof(buf)-1, MSG_TRUNC))
    {
      case -1:
        warn("error reading UNIX domain socket: %s", strerror(errno));
        luaA_cs_cleanup();
        break;
      case 0:
        break;
      default:
        if(r >= ssizeof(buf))
            break;
        buf[r] = '\0';
        luaA_docmd(buf);
    }
}

void
luaA_cs_init(void)
{
    int csfd = socket_getclient();

    if (csfd < 0)
        return;
    addr = socket_getaddr(getenv("DISPLAY"));

    if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
    {
        if(errno == EADDRINUSE)
        {
            if(unlink(addr->sun_path))
                warn("error unlinking existing file: %s", strerror(errno));
            if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                warn("error binding UNIX domain socket: %s", strerror(errno));
        }
        else
            warn("error binding UNIX domain socket: %s", strerror(errno));
    }
    ev_io_init(&csio, &luaA_cb, csfd, EV_READ);
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
}

void
luaA_pushcolor(lua_State *L, const xcolor_t *c)
{
    uint8_t r = (unsigned)c->red   * 0xff / 0xffff;
    uint8_t g = (unsigned)c->green * 0xff / 0xffff;
    uint8_t b = (unsigned)c->blue  * 0xff / 0xffff;
    uint8_t a = (unsigned)c->alpha * 0xff / 0xffff;
    lua_pushfstring(L, "#%02x%02x%02x%02x", r, g, b, a);
}
