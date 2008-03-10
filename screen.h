/*
 * screen.h - screen management header
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

#ifndef AWESOME_SCREEN_H
#define AWESOME_SCREEN_H

#include "structs.h"

typedef struct
{
    int nscreen;
    Area *geometry;
} ScreensInfo;

Area screen_get_area(int, Statusbar *, Padding *);
Area get_display_area(int, Statusbar *, Padding *);
int screen_get_bycoord(int, int, int);
void screensinfo_delete(ScreensInfo **);
ScreensInfo *screensinfo_new(Display *);
int get_phys_screen(int);
void move_client_to_screen(Client *, int, Bool);

Uicb uicb_screen_focus;
Uicb uicb_client_movetoscreen;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
