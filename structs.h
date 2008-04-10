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

#include <regex.h>
#include "layout.h"
#include "common/draw.h"
#include "common/swindow.h"
#include "common/xscreen.h"

/** stacking layout */
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

typedef struct
{
    simple_window_t *sw;
    Position position;
    Position dposition;
    Alignment align;
    Alignment text_align;
    int width, height;
    /** Colors */
    struct
    {
        style_t normal;
        style_t focus;
        style_t urgent;
    } styles;
} titlebar_t;

/** Rule type */
typedef struct rule_t rule_t;
struct rule_t
{
    char *icon;
    char *xprop;
    int screen;
    Fuzzy isfloating;
    Fuzzy ismaster;
    titlebar_t titlebar;
    double opacity;
    regex_t *prop_r;
    regex_t *tags_r;
    regex_t *xpropval_r;
    /** Next and previous rules */
    rule_t *prev, *next;
};

/** Key bindings */
typedef struct Key Key;
struct Key
{
    unsigned long mod;
    xcb_keysym_t keysym;
    xcb_keycode_t keycode;
    Uicb *func;
    char *arg;
    /** Next and previous keys */
    Key *prev, *next;
};

/** Mouse buttons bindings */
typedef struct Button Button;
struct Button
{
    unsigned long mod;
    unsigned int button;
    Uicb *func;
    char *arg;
    /** Next and previous buttons */
    Button *prev, *next;
};

/** Widget tell status code */
typedef enum
{
    WIDGET_NOERROR = 0,
    WIDGET_ERROR,
    WIDGET_ERROR_NOVALUE,
    WIDGET_ERROR_CUSTOM,
    WIDGET_ERROR_FORMAT_BOOL,
    WIDGET_ERROR_FORMAT_FONT,
    WIDGET_ERROR_FORMAT_COLOR,
    WIDGET_ERROR_FORMAT_SECTION
} widget_tell_status_t;

/** Widget */
typedef struct Widget Widget;
typedef struct Statusbar Statusbar;
struct Widget
{
    /** Widget name */
    char *name;
    /** Draw function */
    int (*draw)(Widget *, DrawCtx *, int, int);
    /** Update function */
    widget_tell_status_t (*tell)(Widget *, char *, char *);
    /** ButtonPressedEvent handler */
    void (*button_press)(Widget *, xcb_button_press_event_t *);
    /** Statusbar */
    Statusbar *statusbar;
    /** Alignement */
    Alignment alignment;
    /** Misc private data */
    void *data;
    /** true if user supplied coords */
    bool user_supplied_x;
    bool user_supplied_y;
    /** area_t */
    area_t area;
    /** Buttons bindings */
    Button *buttons;
    /** Cache */
    struct
    {
        bool needs_update;
        int flags;
    } cache;
    /** Next and previous widgets */
    Widget *prev, *next;
};

/** Status bar */
struct Statusbar
{
    /** Window */
    simple_window_t *sw;
    /** Statusbar name */
    char *name;
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Default position */
    Position dposition;
    /** Bar position */
    Position position;
    /** Screen */
    int screen;
    /** Physical screen id */
    int phys_screen;
    /** Widget list */
    Widget *widgets;
    /** Draw context */
    DrawCtx *ctx;
    /** Next and previous statusbars */
    Statusbar *prev, *next;
};

/** Client type */
typedef struct Client Client;
struct Client
{
    /** Client name */
    char name[256];
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
    /** true if the window is maximized */
    bool ismax;
    /** true if the client must be skipped from client list */
    bool skip;
    /** true if the client is moving */
    bool ismoving;
    /** true if the client must be skipped from task bar client list */
    bool skiptb;
    /** Next and previous clients */
    Client *prev, *next;
    /** Window of the client */
    xcb_window_t win;
    /** Client logical screen */
    int screen;
    /** Client physical screen */
    int phys_screen;
    /** True if the client is a new one */
    bool newcomer;
    /** titlebar_t */
    titlebar_t titlebar;
    /** layer in the stacking order */
    layer_t layer;
    layer_t oldlayer;
};

typedef struct client_node_t client_node_t;
struct client_node_t
{
    Client *client;
    /** Next and previous client_nodes */
    client_node_t *prev, *next;
};

/** Tag type */
typedef struct Tag Tag;
struct Tag
{
    /** Tag name */
    char *name;
    /** Screen */
    int screen;
    /** true if selected */
    bool selected;
    /** true if was selected before selecting others tags */
    bool was_selected;
    /** Current tag layout */
    Layout *layout;
    /** Master width factor */
    double mwfact;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
    /** Next and previous tags */
    Tag *prev, *next;
};

/** tag_client_node type */
typedef struct tag_client_node_t tag_client_node_t;
struct tag_client_node_t
{
    Tag *tag;
    Client *client;
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

typedef area_t (FloatingPlacement)(Client *);
typedef struct
{
    /** titlebar_t default parameters */
    titlebar_t titlebar_default;
    /** Number of pixels to snap windows */
    int snap;
    /** Border size */
    int borderpx;
    /** Mwfact limits */
    float mwfact_upper_limit, mwfact_lower_limit;
    /** Floating window placement algo */
    FloatingPlacement *floating_placement;
    /** Respect resize hints */
    bool resize_hints;
    /** Sloppy focus: focus follow mouse */
    bool sloppy_focus;
    /** true if we should raise windows on focus */
    bool sloppy_focus_raise;
    /** Focus new clients */
    bool new_get_focus;
    /** true if new clients should become master */
    bool new_become_master;
    /** true if we need to arrange() */
    bool need_arrange;
    /** Colors */
    struct
    {
        style_t normal;
        style_t focus;
        style_t urgent;
    } styles;
    /** Transparency of unfocused clients */
    double opacity_unfocused;
    /** Transparency of focused clients */
    double opacity_focused;
    /** Tag list */
    Tag *tags;
    /** Layout list */
    Layout *layouts;
    /** Status bar */
    Statusbar *statusbar;
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
    ScreensInfo *screens_info;
    /** Rules list */
    rule_t *rules;
    /** Keys bindings list */
    Key *keys;
    /** Mouse bindings list */
    struct
    {
           Button *root;
           Button *client;
           Button *titlebar;
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
    /** Clients list */
    Client *clients;
    /** Scratch client */
    struct
    {
        Client *client;
        bool isvisible;
    } scratch;
    /** Path to config file */
    char *configpath;
    /** Selected clients history */
    client_node_t *focus;
    /** Link between tags and clients */
    tag_client_node_t *tclink;
    /** Command line passed to awesome */
    char *argv;
    /** Last XMotionEvent coords */
    int pointer_x, pointer_y;
};

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
