/* 
 * awesome.h - awesome main header
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

#ifndef AWESOME_AWESOME_H
#define AWESOME_AWESOME_H

#include "config.h"

Bool gettextprop(Display *, Window, Atom, char *, unsigned int);   /* return text property, UTF-8 compliant */
void updatebarpos(Display *, Statusbar);        /* updates the bar position */
void uicb_quit(Display *, DC *, awesome_config *, const char *);        /* quit awesome nicely */
int xerror(Display *, XErrorEvent *);   /* awesome's X error handler */
int __attribute__ ((deprecated)) get_windows_area_x(Statusbar);
int __attribute__ ((deprecated)) get_windows_area_y(Statusbar);
int __attribute__ ((deprecated)) get_windows_area_height(Display *, Statusbar);
int __attribute__ ((deprecated)) get_windows_area_width(Display *, Statusbar);

#endif
