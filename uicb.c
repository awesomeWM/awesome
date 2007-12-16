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
#include "util.h"
#include "xutil.h"
#include "uicb.h"
#include "screen.h"
#include "tag.h"
#include "layout.h"
#include "mouse.h"
#include "statusbar.h"
#include "widget.h"
#include "focus.h"
#include "layouts/tile.h"

extern awesome_config globalconf;

const NameFuncLink UicbList[] =
{
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
    {"screen_focus", uicb_screen_focus},
    {"client_movetoscreen", uicb_client_movetoscreen},
    /* awesome.c */
    {"quit", uicb_quit},
    /* statusbar.c */
    {"statusbar_toggle", uicb_statusbar_toggle},
    {"statusbar_set_position", uicb_statusbar_set_position},
    {"statusbar_set_text", uicb_statusbar_set_text},
    /* mouse.c */
    {"client_movemouse", uicb_client_movemouse},
    {"client_resizemouse", uicb_client_resizemouse},
    /* focus.c */
    {"focus_history", uicb_focus_history},
    /* widgets.c */
    {"widget_tell", uicb_widget_tell},
    {NULL, NULL}
};

static int
run_uicb(char *cmd, awesome_config *awesomeconf)
{
    char *p;
    const char *arg;
    char *argcpy;
    int screen, len;
    void (*uicb) (int, const char *);

    len = strlen(cmd);
    p = strtok(cmd, " ");
    if (!p){
        warn("Ignoring malformed command\n");
        return -1;
    }
    screen = atoi(cmd);
    if(screen >= get_screen_count(awesomeconf->display) || screen < 0){
        warn("Invalid screen specified: %i\n", screen);
        return -1;
    }

    p = strtok(NULL, " ");
    if (!p){
        warn("Ignoring malformed command.\n");
        return -1;
    }
    uicb = name_func_lookup(p, UicbList);
    if (!uicb){
        warn("Unknown UICB function: %s.\n", p);
        return -1;
    }

    if (p+strlen(p) < cmd+len)
    {
        arg = p + strlen(p) + 1;
        /* Allow our callees to modify this string. */
        argcpy = p_new(char, strlen(arg)+1);
        strncpy(argcpy, arg, strlen(arg));
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
        run_uicb(curcmd, &globalconf);
        curcmd = p + 1;
    }
    return 0;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
