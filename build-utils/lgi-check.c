/*
 * lgi-check.c - Check that LGI is available
 *
 * Copyright © 2017 Uli Schlachter <psychon@znc.in>
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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>

const char commands[] =
"print(string.format('Building for %s.', jit and jit.version or _VERSION))\n"
"local lgi_version = require('lgi.version')\n"
"print(string.format('Found lgi %s.', lgi_version))\n"
"_, _, major_minor, patch = string.find(lgi_version, '^(%d%.%d)%.(%d)')\n"
"if tonumber(major_minor) < 0.8 or (tonumber(major_minor) == 0.8 and tonumber(patch) < 0) then\n"
"    error(string.format('lgi is too old, need at least version %s, got %s.',\n"
"        '0.8.0', require('lgi.version')))\n"
"end\n"
"lgi = require('lgi')\n"
"assert(lgi.cairo, lgi.Pango, lgi.PangoCairo, lgi.GLib, lgi.Gio)\n"
;

int main()
{
    int result = 0;
    const char *env = "AWESOME_IGNORE_LGI";
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_dostring(L, commands))
    {
        fprintf(stderr, "Error: %s\n",
                lua_tostring(L, -1));
        fprintf(stderr, "\n\n       WARNING\n       =======\n\n"
                " The lgi check failed.\n"
                " Awesome needs lgi to run.\n"
                " Add %s=1 to your environment to continue.\n\n\n",
                env);
        if (getenv(env) == NULL)
            result = 1;
    }
    lua_close(L);
    return result;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
