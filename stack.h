/*
 * stack.h - client stack management header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_STACK_H
#define AWESOME_STACK_H
#include "luaa.h"
#include "objects/client.h"
#include "objects/drawin.h"

typedef struct client_t client_t;

void stack_client_remove(lua_State *, client_t *, bool, const char *);
void stack_client_push(lua_State *, client_t *, const char *);
void stack_client_append(lua_State *, client_t *, const char *);
void stack_windows(lua_State *, const char *, client_t *, drawin_t *);
int luaA_set_stacking_order(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
