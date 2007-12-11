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
#include "uicb.h"
#include "screen.h"
#include "tag.h"
#include "layout.h"
#include "mouse.h"
#include "statusbar.h"
#include "layouts/tile.h"

const NameFuncLink UicbList[] = {
    /* xutil.c */
    {"spawn", uicb_spawn},
    {"exec", uicb_exec},
    /* client.c */
    {"client_kill", uicb_client_kill},
    {"client_moveresize", uicb_client_moveresize},
    {"client_settrans", uicb_client_settrans},
    {"setborder", uicb_setborder},
    {"client_swapnext", uicb_client_swapnext},
    {"client_swapprev", uicb_client_swapprev},
    /* tag.c */
    {"client_tag", uicb_client_tag},
    {"client_togglefloating", uicb_client_togglefloating},
    {"tag_toggleview", uicb_tag_toggleview},
    {"client_toggletag", uicb_client_toggletag},
    {"tag_view", uicb_tag_view},
    {"tag_viewprev_selected", uicb_tag_prev_selected},
    {"tag_viewprev", uicb_tag_viewprev},
    {"tag_viewnext", uicb_tag_viewnext},
    /* layout.c */
    {"tag_setlayout", uicb_tag_setlayout},
    {"client_focusnext", uicb_client_focusnext},
    {"client_focusprev", uicb_client_focusprev}, 
    {"client_togglemax", uicb_client_togglemax},
    {"client_toggleverticalmax", uicb_client_toggleverticalmax},
    {"client_togglehorizontalmax", uicb_client_togglehorizontalmax},
    {"client_zoom", uicb_client_zoom},
    /* layouts/tile.c */
    {"tag_setmwfact", uicb_tag_setmwfact},
    {"tag_setnmaster", uicb_tag_setnmaster},
    {"tag_setncol", uicb_tag_setncol},
    /* screen.c */
    {"screen_focusnext", uicb_screen_focusnext},
    {"screen_focusprev", uicb_screen_focusprev},
    {"client_movetoscreen", uicb_client_movetoscreen},
    /* awesome.c */
    {"quit", uicb_quit},
    /* statusbar.c */
    {"togglebar", uicb_togglebar},
    /* config.c */
    {"setstatustext", uicb_setstatustext},
    /* mouse.c */
    {"client_movemouse", uicb_client_movemouse},
    {"client_resizemouse", uicb_client_resizemouse},
    {NULL, NULL}
};

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
    char *p, *uicb_name;
    const char *arg;
    int screen;
    void (*uicb) (awesome_config *, int, const char *);

    if(!a_strlen(cmd))
        return -1;

    if(!(p = strchr(cmd, ' ')))
        return -1;

    *p++ = '\0';

    if((screen = atoi(cmd)) >= get_screen_count(awesomeconf->display))
        return -1;

    uicb_name = p;

    if(!(p = strchr(p, ' ')))
        arg = "";
    else
    {
        *p++ = '\0';
        arg = p;
    }

    if((p = strchr(arg, '\n')))
        *p = '\0';

    uicb = name_func_lookup(uicb_name, UicbList);

    if(uicb)
        uicb(awesomeconf, screen, arg);
    else
        return -1;

    return 0;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
