/*
 * screen.h - screen management header
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

#ifndef AWESOME_SCREEN_H
#define AWESOME_SCREEN_H

#include "client.h"

#include <X11/extensions/Xinerama.h>

typedef XineramaScreenInfo ScreenInfo;

ScreenInfo * get_screen_info(Display *, int, Statusbar *, Padding *);
ScreenInfo * get_display_info(Display *, int, Statusbar *, Padding *);
int get_screen_bycoord(Display *, int, int);
int get_screen_count(Display *);
int get_phys_screen(Display *, int);
void move_client_to_screen(Client *, awesome_config *, int, Bool);

UICB_PROTO(uicb_screen_focusnext);
UICB_PROTO(uicb_screen_focusprev);
UICB_PROTO(uicb_client_movetoscreen);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
