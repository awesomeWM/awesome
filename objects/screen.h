/*
 * screen.h - screen management header
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

#ifndef AWESOME_SCREEN_H
#define AWESOME_SCREEN_H

#include "globalconf.h"
#include "draw.h"
#include "common/array.h"
#include "common/luaclass.h"

typedef struct screen_output_t screen_output_t;
ARRAY_TYPE(screen_output_t, screen_output)

/** Different ways to manage screens */
typedef enum {
    SCREEN_LIFECYCLE_USER =        0, /*!< Unmanaged (ei. from fake_add) */
    SCREEN_LIFECYCLE_LUA  = 0x1 << 0, /*!< Is managed internally by Lua  */
    SCREEN_LIFECYCLE_C    = 0x1 << 1, /*!< Is managed internally by C    */
} screen_lifecycle_t;

struct a_screen
{
    LUA_OBJECT_HEADER
    bool valid;
    /** Who manages the screen lifecycle */
    screen_lifecycle_t lifecycle;
    /** Screen geometry */
    area_t geometry;
    /** Screen workarea */
    area_t workarea;
    /** Opaque pointer to the viewport */
    struct viewport_t *viewport;
    /** Some XID identifying this screen */
    uint32_t xid;
};
ARRAY_FUNCS(screen_t *, screen, DO_NOTHING)

void screen_class_setup(lua_State *L);
void screen_scan(void);
screen_t *screen_getbycoord(int, int);
bool screen_coord_in_screen(screen_t *, int, int);
bool screen_area_in_screen(screen_t *, area_t);
int screen_get_index(screen_t *);
void screen_client_moveto(client_t *, screen_t *, bool);
void screen_update_primary(void);
void screen_update_workarea(screen_t *);
screen_t *screen_get_primary(void);
void screen_schedule_refresh(void);
void screen_emit_scanned(void);
void screen_emit_scanning(void);
void screen_cleanup(void);

screen_t *luaA_checkscreen(lua_State *, int);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
