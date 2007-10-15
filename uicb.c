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

#include <string.h>

#include "util.h"
#include "uicb.h"
#include "screen.h"

extern const NameFuncLink UicbList[];

int
parse_control(char *cmd, awesome_config *awesomeconf)
{
    char *p, *curcmd = cmd;

    if(!a_strlen(cmd))
        return -1;
    
    while((p = strchr(curcmd, '\n')))
    {
        *p = '\0';
        run_uicb(curcmd, awesomeconf);
        curcmd = p + 1;
    }

    return 0;
}

int
run_uicb(char *cmd, awesome_config *awesomeconf)
{
    char *p, *uicb_name, *arg;
    int screen;
    void (*uicb) (awesome_config *, char *);

    if(!a_strlen(cmd))
        return -1;

    if(!(p = strchr(cmd, ' ')))
        return -1;

    *p++ = '\0';

    if((screen = atoi(cmd)) >= get_screen_count(awesomeconf->display))
        return -1;

    uicb_name = p;

    if(!(p = strchr(p, ' ')))
       return -1;

    *p++ = '\0';

    arg = p;

    if((p = strchr(arg, '\n')))
        *p = '\0';

    uicb = name_func_lookup(uicb_name, UicbList);

    if(uicb)
        uicb(&awesomeconf[screen], arg);
    else
        return -1;

    return 0;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
