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

#include "common.h"

LAYOUT_PROTO(layout_tile);
LAYOUT_PROTO(layout_tileleft);

UICB_PROTO(uicb_setnmaster);
UICB_PROTO(uicb_setncols);
UICB_PROTO(uicb_setmwfact);

#endif
