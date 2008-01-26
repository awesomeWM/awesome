/*
 * swindow.h - simple window handling functions header
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_COMMON_SWINDOW_H
#define AWESOME_COMMON_SWINDOW_H

/** A simple window */
typedef struct SimpleWindow
{
    Display *display;
    Window window;
    Drawable drawable;
    Area geometry;
} SimpleWindow;

SimpleWindow * simplewindow_new(Display *, int, int, int, unsigned int, unsigned int, unsigned int);
void simplewindow_delete(SimpleWindow *);
int simplewindow_move(SimpleWindow *, int, int);
int simplewindow_refresh_drawable(SimpleWindow *, int);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
