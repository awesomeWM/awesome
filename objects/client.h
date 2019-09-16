/*
 * client.h - client management header
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

#ifndef AWESOME_OBJECTS_CLIENT_H
#define AWESOME_OBJECTS_CLIENT_H

#include "stack.h"
#include "objects/window.h"

#define CLIENT_SELECT_INPUT_EVENT_MASK (XCB_EVENT_MASK_STRUCTURE_NOTIFY \
                                        | XCB_EVENT_MASK_PROPERTY_CHANGE \
                                        | XCB_EVENT_MASK_FOCUS_CHANGE)

#define FRAME_SELECT_INPUT_EVENT_MASK (XCB_EVENT_MASK_STRUCTURE_NOTIFY \
                                       | XCB_EVENT_MASK_ENTER_WINDOW \
                                       | XCB_EVENT_MASK_LEAVE_WINDOW \
                                       | XCB_EVENT_MASK_EXPOSURE \
                                       | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT \
                                       | XCB_EVENT_MASK_POINTER_MOTION \
                                       | XCB_EVENT_MASK_BUTTON_PRESS \
                                       | XCB_EVENT_MASK_BUTTON_RELEASE)

typedef enum {
    CLIENT_TITLEBAR_TOP = 0,
    CLIENT_TITLEBAR_RIGHT = 1,
    CLIENT_TITLEBAR_BOTTOM = 2,
    CLIENT_TITLEBAR_LEFT = 3,
    /* This is not a valid value, but the number of valid values */
    CLIENT_TITLEBAR_COUNT = 4
} client_titlebar_t;

/* Special bit we invented to "fake" unset hints */
#define MWM_HINTS_AWESOME_SET   (1L << 15)

/* The following is taken from MwmUtil.h and slightly adapted, which is
 * copyright (c) 1987-2012, The Open Group.
 * It is licensed under GPLv2 or later.
 */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3

#define MWM_TEAROFF_WINDOW    (1L<<0)

typedef struct {
    uint32_t hints;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
} motif_wm_hints_t;

/** client_t type */
struct client_t
{
    WINDOW_OBJECT_HEADER
    /** Window we use for input focus and no-input clients */
    xcb_window_t nofocus_window;
    /** Client logical screen */
    screen_t *screen;
    /** Client name */
    char *name, *alt_name, *icon_name, *alt_icon_name;
    /** WM_CLASS stuff */
    char *class, *instance;
    /** Window geometry */
    area_t geometry;
    /** Old window geometry currently configured in X11 */
    area_t x11_client_geometry;
    area_t x11_frame_geometry;
    /** Got a configure request and have to call client_send_configure() if its ignored? */
    bool got_configure_request;
    /** Startup ID */
    char *startup_id;
    /** True if the client is sticky */
    bool sticky;
    /** Has urgency hint */
    bool urgent;
    /** True if the client is hidden */
    bool hidden;
    /** True if the client is minimized */
    bool minimized;
    /** True if the client is fullscreen */
    bool fullscreen;
    /** True if the client is maximized horizontally */
    bool maximized_horizontal;
    /** True if the client is maximized vertically */
    bool maximized_vertical;
    /** True if the client is maximized both horizontally and vertically by the
      * the user
      */
    bool maximized;
    /** True if the client is above others */
    bool above;
    /** True if the client is below others */
    bool below;
    /** True if the client is modal */
    bool modal;
    /** True if the client is on top */
    bool ontop;
    /** True if a client is banned to a position outside the viewport.
     * Note that the geometry remains unchanged and that the window is still mapped.
     */
    bool isbanned;
    /** true if the client must be skipped from task bar client list */
    bool skip_taskbar;
    /** True if the client cannot have focus */
    bool nofocus;
    /** True if the client is focusable.  Overrides nofocus, and can be set
     * from Lua. */
    bool focusable;
    bool focusable_set;
    /** True if the client window has a _NET_WM_WINDOW_TYPE proeprty */
    bool has_NET_WM_WINDOW_TYPE;
    /** Window of the group leader */
    xcb_window_t group_window;
    /** Window holding command needed to start it (session management related) */
    xcb_window_t leader_window;
    /** Client's WM_PROTOCOLS property */
    xcb_icccm_get_wm_protocols_reply_t protocols;
    /** Key bindings */
    key_array_t keys;
    /** Icons */
    cairo_surface_array_t icons;
    /** True if we ever got an icon from _NET_WM_ICON */
    bool have_ewmh_icon;
    /** Size hints */
    xcb_size_hints_t size_hints;
    /** The visualtype that c->window uses */
    xcb_visualtype_t *visualtype;
    /** Do we honor the client's size hints? */
    bool size_hints_honor;
    /** Machine the client is running on. */
    char *machine;
    /** Role of the client */
    char *role;
    /** Client pid */
    uint32_t pid;
    /** Window it is transient for */
    client_t *transient_for;
    /** Value of WM_TRANSIENT_FOR */
    xcb_window_t transient_for_window;
    /** Titelbar information */
    struct {
        /** The size of this bar. */
        uint16_t size;
        /** The drawable for this bar. */
        drawable_t *drawable;
    } titlebar[CLIENT_TITLEBAR_COUNT];
    /** Motif WM hints, with an additional MWM_HINTS_AWESOME_SET bit */
    motif_wm_hints_t motif_wm_hints;
};

ARRAY_FUNCS(client_t *, client, DO_NOTHING)

/** Client class */
lua_class_t client_class;

LUA_OBJECT_FUNCS(client_class, client_t, client)

bool client_on_selected_tags(client_t *);
client_t * client_getbywin(xcb_window_t);
client_t * client_getbynofocuswin(xcb_window_t);
client_t * client_getbyframewin(xcb_window_t);

void client_ban(client_t *);
void client_ban_unfocus(client_t *);
void client_unban(client_t *);
void client_manage(xcb_window_t, xcb_get_geometry_reply_t *, xcb_get_window_attributes_reply_t *);
bool client_resize(client_t *, area_t, bool);
void client_unmanage(client_t *, bool);
void client_kill(client_t *);
void client_set_sticky(lua_State *, int, bool);
void client_set_above(lua_State *, int, bool);
void client_set_below(lua_State *, int, bool);
void client_set_modal(lua_State *, int, bool);
void client_set_ontop(lua_State *, int, bool);
void client_set_fullscreen(lua_State *, int, bool);
void client_set_maximized(lua_State *, int, bool);
void client_set_maximized_horizontal(lua_State *, int, bool);
void client_set_maximized_vertical(lua_State *, int, bool);
void client_set_minimized(lua_State *, int, bool);
void client_set_urgent(lua_State *, int, bool);
void client_set_pid(lua_State *, int, uint32_t);
void client_set_role(lua_State *, int, char *);
void client_set_machine(lua_State *, int, char *);
void client_set_icon_name(lua_State *, int, char *);
void client_set_alt_icon_name(lua_State *, int, char *);
void client_set_class_instance(lua_State *, int, const char *, const char *);
void client_set_type(lua_State *L, int, window_type_t);
void client_set_transient_for(lua_State *L, int, client_t *);
void client_set_name(lua_State *L, int, char *);
void client_set_startup_id(lua_State *L, int, char *);
void client_set_alt_name(lua_State *L, int, char *);
void client_set_group_window(lua_State *, int, xcb_window_t);
void client_set_icons(client_t *, cairo_surface_array_t);
void client_set_icon_from_pixmaps(client_t *, xcb_pixmap_t, xcb_pixmap_t);
void client_set_skip_taskbar(lua_State *, int, bool);
void client_set_motif_wm_hints(lua_State *, int, motif_wm_hints_t);
void client_focus(client_t *);
bool client_focus_update(client_t *);
bool client_hasproto(client_t *, xcb_atom_t);
void client_ignore_enterleave_events(void);
void client_restore_enterleave_events(void);
void client_refresh_partial(client_t *, int16_t, int16_t, uint16_t, uint16_t);
void client_class_setup(lua_State *);
void client_send_configure(client_t *);
void client_find_transient_for(client_t *);
void client_reparent_to_root(client_t *);
drawable_t *client_get_drawable(client_t *, int, int);
drawable_t *client_get_drawable_offset(client_t *, int *, int *);

/** Put client on top of the stack.
 * \param c The client to raise.
 */
static inline void
client_raise(client_t *c)
{
    client_t *tc = c;
    int counter = 0;

    /* Find number of transient layers. */
    for(counter = 0; tc->transient_for; counter++)
        tc = tc->transient_for;

    /* Push them in reverse order. */
    for(; counter > 0; counter--)
    {
        tc = c;
        for(int i = 0; i < counter; i++)
            tc = tc->transient_for;
        stack_client_append(tc);
    }

    /* Push c on top of the stack. */
    stack_client_append(c);

    /* Notify the listeners */
    lua_State *L = globalconf_get_lua_State();
    luaA_object_push(L, c);
    luaA_object_emit_signal(L, -1, "raised", 0);
    lua_pop(L, 1);
}

/** Check if a client has fixed size.
 * \param c A client.
 * \return A boolean value, true if the client has a fixed size.
 */
static inline bool
client_isfixed(client_t *c)
{
    return (c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE
            && c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE
            && c->size_hints.max_width == c->size_hints.min_width
            && c->size_hints.max_height == c->size_hints.min_height
            && c->size_hints.max_width
            && c->size_hints.max_height
            && c->size_hints_honor);
}

/** Returns true if a client is tagged with one of the tags of the
 * specified screen and is not hidden. Note that "banned" clients are included.
 * \param c The client to check.
 * \param screen Virtual screen number.
 * \return true if the client is visible, false otherwise.
 */
static inline bool
client_isvisible(client_t *c)
{
    return (!c->hidden && !c->minimized && client_on_selected_tags(c));
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
