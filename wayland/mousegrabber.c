/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#include <lua.h>

#include "globalconf.h"
#include <mousegrabber.h>
#include "wayland/mousegrabber.h"

static void event_mouse_moved(void *data, struct zway_cooler_mousegrabber *mousegrabber,
        int32_t x, int32_t y, uint32_t button)
{
    lua_State *L = globalconf_get_lua_State();

    uint16_t mask = button << 8;
    mousegrabber_handleevent(L, x, y, mask);
}

static void event_mouse_button(void *data, struct zway_cooler_mousegrabber *mousegrabber,
        int32_t x, int32_t y, uint32_t button)
{
    lua_State *L = globalconf_get_lua_State();

    mousegrabber_handleevent(L, x, y, button);
}

struct zway_cooler_mousegrabber_listener mousegrabber_listener =
{
    .mouse_moved = event_mouse_moved,
    .mouse_button = event_mouse_button,
};

bool wayland_grab_mouse(const char *cursor)
{
    zway_cooler_mousegrabber_grab_mouse(globalconf.wl_mousegrabber, cursor);
    wl_display_roundtrip(globalconf.wl_display);
    const struct wl_interface *interface = NULL;
    int err = wl_display_get_protocol_error(globalconf.wl_display,
            &interface, NULL);
    if (interface && strcmp(interface->name,  zway_cooler_mousegrabber_interface.name) == 0)
    {
        switch ((enum zway_cooler_mousegrabber_error)err) {
        case ZWAY_COOLER_MOUSEGRABBER_ERROR_ALREADY_GRABBED:
            break;
        case ZWAY_COOLER_MOUSEGRABBER_ERROR_NOT_GRABBED:
            fatal("Unexpected error from compositor's mouse grabber protocol");
            break;
        }
        return false;
    }
    return true;

}

void wayland_release_mouse(void)
{
    zway_cooler_mousegrabber_release_mouse(globalconf.wl_mousegrabber);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
