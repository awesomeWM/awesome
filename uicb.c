/*
 * uicb.c - user interface callbacks management
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

/**
 * @defgroup ui_callback User Interface Callbacks
 */

/* strndup() */
#define _GNU_SOURCE

#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "awesome.h"
#include "tag.h"
#include "mouse.h"
#include "statusbar.h"
#include "widget.h"
#include "focus.h"
#include "client.h"
#include "screen.h"
#include "titlebar.h"
#include "layouts/tile.h"

extern AwesomeConf globalconf;

#include "uicbgen.h"

/** Execute another process, replacing the current instance of Awesome
 * \param screen Screen ID
 * \param arg Command
 * \ingroup ui_callback
 */
void
uicb_exec(int screen __attribute__ ((unused)), char *cmd)
{
    Client *c;
    char *args, *path;

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    XSync(globalconf.display, False);

    /* Ignore the leading spaces if any */
    while(cmd[0] && cmd[0] == ' ')
        cmd++;

    /* Get the beginning of the arguments */
    args = strchr(cmd, ' ');

    if(args)
        path = strndup(cmd, args - cmd);
    else
        path = strndup(cmd, a_strlen(cmd));

    execlp(path, cmd, NULL);

    p_delete(&path);
}

/** Spawn another process
 * \param screen Screen ID
 * \param arg Command
 * \ingroup ui_callback
 */
void
uicb_spawn(int screen, char *arg)
{
    static char *shell = NULL;
    char *display = NULL;
    char *tmp, newdisplay[128];

    if(!arg)
        return;

    if(!shell && !(shell = getenv("SHELL")))
        shell = a_strdup("/bin/sh");

    if(!globalconf.screens_info->xinerama_is_active && (tmp = getenv("DISPLAY")))
    {
        display = a_strdup(tmp);
        if((tmp = strrchr(display, '.')))
            *tmp = '\0';
        snprintf(newdisplay, sizeof(newdisplay), "%s.%d", display, screen);
        setenv("DISPLAY", newdisplay, 1);
    }


    /* The double-fork construct avoids zombie processes and keeps the code
     * clean from stupid signal handlers. */
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(globalconf.display)
                close(ConnectionNumber(globalconf.display));
            setsid();
            execl(shell, shell, "-c", arg, NULL);
            warn("execl '%s -c %s' failed: %s\n", shell, arg, strerror(errno));
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);
}

static int
run_uicb(char *cmd)
{
    char *p, *argcpy;
    const char *arg;
    int screen;
    ssize_t len;
    Uicb *uicb;

    len = a_strlen(cmd);
    p = strtok(cmd, " ");
    if (!p)
    {
        warn("ignoring malformed command\n");
        return -1;
    }
    screen = atoi(cmd);
    if(screen >= globalconf.screens_info->nscreen || screen < 0)
    {
        warn("invalid screen specified: %i\n", screen);
        return -1;
    }

    p = strtok(NULL, " ");
    if (!p)
    {
        warn("ignoring malformed command.\n");
        return -1;
    }

    uicb = name_func_lookup(p, UicbList);
    if (!uicb)
    {
        warn("unknown uicb function: %s.\n", p);
        return -1;
    }

    if (p + a_strlen(p) < cmd + len)
    {
        arg = p + a_strlen(p) + 1;
        len = a_strlen(arg);
        /* Allow our callees to modify this string. */
        argcpy = p_new(char, len + 1);
        a_strncpy(argcpy, len + 1,  arg, len);
        uicb(screen, argcpy);
        p_delete(&argcpy);
    }
    else
        uicb(screen, NULL);

    return 0;
}

int
parse_control(char *cmd)
{
    char *p, *curcmd = cmd;

    if(!a_strlen(cmd))
        return -1;

    while((p = strchr(curcmd, '\n')))
    {
        *p = '\0';
        run_uicb(curcmd);
        curcmd = p + 1;
    }

    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
