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
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_aux.h>

#include "structs.h"
#include "window.h"

extern awesome_t globalconf;

/** Mask shorthands */
#define BUTTONMASK     (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE)

/** Set client state (WM_STATE) property.
 * \param win The window to set state.
 * \param state The state to set.
 */
void
window_setstate(xcb_window_t win, long state)
{
    long data[] = { state, XCB_NONE };
    const xcb_atom_t wm_state_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms,
                                                             xutil_intern_atom(globalconf.connection,
                                                                               &globalconf.atoms,
                                                                               "WM_STATE"));

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                        wm_state_atom, wm_state_atom, 32, 2, data);
}

/** Get a window state (WM_STATE).
 * \param w A client window.
 * \return The current state of the window, or -1 on error.
 */
long
window_getstate(xcb_window_t w)
{
    long result = -1;
    unsigned char *p = NULL;
    xcb_get_property_cookie_t prop_c;
    xcb_atom_t wm_state_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms,
                                                       xutil_intern_atom(globalconf.connection,
                                                                         &globalconf.atoms,
                                                                         "WM_STATE"));
    xcb_get_property_reply_t *prop_r;

    prop_c = xcb_get_property_unchecked(globalconf.connection, false, w,
                                        wm_state_atom, wm_state_atom,
                                        0L, 2L);

    if(!(prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)))
        return -1;

    p = xcb_get_property_value(prop_r);
    if(xcb_get_property_value_length(prop_r) != 0)
        result = *p;

    p_delete(&prop_r);

    return result;
}

/** Configure a window with its new geometry and border size.
 * \param win The X window id to configure.
 * \param geometry The new window geometry.
 * \param border The new border size.
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

/** Grab or ungrab buttons on a window.
 * \param win The window.
 * \param root The root window.
 * \param buttons The buttons to grab.
 */
void
window_grabbuttons(xcb_window_t win, xcb_window_t root, button_t *buttons)
{
    button_t *b;

    for(b = buttons; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, win, BUTTONMASK,
                        XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }

    xcb_ungrab_button(globalconf.connection, XCB_BUTTON_INDEX_ANY, root, ANY_MODIFIER);
}

/** Grab all buttons on the root window.
 * \param root The root window.
 */
void
window_root_grabbuttons(xcb_window_t root)
{
    button_t *b;

    for(b = globalconf.buttons.root; b; b = b->next)
    {
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | XCB_MOD_MASK_LOCK);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask);
        xcb_grab_button(globalconf.connection, false, root, BUTTONMASK,
                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE,
                        b->button, b->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
    }
}

/** Grab key on the root windows.
 * \param k The keybinding.
 */
void
window_root_grabkey(keybinding_t *k)
{
    int phys_screen = globalconf.default_screen;
    xcb_screen_t *s;
    xcb_keycode_t kc;

    if((kc = k->keycode)
       || (k->keysym && (kc = xcb_key_symbols_get_keycode(globalconf.keysyms, k->keysym))))
        do
        {
            s = xcb_aux_get_screen(globalconf.connection, phys_screen);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | globalconf.numlockmask, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC,
                         XCB_GRAB_MODE_ASYNC);
        phys_screen++;
        } while(!globalconf.screens_info->xinerama_is_active
                && phys_screen < globalconf.screens_info->nscreen);
}

/** Ungrab key on the root windows.
 * \param k The keybinding.
 */
void
window_root_ungrabkey(keybinding_t *k)
{
    int phys_screen = globalconf.default_screen;
    xcb_screen_t *s;
    xcb_keycode_t kc;

    if((kc = k->keycode)
       || (k->keysym && (kc = xcb_key_symbols_get_keycode(globalconf.keysyms, k->keysym))))
        do
        {
            s = xcb_aux_get_screen(globalconf.connection, phys_screen);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | XCB_MOD_MASK_LOCK);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | globalconf.numlockmask);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
        phys_screen++;
        } while(!globalconf.screens_info->xinerama_is_active
                && phys_screen < globalconf.screens_info->nscreen);
}

/** Set transparency of a window.
 * \param win The window.
 * \param opacity Opacity of the window, between 0 and 1.
 */
void
window_settrans(xcb_window_t win, double opacity)
{
    unsigned int real_opacity = 0xffffffff;
    xcb_atom_t wopacity_atom;
    xutil_intern_atom_request_t wopacity_atom_q;

    wopacity_atom_q = xutil_intern_atom(globalconf.connection, &globalconf.atoms, "_NET_WM_WINDOW_OPACITY");

    if(opacity >= 0 && opacity <= 1)
    {
        real_opacity = opacity * 0xffffffff;
        wopacity_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, wopacity_atom_q);
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, win,
                            wopacity_atom, CARDINAL, 32, 1L, &real_opacity);
    }
    else
    {
        wopacity_atom = xutil_intern_atom_reply(globalconf.connection, &globalconf.atoms, wopacity_atom_q);
        xcb_delete_property(globalconf.connection, win,
                            wopacity_atom);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
