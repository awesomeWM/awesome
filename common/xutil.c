/*
 * common/xutil.c - X-related useful functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

/* strndup() */
#define _GNU_SOURCE

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xinerama.h>

#include "common/util.h"
#include "common/xutil.h"

bool
xgettextprop(xcb_connection_t *conn, xcb_window_t w, xcb_atom_t atom,
             char *text, ssize_t textlen)
{
    xcb_get_property_reply_t *name = NULL;
    char *prop_val = NULL;

    if(!text || !textlen)
        return false;

    text[0] = '\0';
    name = xcb_get_property_reply(conn,
                                  xcb_get_property_unchecked(conn, false,
                                                             w, atom,
                                                             XCB_GET_PROPERTY_TYPE_ANY,
                                                             0L, 1000000L),
                                  NULL);

    if(!name->value_len)
        return false;

    prop_val = (char *) xcb_get_property_value(name);

    /* TODO: XCB doesn't provide a XmbTextPropertyToTextList(), check 
     * whether this code is correct (locales) */
    if(name->type == STRING || name->format == 8)
        a_strncpy(text, name->value_len + 1, prop_val, textlen - 1);

    text[textlen - 1] = '\0';
    p_delete(&name);

    return true;
}

void
xutil_get_lock_mask(xcb_connection_t *conn, xcb_key_symbols_t *keysyms,
                    unsigned int *numlockmask, unsigned int *shiftlockmask,
                    unsigned int *capslockmask)
{
    xcb_get_modifier_mapping_reply_t *modmap_r;
    xcb_keycode_t *modmap, kc;
    unsigned int mask;
    int i, j;

    modmap_r = xcb_get_modifier_mapping_reply(conn,
                                              xcb_get_modifier_mapping_unchecked(conn),
                                              NULL);

    modmap = xcb_get_modifier_mapping_keycodes(modmap_r);

    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap_r->keycodes_per_modifier; j++)
        {
            kc = modmap[i * modmap_r->keycodes_per_modifier + j];
            mask = (1 << i);

            if(numlockmask != NULL && 
               kc == xcb_key_symbols_get_keycode(keysyms, XK_Num_Lock))
                *numlockmask = mask;
            else if(shiftlockmask != NULL &&
                    kc == xcb_key_symbols_get_keycode(keysyms, XK_Shift_Lock))
                *shiftlockmask = mask;
            else if(capslockmask != NULL &&
                    kc == xcb_key_symbols_get_keycode(keysyms, XK_Caps_Lock))
                *capslockmask = mask;
        }

    p_delete(&modmap_r);
}

/** Equivalent to 'XGetTransientForHint' which is actually a
 * 'XGetWindowProperty' which gets the WM_TRANSIENT_FOR property of
 * the specified window
 *
 * \param c X connection
 * \param win get the property from this window
 * \param prop_win returns the WM_TRANSIENT_FOR property of win
 * \return return true if successfull
 */
bool
xutil_get_transient_for_hint(xcb_connection_t *c, xcb_window_t win,
			 xcb_window_t *prop_win)
{
    xcb_get_property_reply_t *r;

    /* Use checked because the error handler should not take care of
     * this error as we only return a boolean */
    r = xcb_get_property_reply(c,
                               xcb_get_property(c, false, win,
                                                WM_TRANSIENT_FOR,
                                                WINDOW, 0, 1),
                               NULL);

    if(!r || r->type != WINDOW || r->format != 32 || r->length == 0)
    {
        *prop_win = XCB_NONE;
        if(r)
            p_delete(&r);

        return false;
    }

    *prop_win = *((xcb_window_t *) xcb_get_property_value(r));
    p_delete(&r);

    return true;
}

xcb_atom_t
xutil_intern_atom(xcb_connection_t *c, const char *property)
{
    xcb_atom_t atom;
    xcb_intern_atom_reply_t *r_atom;

    r_atom = xcb_intern_atom_reply(c,
				   xcb_intern_atom_unchecked(c, false, strlen(property), property),
				   NULL);

    if(!r_atom)
	return 0;

    atom = r_atom->atom;
    p_delete(&r_atom);

    return atom;
}

class_hint_t *
xutil_get_class_hint(xcb_connection_t *conn, xcb_window_t win)
{
    xcb_get_property_reply_t *r = NULL;
    char *data = NULL;

    int len_name, len_class;

    class_hint_t *ch = p_new(class_hint_t, 1);

    /* TODO: 2048? (BUFINC is declared as private in xcb.c) */
    r = xcb_get_property_reply(conn,
			       xcb_get_property_unchecked(conn,
							  false, win, WM_CLASS,
							  STRING, 0L, 2048),
			       NULL);

    if(!r || r->type != STRING || r->format != 8)
	return NULL;

    data = xcb_get_property_value(r);

    len_name = strlen((char *) data);
    len_class = strlen((char *) (data + len_name + 1));

    ch->res_name = strndup(data, len_name);
    ch->res_class = strndup(data + len_name + 1, len_class);

    p_delete(&r);

    return ch;
}

void
xutil_map_raised(xcb_connection_t *conn, xcb_window_t win)
{
    const uint32_t map_raised_val = XCB_STACK_MODE_ABOVE;

    xcb_configure_window(conn, win, XCB_CONFIG_WINDOW_STACK_MODE,
                         &map_raised_val);

    xcb_map_window(conn, win);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
