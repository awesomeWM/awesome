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
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    bool initialized;
} color_t;

typedef struct
{
    color_t *color;
    const char *colstr;
    xcb_alloc_named_color_cookie_t cookie;
    bool has_error;
} color_init_cookie_t;

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
    union
    {
        xcb_alloc_color_cookie_t cookie_hexa;
        xcb_alloc_named_color_cookie_t cookie_named;
    };

    uint16_t alpha;
    xcolor_t *color;
    bool is_hexa, has_error;
    const char *colstr;
} xcolor_init_request_t;

color_init_cookie_t color_init_unchecked(color_t *, const char *, ssize_t);
bool color_init_reply(color_init_cookie_t);

xcolor_init_request_t xcolor_init_unchecked(xcolor_t *, const char *, ssize_t);
bool xcolor_init_reply(xcolor_init_request_t);

bool xcolor_to_color(const xcolor_t *, color_t *);

int luaA_pushxcolor(lua_State *, const xcolor_t);
int luaA_pushcolor(lua_State *, const color_t *);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
