/*
 * statusbar.h - statusbar functions header
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

#ifndef AWESOME_STATUSBAR_H
#define AWESOME_STATUSBAR_H

#include "common.h"

void initstatusbar(Display *, int, Statusbar *, Cursor, XftFont *, Layout *, int, Padding *);
void drawstatusbar(awesome_config *, int);
void updatebarpos(Display *, Statusbar, Padding *);

UICB_PROTO(uicb_togglebar);
UICB_PROTO(uicb_setstatustext);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
