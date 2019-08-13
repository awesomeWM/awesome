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

#include "globalconf.h"
#include <root.h>
#include "common/array.h"
#include "wayland/root.h"
#include "way-cooler-keybindings-unstable-v1.h"

#include <xcb/xcb.h>

extern struct root_impl root_impl;

static void on_key(void *data, struct zway_cooler_keybindings *keybindings,
        uint32_t time, uint32_t keycode, uint32_t state, uint32_t mods)
{
    /* get keysym ignoring all modifiers */
    xcb_keysym_t keysym =
        xcb_key_symbols_get_keysym(globalconf.keysyms, keycode, 0);
    bool pressed = state == ZWAY_COOLER_KEYBINDINGS_KEY_STATE_PRESSED;

    // TODO Check if intersects client, like in X11, and emit proper signals.

    root_handle_key(&globalconf.keys, false, time, keycode, mods,
            pressed, keysym, &root_impl);
}

struct zway_cooler_keybindings_listener keybindings_listener =
{
    .key = on_key,
};

void wayland_grab_keys(void) {
    zway_cooler_keybindings_clear_keys(globalconf.wl_keybindings);

    foreach(key_, globalconf.keys)
    {
        keyb_t *key = *key_;

        if (key->keycode)
        {
            zway_cooler_keybindings_register_key(globalconf.wl_keybindings,
                    key->keycode, key->modifiers);
        }
        else
        {
            xcb_keycode_t *keycodes =
                xcb_key_symbols_get_keycode(globalconf.keysyms, key->keysym);
            if(keycodes)
            {
                for(xcb_keycode_t *keycode = keycodes; *keycode; keycode++)
                {
                    zway_cooler_keybindings_register_key(globalconf.wl_keybindings,
                            *keycode, key->modifiers);
                }
            }
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
