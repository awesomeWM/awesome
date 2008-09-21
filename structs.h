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

#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_property.h>
#include <ev.h>

#include "luaa.h"
#include "layout.h"
#include "swindow.h"
#include "image.h"
#include "common/xutil.h"
#include "common/xembed.h"
#include "common/refcount.h"

/** Stacking layout layers */
typedef enum
{
    LAYER_DESKTOP = 1,
    LAYER_BELOW,
    LAYER_TILE,
    LAYER_FLOAT,
    LAYER_ABOVE,
    LAYER_FULLSCREEN,
    LAYER_MODAL,
    LAYER_ONTOP,
    LAYER_OUTOFSPACE
} layer_t;

/** Windows type */
typedef enum
{
    WINDOW_TYPE_NORMAL = 0,
    WINDOW_TYPE_DESKTOP,
    WINDOW_TYPE_DOCK,
    WINDOW_TYPE_SPLASH,
    WINDOW_TYPE_DIALOG,
} window_type_t;

/** Cursors */
enum
{
    CurNormal, CurResize, CurResizeH, CurResizeV, CurMove,
    CurTopLeft, CurTopRight, CurBotLeft, CurBotRight, CurLast
};

typedef struct button_t button_t;
typedef struct widget_t widget_t;
typedef struct widget_node_t widget_node_t;
typedef struct client_t client_t;
typedef struct client_node_t client_node_t;
typedef struct _tag_t tag_t;
typedef struct tag_client_node_t tag_client_node_t;
typedef widget_t *(widget_constructor_t)(alignment_t);
typedef void (widget_destructor_t)(widget_t *);
typedef struct awesome_t awesome_t;

/** Wibox type */
typedef struct
{
    /** Ref count */
    int refcount;
    /** Window */
    simple_window_t sw;
    /** Wibox name */
    char *name;
    /** Box width and height */
    uint16_t width, height;
    /** Box position */
    position_t position;
    /** Alignment */
    alignment_t align;
    /** Screen */
    int screen;
    /** Widget list */
    widget_node_t *widgets;
    /** Widget the mouse is over */
    widget_node_t *mouse_over;
    /** Need update */
    bool need_update;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    struct
    {
        xcolor_t color;
        uint16_t width;
    } border;
} wibox_t;
ARRAY_TYPE(wibox_t *, wibox)

/** Mouse buttons bindings */
struct button_t
{
    /** Ref count */
    int refcount;
    /** Key modifiers */
    unsigned long mod;
    /** Mouse button number */
    unsigned int button;
    /** Lua function to execute on press. */
    luaA_ref press;
    /** Lua function to execute on release. */
    luaA_ref release;
};

DO_RCNT(button_t, button, p_delete)
DO_ARRAY(button_t *, button, button_unref)

/** Widget */
struct widget_t
{
    /** Ref count */
    int refcount;
    /** widget_t name */
    char *name;
    /** Widget type is constructor */
    widget_constructor_t *type;
    /** Widget destructor */
    widget_destructor_t *destructor;
    /** Widget detach function */
    void (*detach)(widget_t *, wibox_t *);
    /** Draw function */
    int (*draw)(draw_context_t *, int, widget_node_t *, int, int, wibox_t *, awesome_type_t);
    /** Index function */
    int (*index)(lua_State *, awesome_token_t);
    /** Newindex function */
    int (*newindex)(lua_State *, awesome_token_t);
    /** Button event handler */
    void (*button)(widget_node_t *, xcb_button_press_event_t *, int, wibox_t *, awesome_type_t);
    /** Mouse over event handler */
    luaA_ref mouse_enter, mouse_leave;
    /** Alignement */
    alignment_t align;
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_array_t buttons;
    /** Cache flags */
    int cache_flags;
    /** True if the widget is visible */
    bool isvisible;
};

/** Delete a widget structure.
 * \param widget The widget to destroy.
 */
static inline void
widget_delete(widget_t **widget)
{
    if((*widget)->destructor)
        (*widget)->destructor(*widget);
    button_array_wipe(&(*widget)->buttons);
    p_delete(&(*widget)->name);
    p_delete(widget);
}

DO_RCNT(widget_t, widget, widget_delete)

struct widget_node_t
{
    /** The widget */
    widget_t *widget;
    /** The area where the widget was drawn */
    area_t area;
    /** Next and previous widget in the list */
    widget_node_t *prev, *next;
};

/** Delete a widget node structure.
 * \param node The node to destroy.
 */
static inline void
widget_node_delete(widget_node_t **node)
{
    widget_unref(&(*node)->widget);
    p_delete(node);
}

DO_SLIST(widget_node_t, widget_node, widget_node_delete)

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
    char *name;
    /** Window geometry */
    area_t geometry;
    /** Floating window geometry */
    area_t f_geometry;
    /** Max window geometry */
    area_t m_geometry;
    /* Size hints */
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    bool hassizehints;
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
    /** true if the window is floating */
    bool isfloating;
    /** true if the client is moving */
    bool ismoving;
    /** True if the client is hidden */
    bool ishidden;
    /** True if the client is minimized */
    bool isminimized;
    /** True if the client is fullscreen */
    bool isfullscreen;
    /** True if the client is above others */
    bool isabove;
    /** True if the client is below others */
    bool isbelow;
    /** True if the client is modal */
    bool ismodal;
    /** True if the client is on top */
    bool isontop;
    /** true if the client must be skipped from task bar client list */
    bool skiptb;
    /** True if the client cannot have focus */
    bool nofocus;
    /** The window type */
    window_type_t type;
    /** Window of the client */
    xcb_window_t win;
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

static void
client_delete(client_t **c)
{
    button_array_wipe(&(*c)->buttons);
    p_delete(&(*c)->icon_path);
    p_delete(&(*c)->name);
    p_delete(c);
}

DO_ARRAY(client_t *, client, DO_NOTHING)
DO_RCNT(client_t, client, client_delete)

struct client_node_t
{
    /** The client */
    client_t *client;
    /** Next and previous client_nodes */
    client_node_t *prev, *next;
};

/** Tag type */
struct _tag_t
{
    /** Ref count */
    int refcount;
    /** Tag name */
    char *name;
    /** Screen */
    int screen;
    /** true if selected */
    bool selected;
    /** Current tag layout */
    layout_t *layout;
    /** Master width factor */
    double mwfact;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
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
    /** Statusbars */
    wibox_array_t statusbars;
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
    /** Cursors */
    xcb_cursor_t cursor[CurLast];
    /** Clients list */
    client_t *clients;
    /** Embedded windows */
    xembed_window_t *embedded;
    /** Path to config file */
    char *configpath;
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
        /** Command to run when mouse is over */
        luaA_ref mouse_over;
        /** Command to run on arrange */
        luaA_ref arrange;
        /** Command to run on title change */
        luaA_ref titleupdate;
        /** Command to run on urgent flag */
        luaA_ref urgent;
        /** Command to run on time */
        luaA_ref timer;
    } hooks;
    /** The event loop */
    struct ev_loop *loop;
    /** The timeout after which we need to stop select() */
    struct ev_timer timer;
    /** The key grabber function */
    luaA_ref keygrabber;
    /** Focused screen */
    screen_t *screen_focus;
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
