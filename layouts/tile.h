/* 
 * tile.h - tile layout
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info> 
 * Copyright © 2007 Ross Mohn <rpmohn@waxandwane.org>
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

#ifndef AWESOME_TILE_H
#define AWESOME_TILE_H

#include <config.h>

void uicb_setnmaster(Display *, DC *, awesome_config *, const char *);        /* change number of master windows */
void uicb_setncols(Display *, DC *, awesome_config *, const char *);
void uicb_setmwfact(Display *, DC *, awesome_config *, const char *);        /* sets master width factor */
void tile(Display *, awesome_config *);
void tileleft(Display *, awesome_config *);

#endif
