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

#include "lua.h"
#include "layout.h"
#include "common/draw.h"
#include "common/swindow.h"
#include "common/xscreen.h"
#include "common/refcount.h"

/** Stacking layout layers */
typedef enum
{
    LAYER_DESKTOP,
    LAYER_BELOW,
    LAYER_TILE,
    LAYER_FLOAT,
    LAYER_ABOVE,
    LAYER_FULLSCREEN
} layer_t;

/** Cursors */
enum
{ CurNormal, CurResize, CurMove, CurLast };

/** Titlebar template structure */
typedef struct
{
    /** Ref count */
    int refcount;
    /** Position */
    position_t position;
    /** Alignment on window */
    alignment_t align;
    /** Width and height */
    int width, height;
    /** Various text used for rendering */
    char *text_normal, *text_focus, *text_urgent;
} titlebar_t;

/** Delete a titlebar structure.
 * \param t The titlebar to destroy.
 */
static inline void
titlebar_delete(titlebar_t **t)
{
    p_delete(&(*t)->text_normal);
    p_delete(&(*t)->text_focus);
    p_delete(&(*t)->text_urgent);
    p_delete(t);
}

DO_RCNT(titlebar_t, titlebar, titlebar_delete)

/** Keys bindings */
typedef struct keybinding_t keybinding_t;
struct keybinding_t
{
    /** Key modifier */
    unsigned long mod;
    /** Keysym */
    xcb_keysym_t keysym;
    /** Keycode */
    xcb_keycode_t keycode;
    /** Lua function to execute. */
    luaA_function fct;
    /** Next and previous keys */
    keybinding_t *prev, *next;
};

DO_SLIST(keybinding_t, keybinding, p_delete)

/** Mouse buttons bindings */
typedef struct button_t button_t;
struct button_t
{
    /** Key modifiers */
    unsigned long mod;
    /** Mouse button number */
    unsigned int button;
    /** Lua function to execute. */
    luaA_function fct;
    /** Next and previous buttons */
    button_t *prev, *next;
};

DO_SLIST(button_t, button, p_delete)

/** Widget tell status code */
typedef enum
{
    WIDGET_NOERROR = 0,
    WIDGET_ERROR,
    WIDGET_ERROR_NOVALUE,
    WIDGET_ERROR_CUSTOM,
    WIDGET_ERROR_FORMAT_FONT,
    WIDGET_ERROR_FORMAT_COLOR,
    WIDGET_ERROR_FORMAT_SECTION
} widget_tell_status_t;

/** Widget */
typedef struct widget_t widget_t;
typedef struct widget_node_t widget_node_t;
typedef struct statusbar_t statusbar_t;
struct widget_t
{
    /** Ref count */
    int refcount;
    /** widget_t name */
    char *name;
    /** Draw function */
    int (*draw)(widget_node_t *, statusbar_t *, int, int);
    /** Update function */
    widget_tell_status_t (*tell)(widget_t *, const char *, const char *);
    /** ButtonPressedEvent handler */
    void (*button_press)(widget_node_t *, statusbar_t *, xcb_button_press_event_t *);
    /** Alignement */
    alignment_t align;
    /** Misc private data */
    void *data;
    /** Button bindings */
    button_t *buttons;
    /** Cache flags */
    int cache_flags;
};

struct widget_node_t
{
    /** The widget */
    widget_t *widget;
    /** The area where the widget was drawn */
    area_t area;
    /** Next and previous widget in the list */
    widget_node_t *prev, *next;
};

/** Delete a widget structure.
 * \param widget The widget to destroy.
 */
static inline void
widget_delete(widget_t **widget)
{
    button_list_wipe(&(*widget)->buttons);
    p_delete(&(*widget)->data);
    p_delete(&(*widget)->name);
    p_delete(widget);
}

DO_RCNT(widget_t, widget, widget_delete)

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

/** Status bar */
struct statusbar_t
{
    /** Ref count */
    int refcount;
    /** Window */
    simple_window_t *sw;
    /** statusbar_t name */
    char *name;
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Bar position */
    position_t position;
    /** Screen */
    int screen;
    /** Physical screen id */
    int phys_screen;
    /** Widget list */
    widget_node_t *widgets;
    /** Draw context */
    draw_context_t *ctx;
    /** Need update */
    bool need_update;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Next and previous statusbars */
    statusbar_t *prev, *next;
};

/** client_t type */
typedef struct client_t client_t;
struct client_t
{
    /** Client name */
    char *name;
    /** Window geometry */
    area_t geometry;
    /** Floating window geometry */
    area_t f_geometry;
    /** Max window geometry */
    area_t m_geometry;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    int border, oldborder;
    /** Has urgency hint */
    bool isurgent;
    /** Store previous floating state before maximizing */
    bool wasfloating;
    /** true if the window is floating */
    bool isfloating;
    /** true if the window is fixed */
    bool isfixed;
    /** true if the client must be skipped from client list */
    bool skip;
    /** true if the client is moving */
    bool ismoving;
    /** true if the client must be skipped from task bar client list */
    bool skiptb;
    /** Next and previous clients */
    client_t *prev, *next;
    /** Window of the client */
    xcb_window_t win;
    /** Client logical screen */
    int screen;
    /** Client physical screen */
    int phys_screen;
    /** True if the client is a new one */
    bool newcomer;
    /** Titlebar */
    titlebar_t titlebar;
    /** Titlebar window */
    simple_window_t *titlebar_sw;
    /** Old position */
    position_t titlebar_oldposition;
    /** Layer in the stacking order */
    layer_t layer, oldlayer;
    /** Path to an icon */
    char *icon_path;
};

typedef struct client_node_t client_node_t;
struct client_node_t
{
    /** The client */
    client_t *client;
    /** Next and previous client_nodes */
    client_node_t *prev, *next;
};

/** Tag type */
typedef struct _tag_t tag_t;
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
    /** Next and previous tags */
    tag_t *prev, *next;
};

/** Tag client link type */
typedef struct tag_client_node_t tag_client_node_t;
struct tag_client_node_t
{
    tag_t *tag;
    client_t *client;
    /** Next and previous tag_client_nodes */
    tag_client_node_t *prev, *next;
};

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
} Padding;

typedef area_t (FloatingPlacement)(client_t *);

typedef struct
{
    /** true if we need to arrange() */
    bool need_arrange;
    /** Tag list */
    tag_t *tags;
    /** Status bar */
    statusbar_t *statusbar;
    /** Padding */
    Padding padding;
} VirtScreen;

/** Main configuration structure */
typedef struct AwesomeConf AwesomeConf;
struct AwesomeConf
{
    /** Connection ref */
    xcb_connection_t *connection;
    /** Event and error handlers */
    xcb_event_handlers_t *evenths;
    /** Default screen number */
    int default_screen;
    /** Keys symbol table */
    xcb_key_symbols_t *keysyms;
    /** Logical screens */
    VirtScreen *screens;
    /** Screens info */
    screens_info_t *screens_info;
    /** Keys bindings list */
    keybinding_t *keys;
    /** Mouse bindings list */
    struct
    {
           button_t *root;
           button_t *client;
           button_t *titlebar;
    } buttons;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Numlock mask */
    unsigned int shiftlockmask;
    /** Numlock mask */
    unsigned int capslockmask;
    /** Check for XShape extension */
    bool have_shape;
    /** Check for XRandR extension */
    bool have_randr;
    /** Cursors */
    xcb_cursor_t cursor[CurLast];
    /** client_ts list */
    client_t *clients;
    /** Path to config file */
    char *configpath;
    /** Floating window placement algo */
    FloatingPlacement *floating_placement;
    /** Selected clients history */
    client_node_t *focus;
    /** Link between tags and clients */
    tag_client_node_t *tclink;
    /** Command line passed to awesome */
    char *argv;
    /** Last XMotionEvent coords */
    int pointer_x, pointer_y;
    /** Atoms cache */
    xutil_atom_cache_t *atoms;
    /** Lua VM state */
    lua_State *L;
    /** Default colors */
    struct
    {
        xcolor_t fg, bg;
    } colors;
    /** Default font */
    font_t *font;
    /** Respect resize hints */
    bool resize_hints;
    struct
    {
        /** Command to execute when spawning a new client */
        luaA_function newclient;
        /** Command to execute when giving focus to a client */
        luaA_function focus;
        /** Command to execute when removing focus to a client */
        luaA_function unfocus;
        /** Command to run when mouse is over */
        luaA_function mouseover;
        /** Command to run on arrange */
        luaA_function arrange;
    } hooks;
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
