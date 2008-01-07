/*
 * config.h - configuration management header
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

#ifndef AWESOME_CONFIG_H
#define AWESOME_CONFIG_H

#include <regex.h>
#include "draw.h"
#include "layout.h"

/** Bar possible position */
typedef enum
{
    Top,
    Bottom,
    Left,
    Right,
    Off
} Position;

typedef enum
{
    Float,
    Tile,
    Auto,
} RuleFloat;

/** Common colors */
enum
{ ColBorder, ColFG, ColBG, ColLast };

enum
{ CurNormal, CurResize, CurMove, CurLast };     /* cursor */

typedef struct Rule Rule;
struct Rule
{
    char *icon;
    char *xprop;
    int screen;
    RuleFloat isfloating;
    Bool not_master;
    regex_t *prop_r;
    regex_t *tags_r;
    regex_t *xpropval_r;
    Rule *next;
};

typedef struct AwesomeConf AwesomeConf;

typedef struct Key Key;
struct Key
{
    unsigned long mod;
    KeySym keysym;
    Uicb *func;
    char *arg;
    Key *next;
};

typedef struct Button Button;
struct Button
{
    unsigned long mod;
    unsigned int button;
    Uicb *func;
    char *arg;
    Button *next;
};

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
    void (*tell)(Widget *, char *);
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
    /** Statusbar name */
    char *name;
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Layout txt width */
    int txtlayoutwidth;
    /** Default position */
    Position dposition;
    /** Bar position */
    Position position;
    /** Window */
    Window window;
    /** Screen */
    int screen;
    /** Widget list */
    Widget *widgets;
    /** Drawable */
    Drawable drawable;
    /** Next statusbar */
    Statusbar *next;
};

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
    /** Previous client */
    Client *prev;
    /** Window of the client */
    Window win;
    /** Client logical screen */
    int screen;
};

typedef struct FocusList FocusList;
struct FocusList
{
    Client *client;
    FocusList *prev;
};

/** Tag type */
typedef struct Tag Tag;
struct Tag
{
    /** Tag name */
    char *name;
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

/** TagClientLink type */
typedef struct TagClientLink TagClientLink;
struct TagClientLink
{
    Tag *tag;
    Client *client;
    TagClientLink *next;
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

typedef struct
{
    /** Number of pixels to snap windows */
    int snap;
    /** Border size */
    int borderpx;
    /** Transparency of unfocused clients */
    int opacity_unfocused;
    /** Focus move pointer */
    Bool focus_move_pointer;
    /** Allow floats to be lowered on focus change */
    Bool allow_lower_floats;
    /** Respect resize hints */
    Bool resize_hints;
    /** Sloppy focus: focus follow mouse */
    Bool sloppy_focus;
    /** True if new clients should become master */
    Bool new_become_master;
    /** Normal colors */
    XColor colors_normal[ColLast];
    /** Selected colors */
    XColor colors_selected[ColLast];
    /** Urgency colors */
    XColor colors_urgent[ColLast];
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
} VirtScreen;

/** Main configuration structure */
struct AwesomeConf
{
    /** Display ref */
    Display *display;
    /** Logical screens */
    VirtScreen *screens;
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
    /** Path to config file */
    char *configpath;
    /** Selected clients on this tag */
    FocusList *focus;
    /** Link between tags and clients */
    TagClientLink *tclink;
};

void config_parse(const char *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
