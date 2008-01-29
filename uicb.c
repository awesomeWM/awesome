/*
 * uicb.c - user interface callbacks management
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include "awesome.h"
#include "xutil.h"
#include "tag.h"
#include "mouse.h"
#include "statusbar.h"
#include "widget.h"
#include "focus.h"
#include "client.h"
#include "screen.h"
#include "layouts/tile.h"

extern AwesomeConf globalconf;

#include "uicbgen.h"

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
        warn("Ignoring malformed command\n");
        return -1;
    }
    screen = atoi(cmd);
    if(screen >= globalconf.nscreen || screen < 0)
    {
        warn("Invalid screen specified: %i\n", screen);
        return -1;
    }

    p = strtok(NULL, " ");
    if (!p)
    {
        warn("Ignoring malformed command.\n");
        return -1;
    }

    uicb = name_func_lookup(p, UicbList);
    if (!uicb)
    {
        warn("Unknown UICB function: %s.\n", p);
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
