/*
 * version.c - version message utility functions
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Hans Ulrich Niedermann <hun@n-dimensional.de>
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
 */

#include "config.h"
#include "common/version.h"
#include "globalconf.h"
#include "awesome-version-internal.h"

#include <stdlib.h>
#include <stdio.h>

#include <lualib.h>
#include <lauxlib.h>
#include <xcb/randr.h> /* for XCB_RANDR_GET_MONITORS */

/** \brief Print version message and quit program.
 * \param executable program name
 */
void
eprint_version(void)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_getglobal(L, "_VERSION");
    lua_getglobal(L, "jit");

    if (lua_istable(L, 2))
        lua_getfield(L, 2, "version");
    else
        lua_pop(L, 1);

    /* Either push version number or error message onto stack */
    (void) luaL_dostring(L, "return require('lgi.version')");

#ifdef WITH_DBUS
    const char *has_dbus = "yes";
#else
    const char *has_dbus = "no";
#endif
#ifdef WITH_XCB_ERRORS
    const char *has_xcb_errors = "yes";
#else
    const char *has_xcb_errors = "no";
#endif
#ifdef HAS_EXECINFO
    const char *has_execinfo = "yes";
#else
    const char *has_execinfo = "no";
#endif

    printf("awesome %s (%s)\n"
           " • Compiled against %s (running with %s)\n"
           " • API level: %d\n"
           " • D-Bus support: %s\n"
           " • xcb-errors support: %s\n"
           " • execinfo support: %s\n"
           " • xcb-randr version: %d.%d\n"
           " • LGI version: %s\n"
           " • Transparency enabled: %s\n"
           " • Custom search paths: %s\n",
        /* version      */ AWESOME_VERSION,
        /* release      */ AWESOME_RELEASE,
        /* Lua linked   */ LUA_RELEASE,
        /* Lua runtime  */ lua_tostring(L, -2),
        /* API Level    */ globalconf.api_level,
        /* DBus         */ has_dbus,
        /* XCB Error    */ has_xcb_errors,
        /* Execinfo     */ has_execinfo,
        /* XRandR major */ XCB_RANDR_MAJOR_VERSION,
        /* XRandR minor */ XCB_RANDR_MINOR_VERSION,
        /* LGI version  */ lua_tostring(L, -1),
        /* ARGB support */ globalconf.had_overriden_depth ? "no"  : "yes",
        /* Search path  */ globalconf.have_searchpaths    ? "yes" : "no"
    );
    lua_close(L);

    exit(EXIT_SUCCESS);
}

/** Get version string.
 * \return A string describing the current version.
 */
const char *
awesome_version_string(void)
{
    return AWESOME_VERSION;
}

/** Get release string.
 * \return A string describing the code name of the relase.
 */
const char *
awesome_release_string(void)
{
    return AWESOME_RELEASE;
}


int
awesome_default_api_level(void)
{
    return AWESOME_API_LEVEL;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
