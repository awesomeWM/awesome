/*
 * structs.h - basic structs header
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

#ifndef AWESOME_STRUCTS_H
#define AWESOME_STRUCTS_H

#include <xcb/xcb_icccm.h>
#include <xcb/xcb_property.h>

#include "config.h"
#include "luaa.h"
#include "swindow.h"
#include "keybinding.h"
#include "common/xutil.h"
#include "common/xembed.h"
#include "common/refcount.h"

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
    WINDOW_TYPE_UTILITY
} window_type_t;

/** Wibox types */
typedef enum
{
    WIBOX_TYPE_NORMAL = 0,
    WIBOX_TYPE_TITLEBAR
} wibox_type_t;

typedef struct button_t button_t;
typedef struct widget_t widget_t;
typedef struct widget_node_t widget_node_t;
typedef struct client_t client_t;
typedef struct client_node client_node_t;
typedef struct tag tag_t;
typedef struct tag_client_node_t tag_client_node_t;
typedef widget_t *(widget_constructor_t)(widget_t *);
typedef void (widget_destructor_t)(widget_t *);
typedef struct awesome_t awesome_t;

ARRAY_TYPE(widget_node_t, widget_node)
ARRAY_TYPE(button_t *, button)

/** Wibox type */
typedef struct
{
    /** Ref count */
    int refcount;
    /** Ontop */
    bool ontop;
    /** Visible */
    bool isvisible;
    /** Position */
    position_t position;
    /** Wibox type */
    wibox_type_t type;
    /** Window */
    simple_window_t sw;
    /** Alignment */
    alignment_t align;
    /** Screen */
    int screen;
    /** Widget list */
    widget_node_array_t widgets;
    luaA_ref widgets_table;
    /** Widget the mouse is over */
    widget_t *mouse_over;
    /** Need update */
    bool need_update;
    /** Cursor */
    char *cursor;
} wibox_t;
ARRAY_TYPE(wibox_t *, wibox)

/** Widget */
struct widget_t
{
    /** Ref count */
    int refcount;
    /** Widget type is constructor */
    widget_constructor_t *type;
    /** Widget destructor */
    widget_destructor_t *destructor;
    /** Geometry function */
    area_t (*geometry)(widget_t *, int, int, int);
    /** Draw function */
    void (*draw)(widget_t *, draw_context_t *, area_t, int, wibox_t *);
    /** Index function */
    int (*index)(lua_State *, awesome_token_t);
    /** Newindex function */
    int (*newindex)(lua_State *, awesome_token_t);
    /** Mouse over event handler */
    luaA_ref mouse_enter, mouse_leave;
    /** Alignement */
    alignment_t align;
    /** Supported alignment */
    alignment_t align_supported;
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_array_t buttons;
    /** True if the widget is visible */
    bool isvisible;
};

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
    /** Ref counter */
    int refcount;
    /** Valid, or not ? */
    bool invalid;
    /** Client name */
    char *name, *icon_name;
    /** Window geometry */
    area_t geometry;
    struct
    {
        /** Client geometry when (un)fullscreen */
        area_t fullscreen;
        /** Client geometry when (un)-max */
        area_t max;
    } geometries;
    /* Size hints */
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    /** Strut */
    strut_t strut;
    /** Respect resize hints */
    bool honorsizehints;
    int border, oldborder;
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
    /** Client logical screen */
    int screen;
    /** Client physical screen */
    int phys_screen;
    /** Path to an icon */
    char *icon_path;
    /** Titlebar */
    wibox_t *titlebar;
    /** Button bindings */
    button_array_t buttons;
    /** Icon */
    image_t *icon;
    /** Size hints */
    xcb_size_hints_t size_hints;
    /** Window it is transient for */
    client_t *transient_for;
    /** Next and previous clients */
    client_t *prev, *next;
};
ARRAY_TYPE(client_t *, client)

/** Tag type */
struct tag
{
    /** Ref count */
    int refcount;
    /** Tag name */
    char *name;
    /** Screen */
    int screen;
    /** true if selected */
    bool selected;
    /** clients in this tag */
    client_array_t clients;
};
ARRAY_TYPE(tag_t *, tag)

/** Padding type */
typedef struct
{
    /** Padding at top */
    int top;
    /** Padding at bottom */
    int bottom;
    /** Padding at left */
    int left;
    /** Padding at right */
    int right;
} padding_t;

typedef struct
{
    /** Screen index */
    int index;
    /** Screen geometry */
    area_t geometry;
    /** true if we need to arrange() */
    bool need_arrange;
    /** Tag list */
    tag_array_t tags;
    /** Wiboxes */
    wibox_array_t wiboxes;
    /** Padding */
    padding_t padding;
    /** Window that contains the systray */
    struct
    {
        xcb_window_t window;
        /** Systray window parent */
        xcb_window_t parent;
    } systray;
    /** Focused client */
    client_t *client_focus;
} screen_t;

/** Main configuration structure */
struct awesome_t
{
    /** Connection ref */
    xcb_connection_t *connection;
    /** Event and error handlers */
    xcb_event_handlers_t evenths;
    /** Property change handler */
    xcb_property_handlers_t prophs;
    /** Default screen number */
    int default_screen;
    /** Keys symbol table */
    xcb_key_symbols_t *keysyms;
    /** Logical screens */
    screen_t *screens;
    /** Number of screens */
    int nscreen;
    /** True if xinerama is active */
    bool xinerama_is_active;
    /** Key bindings */
    struct
    {
        keybinding_array_t by_code;
        keybinding_array_t by_sym;
    } keys;
    /** Mouse bindings list */
    button_array_t buttons;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Numlock mask */
    unsigned int shiftlockmask;
    /** Numlock mask */
    unsigned int capslockmask;
    /** Check for XRandR extension */
    bool have_randr;
    /** Clients list */
    client_t *clients;
    /** Embedded windows */
    xembed_window_array_t embedded;
    /** Path to config file */
    char *conffile;
    /** Stack client history */
    client_node_t *stack;
    /** Command line passed to awesome */
    char *argv;
    /** Last XMotionEvent coords */
    int pointer_x, pointer_y;
    /** Lua VM state */
    lua_State *L;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Default font */
    font_t *font;
    struct
    {
        /** Command to execute when spawning a new client */
        luaA_ref manage;
        /** Command to execute when unmanaging client */
        luaA_ref unmanage;
        /** Command to execute when giving focus to a client */
        luaA_ref focus;
        /** Command to execute when removing focus to a client */
        luaA_ref unfocus;
        /** Command to run when mouse enter a client */
        luaA_ref mouse_enter;
        /** Command to run when mouse leave a client */
        luaA_ref mouse_leave;
        /** Command to run on arrange */
        luaA_ref arrange;
        /** Command to run when client list changes */
        luaA_ref clients;
        /** Command to run on numbers of tag changes */
        luaA_ref tags;
        /** Command to run when client gets (un)tagged */
        luaA_ref tagged;
        /** Command to run on property change */
        luaA_ref property;
        /** Command to run on time */
        luaA_ref timer;
#ifdef WITH_DBUS
        /** Command to run on dbus events */
        luaA_ref dbus;
#endif
    } hooks;
    /** The event loop */
    struct ev_loop *loop;
    /** The timeout after which we need to stop select() */
    struct ev_timer timer;
    /** The key grabber function */
    luaA_ref keygrabber;
    /** The mouse pointer grabber function */
    luaA_ref mousegrabber;
    /** Focused screen */
    screen_t *screen_focus;
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
