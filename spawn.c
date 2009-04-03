/*
 * spawn.c - Lua configuration management
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "structs.h"
#include "spawn.h"
#include "luaa.h"

extern awesome_t globalconf;

/** Spawn a program.
 * This function is multi-head (Zaphod) aware and will set display to
 * the right screen according to mouse position.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack
 * \luastack
 * \lparam The command to launch.
 * \lparam The optional screen number to spawn the command on.
 */
int
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

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
