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
#include "awesome-version-internal.h"

#include <stdlib.h>
#include <stdio.h>

#include <lualib.h>
#include <lauxlib.h>

/** \brief Print version message and quit program.
 * \param executable program name
 */
void
eprint_version(void)
{
    lua_State *L = luaL_newstate();
    luaopen_base(L);
    lua_getglobal(L, "_VERSION");

#ifdef WITH_DBUS
    const char *has_dbus = "✔";
#else
    const char *has_dbus = "✘";
#endif

    printf("awesome %s (%s)\n"
           " • Compiled against %s (running with %s)\n"
           " • D-Bus support: %s\n",
           AWESOME_VERSION, AWESOME_RELEASE,
           LUA_RELEASE, lua_tostring(L, -1),
           has_dbus);
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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
