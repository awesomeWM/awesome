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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

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

const name_func_link_t UicbList[] =
{
    /* awesome.h */
    { "quit", uicb_quit },
    /* client.h */
    { "client_kill", uicb_client_kill },
    { "client_moveresize", uicb_client_moveresize },
    { "client_settrans", uicb_client_settrans },
    { "client_swap", uicb_client_swap },
    { "client_togglemax", uicb_client_togglemax },
    { "client_focus", uicb_client_focus },
    { "client_setfloating", uicb_client_setfloating },
    { "client_togglescratch", uicb_client_togglescratch },
    { "client_setscratch", uicb_client_setscratch },
    /* focus.h */
    { "focus_history", uicb_focus_history },
    { "focus_client_byname", uicb_focus_client_byname },
    /* layout.h */
    { "tag_setlayout", uicb_tag_setlayout },
    /* mouse.h */
    { "client_movemouse", uicb_client_movemouse },
    { "client_resizemouse", uicb_client_resizemouse },
    /* screen.h */
    { "screen_focus", uicb_screen_focus },
    { "client_movetoscreen", uicb_client_movetoscreen },
    /* statusbar.h */
    { "statusbar_toggle", uicb_statusbar_toggle },
    /* tag.h */
    { "client_tag", uicb_client_tag },
    { "client_toggletag", uicb_client_toggletag },
    { "tag_toggleview", uicb_tag_toggleview },
    { "tag_view", uicb_tag_view },
    { "tag_prev_selected", uicb_tag_prev_selected },
    { "tag_viewnext", uicb_tag_viewnext },
    { "tag_viewprev", uicb_tag_viewprev },
    { "tag_create", uicb_tag_create },
    /* titlebar.h */
    { "client_toggletitlebar", uicb_client_toggletitlebar },
    /* uicb.h */
    { "restart", uicb_restart },
    { "exec", uicb_exec },
    { "spawn", uicb_spawn },
    /* widget.h */
    { "widget_tell", uicb_widget_tell },
    /* layouts/tile.h */
    { "tag_setnmaster", uicb_tag_setnmaster},
    { "tag_setncol", uicb_tag_setncol },
    { "tag_setmwfact", uicb_tag_setmwfact },
    { NULL, NULL }
};

/** Restart awesome with the current command line.
 * \param screen The virtual screen number.
 * \param arg An unused argument.
 * \ingroup ui_callback
 */
void
uicb_restart(int screen, char *arg __attribute__ ((unused)))
{
    uicb_exec(screen, globalconf.argv);
}

/** Execute another process, replacing the current instance of awesome.
 * \param screen The virtual screen number.
 * \param cmd The command to start.
 * \ingroup ui_callback
 */
void
uicb_exec(int screen __attribute__ ((unused)), char *cmd)
{
    client_t *c;
    char *args, *path;

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);
    xcb_disconnect(globalconf.connection);

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
 * \param screen The virtual screen number.
 * \param arg The command to run.
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
            if(globalconf.connection)
                close(xcb_get_file_descriptor(globalconf.connection));
            setsid();
            execl(shell, shell, "-c", arg, NULL);
            warn("execl '%s -c %s' failed: %s\n", shell, arg, strerror(errno));
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);
}

/** Run the uicb.
 * \param cmd The uicb command to parse.
 * \return true on succes, false on failure.
 */
static bool
__uicb_run(char *cmd)
{
    char *p, *argcpy;
    const char *arg;
    int screen;
    ssize_t len;
    uicb_t *uicb;

    len = a_strlen(cmd);
    p = strtok(cmd, " ");
    if (!p)
    {
        warn("ignoring malformed command\n");
        return false;
    }
    screen = atoi(cmd);
    if(screen >= globalconf.screens_info->nscreen || screen < 0)
    {
        warn("invalid screen specified: %i\n", screen);
        return false;
    }

    p = strtok(NULL, " ");
    if (!p)
    {
        warn("ignoring malformed command.\n");
        return false;
    }

    uicb = name_func_lookup(p, UicbList);
    if (!uicb)
    {
        warn("unknown uicb function: %s.\n", p);
        return false;
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

    return true;
}

/** Parse a command.
 * \param cmd The buffer to parse.
 * \return true on succes, false on failure.
 */
bool
__uicb_parsecmd(char *cmd)
{
    char *p, *curcmd = cmd;
    bool ret = false;

    if(a_strlen(cmd))
        while((p = strchr(curcmd, '\n')))
        {
            *p = '\0';
            ret = __uicb_run(curcmd);
            curcmd = p + 1;
        }

    return ret;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
