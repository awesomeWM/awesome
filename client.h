/*
 * client.h - client management header
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

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include "mouse.h"
#include "stack.h"
#include "common/luaclass.h"

#define CLIENT_SELECT_INPUT_EVENT_MASK (XCB_EVENT_MASK_STRUCTURE_NOTIFY \
                                        | XCB_EVENT_MASK_PROPERTY_CHANGE \
                                        | XCB_EVENT_MASK_ENTER_WINDOW \
                                        | XCB_EVENT_MASK_LEAVE_WINDOW \
                                        | XCB_EVENT_MASK_FOCUS_CHANGE)

/** Windows type */
typedef enum
{
    WINDOW_TYPE_NORMAL = 0,
    WINDOW_TYPE_DESKTOP,
    WINDOW_TYPE_DOCK,
    WINDOW_TYPE_SPLASH,
    WINDOW_TYPE_DIALOG,
    /* The ones below may have TRANSIENT_FOR, but are not plain dialogs.
     * They were purposefully placed below DIALOG.
     */
    WINDOW_TYPE_MENU,
    WINDOW_TYPE_TOOLBAR,
    WINDOW_TYPE_UTILITY,
    /* This ones are usually set on override-redirect windows. */
    WINDOW_TYPE_DROPDOWN_MENU,
    WINDOW_TYPE_POPUP_MENU,
    WINDOW_TYPE_TOOLTIP,
    WINDOW_TYPE_NOTIFICATION,
    WINDOW_TYPE_COMBO,
    WINDOW_TYPE_DND
} window_type_t;

/* Strut */
typedef struct
{
    uint16_t left, right, top, bottom;
    uint16_t left_start_y, left_end_y;
    uint16_t right_start_y, right_end_y;
    uint16_t top_start_x, top_end_x;
    uint16_t bottom_start_x, bottom_end_x;
} strut_t;

/** client_t type */
struct client_t
{
    LUA_OBJECT_HEADER
    /** Valid, or not ? */
    bool invalid;
    /** Client name */
    char *name, *icon_name;
    /** WM_CLASS stuff */
    char *class, *instance;
    /** Startup ID */
    char *startup_id;
    /** Window geometry */
    area_t geometry;
    struct
    {
        /** Client geometry when (un)fullscreen */
        area_t fullscreen;
        /** Client geometry when (un)-max */
        area_t max;
        /** Internal geometry (matching X11 protocol) */
        area_t internal;
    } geometries;
    /** Strut */
    strut_t strut;
    /** Border width and pre-fullscreen border width */
    int border, border_fs;
    xcolor_t border_color;
    /** True if the client is sticky */
    bool issticky;
    /** Has urgency hint */
    bool isurgent;
    /** True if the client is hidden */
    bool ishidden;
    /** True if the client is minimized */
    bool isminimized;
    /** True if the client is fullscreen */
    bool isfullscreen;
    /** True if the client is maximized horizontally */
    bool ismaxhoriz;
    /** True if the client is maximized vertically */
    bool ismaxvert;
    /** True if the client is above others */
    bool isabove;
    /** True if the client is below others */
    bool isbelow;
    /** True if the client is modal */
    bool ismodal;
    /** True if the client is on top */
    bool isontop;
    /** True if a client is banned to a position outside the viewport.
     * Note that the geometry remains unchanged and that the window is still mapped.
     */
    bool isbanned;
    /** true if the client must be skipped from task bar client list */
    bool skiptb;
    /** True if the client cannot have focus */
    bool nofocus;
    /** The window type */
    window_type_t type;
    /** Window of the client */
    xcb_window_t win;
    /** Window of the group leader */
    xcb_window_t group_win;
    /** Window holding command needed to start it (session management related) */
    xcb_window_t leader_win;
    /** Client's WM_PROTOCOLS property */
    xcb_get_wm_protocols_reply_t protocols;
    /** Client logical screen */
    screen_t *screen;
    /** Client physical screen */
    int phys_screen;
    /** Titlebar */
    wibox_t *titlebar;
    /** Button bindings */
    button_array_t buttons;
    /** Key bindings */
    key_array_t keys;
    /** Icon */
    image_t *icon;
    /** Size hints */
    xcb_size_hints_t size_hints;
    bool size_hints_honor;
    /** Machine the client is running on. */
    char *machine;
    /** Role of the client */
    char *role;
    /** Client pid */
    uint32_t pid;
    /** Window it is transient for */
    client_t *transient_for;
    /** Window opacity */
    double opacity;
};

client_t * luaA_client_checkudata(lua_State *, int);

ARRAY_FUNCS(client_t *, client, DO_NOTHING)

/** Client class */
lua_class_t client_class;

LUA_OBJECT_FUNCS(client_class, client_t, client, "client")

#define client_need_reban(c) \
    do { \
        if(!c->screen->need_reban \
           && client_isvisible(c, (c)->screen)) \
            c->screen->need_reban = true; \
    } while(0)

bool client_maybevisible(client_t *, screen_t *);
client_t * client_getbywin(xcb_window_t);
void client_ban(client_t *);
void client_unban(client_t *);
void client_manage(xcb_window_t, xcb_get_geometry_reply_t *, int, bool);
area_t client_geometry_hints(client_t *, area_t);
bool client_resize(client_t *, area_t, bool);
void client_unmanage(client_t *);
void client_kill(client_t *);
void client_setsticky(client_t *, bool);
void client_setabove(client_t *, bool);
void client_setbelow(client_t *, bool);
void client_setmodal(client_t *, bool);
void client_setontop(client_t *, bool);
void client_setfullscreen(client_t *, bool);
void client_setmaxhoriz(client_t *, bool);
void client_setmaxvert(client_t *, bool);
void client_setminimized(client_t *, bool);
void client_setborder(client_t *, int);
void client_seturgent(client_t *, bool);
void client_focus(client_t *);
void client_focus_update(client_t *);
void client_unfocus(client_t *);
void client_unfocus_update(client_t *);
void client_stack_refresh(void);
bool client_hasproto(client_t *, xcb_atom_t);
void client_setfocus(client_t *, bool);
void client_ignore_enterleave_events(void);
void client_restore_enterleave_events(void);

static inline void
client_stack(void)
{
    globalconf.client_need_stack_refresh = true;
}

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
    client_stack();
}

/** Put client on the end of the stack.
 * \param c The client to lower.
 */
static inline void
client_lower(client_t *c)
{
    stack_client_push(c);

    /* Traverse all transient layers. */
    for(client_t *tc = c->transient_for; tc; tc = tc->transient_for)
        stack_client_push(tc);

    client_stack();
}

/** Check if a client has fixed size.
 * \param c A client.
 * \return A boolean value, true if the client has a fixed size.
 */
static inline bool
client_isfixed(client_t *c)
{
    return (c->size_hints.flags & XCB_SIZE_HINT_P_MAX_SIZE
            && c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE
            && c->size_hints.max_width == c->size_hints.min_width
            && c->size_hints.max_height == c->size_hints.min_height
            && c->size_hints.max_width
            && c->size_hints.max_height);
}

/** Returns true if a client is tagged with one of the tags of the 
 * specified screen and is not hidden. Note that "banned" clients are included.
 * \param c The client to check.
 * \param screen Virtual screen number.
 * \return true if the client is visible, false otherwise.
 */
static inline bool
client_isvisible(client_t *c, screen_t *screen)
{
    return (!c->ishidden && !c->isminimized && client_maybevisible(c, screen));
}

/** Check if a client has strut information.
 * \param c A client.
 * \return A boolean value, true if the client has strut information.
 */
static inline bool
client_hasstrut(client_t *c)
{
    return (c->strut.left
            || c->strut.right
            || c->strut.top
            || c->strut.bottom
            || c->strut.left_start_y
            || c->strut.left_end_y
            || c->strut.right_start_y
            || c->strut.right_end_y
            || c->strut.top_start_x
            || c->strut.top_end_x
            || c->strut.bottom_start_x
            || c->strut.bottom_end_x);
}

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
