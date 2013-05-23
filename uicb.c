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

/** Restart awesome with the current command line.
 * \param screen ID
 * \param arg arg (unused)
 * \ingroup ui_callback
 */
void
uicb_restart(int screen, char *arg __attribute__ ((unused)))
{
    uicb_exec(screen, globalconf.argv);
}

/** Execute another process, replacing the current instance of awesome.
 * \param screen Screen ID
 * \param cmd Command
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
    XCloseDisplay(globalconf.display);

    /* Ignore the leading spaces if any */
    while(cmd[0] && cmd[0] == ' ')
        cmd++;

    /* Get the beginning of the arguments */
    args = strchr(cmd, ' ');

    if(args)
        path = a_strndup(cmd, args - cmd);
    else
        path = a_strndup(cmd, a_strlen(cmd));

    execlp(path, cmd, NULL);

    p_delete(&path);
}

/** Spawn another process.
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
        if ((tmp = strrchr(display, '.')) && !strchr(tmp, ':'))
            *tmp = '\0';
        snprintf(newdisplay, sizeof(newdisplay), "%s.%d", display, screen);
        setenv("DISPLAY", newdisplay, 1);
        p_delete(&display);
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

static void
__uicb_run_args(Uicb* fun, int screen, char *cmd) {
    char *argcpy = NULL;
    const char *arg;
    ssize_t len;

    if (cmd && (a_strlen(cmd) > 0)) {
        ssize_t len = a_strlen(cmd);
        argcpy = p_new(char, len);
        a_strncpy(argcpy, len + 1, cmd, len);
    }

    fun(screen, argcpy);
    if (argcpy)
        p_delete(&argcpy);
}

/** Run the uicb
 * \param cmd the uicb command to parse
 * \return 0 on succes, -1 on failure
 */
static int
__uicb_run(char *cmd)
{
    char *p, *argcpy;
    const char *arg;
    char *tmp;
    int screen;
    ssize_t len;
    Uicb *uicb;

    len = a_strlen(cmd);

    p = strsep(&cmd, " ");
    screen = (int) strtol(p, &tmp, 10);
    if ((*tmp != '\0') || (screen > globalconf.screens_info->nscreen) || (screen < -1)) {
        warn("invalid screen: %s\n", p);
        return -1;
    }

    p = strsep(&cmd, " ");
    if ((!p) || (*p == '\0')) {
        warn("ignoring malformed command\n");
        return -1;
    }
    uicb = name_func_lookup(p, UicbList);
    if (!uicb)
    {
        warn("unknown uicb function: %s.\n", p);
        return -1;
    }

    if (screen == -1) {
        for (int x = 0; x < globalconf.screens_info->nscreen; x++)
            __uicb_run_args(uicb, x, cmd);
    } else
        __uicb_run_args(uicb, screen, cmd);

    return 0;
}

/** Parse the control buffer.
 * \param cmd the control buffer
 * \return 0 on succes, -1 on failure
 */
int
parse_control(char *cmd)
{
    char *p, *curcmd = cmd;

    if(!a_strlen(cmd))
        return -1;

    while((p = strchr(curcmd, '\n')))
    {
        *p = '\0';
        __uicb_run(curcmd);
        curcmd = p + 1;
    }

    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
