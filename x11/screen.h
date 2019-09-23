/*
 * screen.c - screen management
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#ifndef AWESOME_X11_SCREEN_H
#define AWESOME_X11_SCREEN_H

#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>

#include "common/array.h"
#include "common/luaclass.h"
#include "objects/screen.h"

/* The XID that is used on fake screens. X11 guarantees that the top three bits
 * of a valid XID are zero, so this will not clash with anything.
 */
#define FAKE_SCREEN_XID ((uint32_t) 0xffffffff)

DO_ARRAY(xcb_randr_output_t, randr_output, DO_NOTHING);

struct screen_output_t
{
    /** The XRandR names of the output */
    char *name;
    /** The size in millimeters */
    uint32_t mm_width, mm_height;
    /** The XID */
    randr_output_array_t outputs;
};

typedef struct screen_output_t screen_output_t;
ARRAY_TYPE(screen_output_t, screen_output);

/** Keep track of the screen viewport(s) independently from the screen objects.
 *
 * A viewport is a collection of `outputs` objects and their associated
 * metadata. This structure is copied into Lua and then further extended from
 * there. The `id` field allows to differentiate between viewports that share
 * the same position and dimensions without having to rely on userdata pointer
 * comparison.
 *
 * Screen objects are widely used by the public API and imply a very "visible"
 * concept. A viewport is a subset of what the concerns the "screen" class
 * previously handled. It is meant to be used by some low level Lua logic to
 * create screens from Lua rather than from C. This is required to increase the
 * flexibility of multi-screen setup or when screens are connected and
 * disconnected often.
 *
 * Design rationals:
 *
 * * The structure is not directly shared with Lua to avoid having to use the
 *   slow "miss_handler" and unsafe "valid" systems used by other CAPI objects.
 * * The `viewport_t` implements a linked-list because its main purpose is to
 *   offers a deduplication algorithm. Random access is never required.
 * * Everything that can be done in Lua is done in Lua.
 * * Since the legacy and "new" way to initialize screens share a lot of steps,
 *   the C code is bent to share as much code as possible. This will reduce the
 *   "dead code" and improve code coverage by the tests.
 *
 */
typedef struct x11_viewport_t
{
    bool marked;
    int x;
    int y;
    int width;
    int height;
    int id;
    struct x11_viewport_t *next;
    screen_t *screen;
    screen_output_array_t outputs;
} x11_viewport_t;

struct x11_screen
{
    /** Some XID identifying this screen */
    uint32_t xid;
};

void x11_new_screen(screen_t *screen);
void x11_wipe_screen(screen_t *screen);
void x11_cleanup_screens(void);
void x11_mark_fake_screen(screen_t *screen);
void x11_scan_screens(void);
void x11_get_screens(lua_State *L, struct screen_array_t *screens);

int x11_viewport_get_outputs(lua_State *L, void *viewport);
int x11_get_viewports(lua_State *L);
int x11_get_outputs(lua_State *L, screen_t *s);

screen_t *x11_update_primary(void);
screen_t *x11_screen_by_name(char *name);

bool x11_outputs_changed(screen_t *existing, screen_t *other);
bool x11_does_screen_exist(screen_t *screen, screen_array_t screens);
bool x11_is_fake_screen(screen_t *screen);
bool x11_is_same_screen(screen_t *left, screen_t *right);

#endif // AWESOME_X11_SCREEN_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
