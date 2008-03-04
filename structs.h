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

#include <regex.h>
#include "layout.h"
#include "common/draw.h"
#include "common/swindow.h"

/** Bar possible position */
typedef enum
{
    Top,
    Bottom,
    Left,
    Right,
    Off
} Position;

/** Rules for floating rule */
typedef enum
{
    No = False,
    Yes = True,
    Auto
} Fuzzy;

/** Common colors */
enum
{ ColBorder, ColFG, ColBG, ColLast };

/** Cursors */
enum
{ CurNormal, CurResize, CurMove, CurLast };

/** Rule type */
typedef struct Rule Rule;
struct Rule
{
    char *icon;
    char *xprop;
    int screen;
    Fuzzy isfloating;
    Fuzzy ismaster;
    double opacity;
    regex_t *prop_r;
    regex_t *tags_r;
    regex_t *xpropval_r;
    Rule *next;
};

/** Key bindings */
typedef struct Key Key;
struct Key
{
    unsigned long mod;
    KeyCode keycode;
    Uicb *func;
    char *arg;
    Key *next;
};

/** Mouse buttons bindings */
typedef struct Button Button;
struct Button
{
    unsigned long mod;
    unsigned int button;
    Uicb *func;
    char *arg;
    Button *next;
};

/** Widget tell status code */
typedef enum
{
    WIDGET_NOERROR = 0,
    WIDGET_ERROR,
    WIDGET_ERROR_CUSTOM,
    WIDGET_ERROR_FORMAT_BOOL,
    WIDGET_ERROR_FORMAT_FONT,
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
    void (*button_press)(Widget *, XButtonPressedEvent *);
    /** Statusbar */
    Statusbar *statusbar;
    /** Alignement */
    Alignment alignment;
    /** Misc private data */
    void *data;
    /** True if user supplied coords */
    Bool user_supplied_x;
    Bool user_supplied_y;
    /** Area */
    Area area;
    /** Buttons bindings */
    Button *buttons;
    /** Font */
    XftFont *font;
    /** Cache */
    struct
    {
        Bool needs_update;
        int flags;
    } cache;
    /** Next widget */
    Widget *next;
};

/** Status bar */
struct Statusbar
{
    /** Window */
    SimpleWindow *sw;
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
    /** Widget list */
    Widget *widgets;
    /** Next statusbar */
    Statusbar *next;
};

/** Client type */
typedef struct Client Client;
struct Client
{
    /** Client name */
    char name[256];
    /** Window geometry */
    Area geometry;
    /** Floating window geometry */
    Area f_geometry;
    /** Max window geometry */
    Area m_geometry;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    int border, oldborder;
    /** Has urgency hint */
    Bool isurgent;
    /** Store previous floating state before maximizing */
    Bool wasfloating;
    /** True if the window is floating */
    Bool isfloating;
    /** True if the window is fixed */
    Bool isfixed;
    /** True if the window is maximized */
    Bool ismax;
    /** True if the client must be skipped from client list */
    Bool skip;
    /** True if the client must be skipped from task bar client list */
    Bool skiptb;
    /** Next client */
    Client *next;
    /** Window of the client */
    Window win;
    /** Client logical screen */
    int screen;
    /** True if the client is a new one */
    Bool newcomer;
};

typedef struct client_node_t client_node_t;
struct client_node_t
{
    Client *client;
    client_node_t *next;
};

/** Tag type */
typedef struct Tag Tag;
struct Tag
{
    /** Tag name */
    char *name;
    /** Screen */
    int screen;
    /** True if selected */
    Bool selected;
    /** True if was selected before selecting others tags */
    Bool was_selected;
    /** Current tag layout */
    Layout *layout;
    /** Master width factor */
    double mwfact;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
    /** Next tag */
    Tag *next;
};

/** tag_client_node type */
typedef struct tag_client_node_t tag_client_node_t;
struct tag_client_node_t
{
    Tag *tag;
    Client *client;
    tag_client_node_t *next;
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

typedef Area (FloatingPlacement)(Area, int, int);
typedef struct
{
    /** Screen geometry */
    Area geometry;
    /** Number of pixels to snap windows */
    int snap;
    /** Border size */
    int borderpx;
    /** Mwfact limits */
    float mwfact_upper_limit, mwfact_lower_limit;
    /** Floating window placement algo */
    FloatingPlacement *floating_placement;
    /** Respect resize hints */
    Bool resize_hints;
    /** Sloppy focus: focus follow mouse */
    Bool sloppy_focus;
    /** True if we should raise windows on focus */
    Bool sloppy_focus_raise;
    /** Focus new clients */
    Bool new_get_focus;
    /** True if new clients should become master */
    Bool new_become_master;
    /** True if we need to arrange() */
    Bool need_arrange;
    /** Normal colors */
    XColor colors_normal[ColLast];
    /** Selected colors */
    XColor colors_selected[ColLast];
    /** Urgency colors */
    XColor colors_urgent[ColLast];
    /** Transparency of unfocused clients */
    int opacity_unfocused;
    /** Tag list */
    Tag *tags;
    /** Layout list */
    Layout *layouts;
    /** Status bar */
    Statusbar *statusbar;
    /** Padding */
    Padding padding;
    /** Font */
    XftFont *font;
    /** Draw shadow under text */
    int shadow_offset;
} VirtScreen;

/** Main configuration structure */
typedef struct AwesomeConf AwesomeConf;
struct AwesomeConf
{
    /** Display ref */
    Display *display;
    /** Logical screens */
    VirtScreen *screens;
    /** Number of logical screens */
    int nscreen;
    /** Rules list */
    Rule *rules;
    /** Keys bindings list */
    Key *keys;
    /** Mouse bindings list */
    struct
    {
           Button *root;
           Button *client;
    } buttons;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Check for XShape extension */
    Bool have_shape;
    /** Check for XRandR extension */
    Bool have_randr;
    /** Cursors */
    Cursor cursor[CurLast];
    /** Clients list */
    Client *clients;
    /** Scratch client */
    struct
    {
        Client *client;
        Bool isvisible;
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
