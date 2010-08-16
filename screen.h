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

typedef struct screen_output_t screen_output_t;
ARRAY_TYPE(screen_output_t, screen_output)

struct a_screen
{
    /** Screen geometry */
    area_t geometry;
    /** Tag list */
    tag_array_t tags;
    /** Window that contains the systray */
    struct
    {
        xcb_window_t window;
        /** Systray window parent */
        xcb_window_t parent;
        /** Is awesome the systray owner? */
        bool registered;
    } systray;
    /** Previously focused client */
    client_t *prev_client_focus;
    /** Focused client */
    client_t *client_focus;
    /** The monitor of startup notifications */
    SnMonitorContext *snmonitor;
    /** The default visual, used to draw */
    xcb_visualtype_t *visual;
    /** The signals emitted by screen objects */
    signal_array_t signals;
    /** True if the banning on this screen needs to be updated */
    bool need_lazy_banning;
    /** The screen outputs informations */
    screen_output_array_t outputs;
};
ARRAY_FUNCS(screen_t, screen, DO_NOTHING)

void screen_emit_signal(lua_State *, screen_t *, const char *, int);
void screen_scan(void);
screen_t *screen_getbycoord(screen_t *, int, int);
area_t screen_area_get(screen_t *, bool);
area_t display_area_get(void);
void screen_client_moveto(client_t *, screen_t *, bool);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
