/*
 * window.h - window handling functions header
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

#ifndef AWESOME_WINDOW_H
#define AWESOME_WINDOW_H

#include <X11/Xlib.h> 
#include "config.h"

/** Mask shorthands, used in event.c and window.c */ 
#define BUTTONMASK              (ButtonPressMask | ButtonReleaseMask) 

int window_setstate(Display *, Window, long);
long window_getstate(Display *, Window);
Status window_configure(Display *, Window, int, int, int, int, int);
void window_grabbuttons(Display *, int, Window, Bool, Bool, Button *, KeySym, unsigned int);
void window_setshape(Display *, int, Window);
void window_settrans(Display *, Window, double);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
