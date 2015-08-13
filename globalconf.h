/*
 * globalconf.h - basic globalconf.header
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

#ifndef AWESOME_GLOBALCONF_H
#define AWESOME_GLOBALCONF_H

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include <glib.h>

#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_cursor.h>
#include <X11/Xresource.h>

#include "objects/key.h"
#include "common/xembed.h"
#include "common/buffer.h"

#define ROOT_WINDOW_EVENT_MASK \
    (const uint32_t []) { \
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY \
      | XCB_EVENT_MASK_ENTER_WINDOW \
      | XCB_EVENT_MASK_LEAVE_WINDOW \
      | XCB_EVENT_MASK_STRUCTURE_NOTIFY \
      | XCB_EVENT_MASK_BUTTON_PRESS \
      | XCB_EVENT_MASK_BUTTON_RELEASE \
      | XCB_EVENT_MASK_FOCUS_CHANGE \
      | XCB_EVENT_MASK_PROPERTY_CHANGE \
    }

typedef struct drawable_t drawable_t;
typedef struct drawin_t drawin_t;
typedef struct a_screen screen_t;
typedef struct button_t button_t;
typedef struct client_t client_t;
typedef struct tag tag_t;
typedef struct xproperty xproperty_t;
typedef struct window_t window_t;

ARRAY_TYPE(button_t *, button)
ARRAY_TYPE(tag_t *, tag)
ARRAY_TYPE(screen_t *, screen)
ARRAY_TYPE(client_t *, client)
ARRAY_TYPE(drawin_t *, drawin)
ARRAY_TYPE(drawable_t *, drawable)
ARRAY_TYPE(window_t *, window)
ARRAY_TYPE(xproperty_t, xproperty)

/** Main configuration structure */
typedef struct
{
    /** Xlib Display */
    Display *display;
    /** X Resources DB */
    XrmDatabase xrmdb;
    /** Connection ref */
    xcb_connection_t *connection;
    /** Default screen number */
    int default_screen;
    /** xcb-cursor context */
    xcb_cursor_context_t *cursor_ctx;
    /** Keys symbol table */
    xcb_key_symbols_t *keysyms;
    /** Logical screens */
    screen_array_t screens;
    /** Root window key bindings */
    key_array_t keys;
    /** Root window mouse bindings */
    button_array_t buttons;
    /** Modifiers masks */
    uint16_t numlockmask, shiftlockmask, capslockmask, modeswitchmask;
    /** Check for XTest extension */
    bool have_xtest;
    /** Check for SHAPE extension */
    bool have_shape;
    /** Clients list */
    client_array_t clients;
    /** Embedded windows */
    xembed_window_array_t embedded;
    /** Stack client history */
    client_array_t stack;
    /** Lua VM state (opaque to avoid mis-use, see globalconf_get_lua_State()) */
    struct {
        lua_State *real_L_dont_use_directly;
    } L;
    /** All errors messages from loading config files */
    buffer_t startup_errors;
    /** main loop that awesome is running on */
    GMainLoop *loop;
    /** The key grabber function */
    int keygrabber;
    /** The mouse pointer grabber function */
    int mousegrabber;
    /** Input focus information */
    struct
    {
        /** Focused client */
        client_t *client;
        /** Is there a focus change pending? */
        bool need_update;
    } focus;
    /** Drawins */
    drawin_array_t drawins;
    /** The startup notification display struct */
    SnDisplay *sndisplay;
    /** Latest timestamp we got from the X server */
    xcb_timestamp_t timestamp;
    /** Window that contains the systray */
    struct
    {
        xcb_window_t window;
        /** Atom for _NET_SYSTEM_TRAY_%d */
        xcb_atom_t atom;
        /** Do we own the systray selection? */
        bool registered;
        /** Systray window parent */
        drawin_t *parent;
    } systray;
    /** The monitor of startup notifications */
    SnMonitorContext *snmonitor;
    /** The visual, used to draw */
    xcb_visualtype_t *visual;
    /** The screen's default visual */
    xcb_visualtype_t *default_visual;
    /** The screen's information */
    xcb_screen_t *screen;
    /** A graphic context. */
    xcb_gcontext_t gc;
    /** Our default depth */
    uint8_t default_depth;
    /** Our default color map */
    xcb_colormap_t default_cmap;
    /** Do we have to reban clients? */
    bool need_lazy_banning;
    /** List of windows which need properties to be committed. */
    window_array_t need_properties_refresh_windows;
    /** List of clients which need properties to be committed. */
    client_array_t need_properties_refresh_clients;
    /** List of drawables which need their geometry to be committed. */
    drawable_array_t need_geometry_refresh_drawable;
    /** Tag list */
    tag_array_t tags;
    /** List of registered xproperties */
    xproperty_array_t xproperties;
    /* xkb context */
    struct xkb_context *xkb_ctx;
    /* xkb state of dead keys on keyboard */
    struct xkb_state *xkb_state;
} awesome_t;

extern awesome_t globalconf;

/** You should always use this as lua_State *L = globalconf_get_lua_State().
 * That way it becomes harder to introduce coroutine-related problems.
 */
static inline lua_State *globalconf_get_lua_State(void) {
    return globalconf.L.real_L_dont_use_directly;
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
