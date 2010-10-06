/*
 * color.h - color functions header
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2009 Uli Schlachter <psychon@znc.in>
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

#ifndef AWESOME_COLOR_H
#define AWESOME_COLOR_H

#include <xcb/xcb.h>
#include <stdbool.h>
#include <lua.h>

typedef struct
{
    uint32_t pixel;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t alpha;
    bool initialized;
} xcolor_t;

typedef struct
{
    xcb_alloc_color_cookie_t cookie_hexa;
    uint16_t alpha;
    xcolor_t *color;
    bool has_error;
    const char *colstr;
} xcolor_init_request_t;

xcolor_init_request_t xcolor_init_unchecked(xcolor_t *, const char *, ssize_t);
bool xcolor_init_reply(xcolor_init_request_t);

int luaA_pushxcolor(lua_State *, const xcolor_t);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
