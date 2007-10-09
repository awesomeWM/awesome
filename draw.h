/*
 * draw.h - draw functions header
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

#ifndef AWESOME_DRAW_H
#define AWESOME_DRAW_H

#include "config.h"

void drawtext(Display *, int, int, int, int, int, GC, Drawable, XftFont *, const char *, unsigned long *, XColor);
void drawsquare(Display *, int, int, int, GC, Drawable, Bool, unsigned long);
inline unsigned short textwidth(Display *, XftFont *, char *, ssize_t);
#endif
