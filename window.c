/*
 * window.c - window handling functions
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

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/shape.h>
#include <xcb/xcb_keysyms.h>

#include "structs.h"
#include "window.h"

extern AwesomeConf globalconf;

/** Mask shorthands */
#define BUTTONMASK     (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE)

/** Set client WM_STATE property
 * \param win Window
 * \param state state
 * \return XChangeProperty result
 */
void
window_setstate(xcb_window_t win, long state)
{
    long data[] = { state, XCB_NONE };

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                        x_intern_atom(globalconf.connection, "WM_STATE"),
                        x_intern_atom(globalconf.connection, "WM_STATE"), 32,
                        2, data);
}

/** Get a window state (WM_STATE)
 * \param w Client window
 * \return state
 */
long
window_getstate(xcb_window_t w)
{
    long result = -1;
    unsigned char *p = NULL;
    xcb_get_property_cookie_t prop_c;
    xcb_atom_t wm_state_atom = x_intern_atom(globalconf.connection, "WM_STATE");
    xcb_get_property_reply_t *prop_r;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false, w,
                                        wm_state_atom, wm_state_atom,
                                        0L, 2L);

    if((prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)) == NULL)
        return -1;

    p = xcb_get_property_value(prop_r);
    if(xcb_get_property_value_length(prop_r) != 0)
        result = *p;

    p_delete(&prop_r);

    return result;
}

/** Configure a window with its new geometry and border
 * \param win the X window id
 * \param geometry the new window geometry
 * \param border new border size
 * \return the XSendEvent() status
 */
void
window_configure(xcb_window_t win, area_t geometry, int border)
{
    xcb_configure_notify_event_t ce;

    ce.response_type = XCB_CONFIGURE_NOTIFY;
    ce.event = win;
    ce.window = win;
    ce.x = geometry.x;
    ce.y = geometry.y;
    ce.width = geometry.width;
    ce.height = geometry.height;
    ce.border_width = border;
    ce.above_sibling = XCB_NONE;
    ce.override_redirect = false;
    xcb_send_event(globalconf.connection, false, win, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *) &ce);
}

/** Grab or ungrab buttons on a window
 * \param win The window
 * \param phys_screen Physical screen number
 */
void
window_grabbuttons(xcb_window_t win, int phys_screen)
{
    Button *b;

    xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                    XCB_BUTTON_INDEX_1, XCB_NO_SYMBOL);
    xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                    XCB_BUTTON_INDEX_1, XCB_NO_SYMBOL | XCB_MOD_MASK_LOCK);
    xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                    XCB_BUTTON_INDEX_1, XCB_NO_SYMBOL | globalconf.numlockmask);
    xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                    XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                    XCB_BUTTON_INDEX_1, XCB_NO_SYMBOL | globalconf.numlockmask | XCB_MOD_MASK_LOCK);

    for(b = globalconf.buttons.client; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY,
                      root_window(globalconf.connection, phys_screen), ANY_MODIFIER);
}

/** Grab buttons on root window
 * \param phys_screen physical screen id
 */
void
window_root_grabbuttons(int phys_screen)
{
    Button *b;

    for(b = globalconf.buttons.root; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false,
                        root_window(globalconf.connection, phys_screen), BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false,
                        root_window(globalconf.connection, phys_screen), BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false,
                        root_window(globalconf.connection, phys_screen), BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false,
                        root_window(globalconf.connection, phys_screen), BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }
}

/** Grab keys on root window
 * \param phys_screen physical screen id
 */
void
window_root_grabkeys(int phys_screen)
{
    Key *k;
    xcb_keycode_t kc;
    xcb_key_symbols_t *keysyms;

    xcb_ungrab_key(globalconf.connection, ANY_KEY,
                   root_window(globalconf.connection, phys_screen), ANY_MODIFIER);

    keysyms = xcb_key_symbols_alloc(globalconf.connection);

    for(k = globalconf.keys; k; k = k->next)
	if((kc = k->keycode) || (k->keysym && (kc = xcb_key_symbols_get_keycode(keysyms, k->keysym))))
        {
            xcb_grab_key(globalconf.connection, true, root_window(globalconf.connection, phys_screen),
                         k->mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, root_window(globalconf.connection, phys_screen),
                         k->mod | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, root_window(globalconf.connection, phys_screen),
                         k->mod | globalconf.numlockmask, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, root_window(globalconf.connection, phys_screen),
                         k->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC,
                         XCB_GRAB_MODE_ASYNC);
        }

    xcb_key_symbols_free(keysyms);
}

void
window_setshape(xcb_window_t win, int phys_screen)
{
    xcb_shape_query_extents_reply_t *r;

    /* Logic to decide if we have a shaped window cribbed from fvwm-2.5.10. */
    if((r = xcb_shape_query_extents_reply(globalconf.connection,
                                          xcb_shape_query_extents_unchecked(globalconf.connection, win),
                                          NULL)) && r->bounding_shaped)
    {
        xcb_shape_combine(globalconf.connection, XCB_SHAPE_SO_SET,
                          XCB_SHAPE_SK_BOUNDING, XCB_SHAPE_SK_BOUNDING,
                          root_window(globalconf.connection, phys_screen),
                          0, 0, win);

        p_delete(&r);
    }
}

void
window_settrans(xcb_window_t win, double opacity)
{
    unsigned int real_opacity = 0xffffffff;

    if(opacity >= 0 && opacity <= 1)
    {
        real_opacity = opacity * 0xffffffff;
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                            x_intern_atom(globalconf.connection, "_NET_WM_WINDOW_OPACITY"),
                            CARDINAL, 32, 1L, &real_opacity);
    }
    else
        xcb_delete_property(globalconf.connection, win,
                            x_intern_atom(globalconf.connection, "_NET_WM_WINDOW_OPACITY"));
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
