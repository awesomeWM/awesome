/*
 * strut.h - strut management header
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_STRUT_H
#define AWESOME_STRUT_H

#include <lua.h>
#include <stdint.h>
#include <stdbool.h>

/* Strut */
typedef struct
{
    uint16_t left, right, top, bottom;
    uint16_t left_start_y, left_end_y;
    uint16_t right_start_y, right_end_y;
    uint16_t top_start_x, top_end_x;
    uint16_t bottom_start_x, bottom_end_x;
} strut_t;

/** Check if a strut has information.
 * \param strut A strut structure.
 * \return A boolean value, true if the strut has strut information.
 */
static inline bool
strut_has_value(strut_t *strut)
{
    return (strut->left
            || strut->right
            || strut->top
            || strut->bottom
            || strut->left_start_y
            || strut->left_end_y
            || strut->right_start_y
            || strut->right_end_y
            || strut->top_start_x
            || strut->top_end_x
            || strut->bottom_start_x
            || strut->bottom_end_x);
}

int luaA_pushstrut(lua_State *, strut_t);
void luaA_tostrut(lua_State *, int, strut_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
