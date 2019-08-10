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

#include <unistd.h>
#include <stdbool.h>
#include <lua.h>

#include "x11/mousegrabber.h"
#include "globalconf.h"
#include "common/xcursor.h"
#include <mousegrabber.h>

bool x11_grab_mouse(const char *cursor_name)
{
    lua_State *L = globalconf_get_lua_State();

    uint16_t cfont = xcursor_font_fromstr(cursor_name);

    if(cfont)
    {
        xcb_cursor_t cursor = xcursor_new(globalconf.cursor_ctx, cfont);


        xcb_window_t root = globalconf.screen->root;

        for(int i = 1000; i; i--)
        {
            xcb_grab_pointer_reply_t *grab_ptr_r;
            xcb_grab_pointer_cookie_t grab_ptr_c =
                xcb_grab_pointer_unchecked(globalconf.connection, false, root,
                        XCB_EVENT_MASK_BUTTON_PRESS
                        | XCB_EVENT_MASK_BUTTON_RELEASE
                        | XCB_EVENT_MASK_POINTER_MOTION,
                        XCB_GRAB_MODE_ASYNC,
                        XCB_GRAB_MODE_ASYNC,
                        root, cursor, XCB_CURRENT_TIME);

            if((grab_ptr_r = xcb_grab_pointer_reply(globalconf.connection, grab_ptr_c, NULL)))
            {
                p_delete(&grab_ptr_r);
                return true;
            }
            usleep(1000);
        }
        return false;
    }
    else
    {
        luaA_warn(L, "invalid cursor");
        return false;
    }
}

void x11_release_mouse(void)
{
    xcb_ungrab_pointer(globalconf.connection, XCB_CURRENT_TIME);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
