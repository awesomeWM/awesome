/*
 * config.h - configuration management header
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <regex.h>

/** Bar possible position */
enum
{ BarTop, BarBot, BarLeft, BarRight, BarOff };

enum
{ ColBorder, ColFG, ColBG, ColLast };   /* color */

enum
{ CurNormal, CurResize, CurMove, CurLast };     /* cursor */

typedef struct Rule Rule;
struct Rule
{
    char *prop;
    char *tags;
    int screen;
    Bool isfloating;
    regex_t *propregex;
    regex_t *tagregex;
    Rule *next;
};

typedef struct awesome_config awesome_config;

typedef struct
{
    char *symbol;
    void (*arrange) (awesome_config *, int);
} Layout;

typedef struct Key Key;
struct Key
{
    unsigned long mod;
    KeySym keysym;
    void (*func) (awesome_config *, int, char *);
    char *arg;
    Key *next;
};

typedef struct Button Button;
struct Button
{
    unsigned long mod;
    unsigned int button;
    void (*func) (awesome_config *, int, char *);
    char *arg;
    Button *next;
};

/** Status bar */
typedef struct
{
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Layout txt width */
    int txtlayoutwidth;
    /** Default position */
    int dposition;
    /** Bar position */
    int position;
    /** Window */
    Window window;
    /** Screen */
    int screen;
} Statusbar;

typedef struct Client Client; 
struct Client
{
    /** Client name */
    char name[256];
    /** Window geometry */
    int x, y, w, h;
    /** Real window geometry for floating */
    int rx, ry, rw, rh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    long flags;
    int border, oldborder;
    /** Store previous floating state before maximizing */
    Bool wasfloating;
    /** True if the window is floating */
    Bool isfloating;
    /** True if the window is fixed */
    Bool isfixed;
    /** True if the window is maximized */
    Bool ismax;
    /** Tags for the client */
    Bool *tags;
    /** Next client */
    Client *next;
    /** Previous client */
    Client *prev;
    /** Window of the client */
    Window win;
    /** Client display */
    Display *display;
    /** Client logical screen */
    int screen;
    /** Client physical screen */
    int phys_screen;
};

/** Tag type */
typedef struct
{
    /** Tag name */
    char *name;
    /** True if selected */
    Bool selected;
    /** True if was selected before selecting others tags */
    Bool was_selected;
    /** Current tag layout */
    Layout *layout;
    /** Selected client on this tag */
    Client *client_sel;
    /** Master width factor */
    double mwfact;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
} Tag;

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
    /** Text displayed in bar */
    char statustext[256];
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
    /** Normal colors */
    XColor colors_normal[ColLast];
    /** Selected colors */
    XColor colors_selected[ColLast];
    /** Tag list */
    Tag *tags;
    /** Number of tags in **tags */
    int ntags;
    /** Layout list */
    Layout *layouts;
    int nlayouts;
    /** Status bar */
    Statusbar statusbar;
    /** Padding */
    Padding padding;	
    /** Font */
    XftFont *font;
} VirtScreen;

/** Main configuration structure */
struct awesome_config
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
           Button *tag;
           Button *title;
           Button *layout;
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
};

void parse_config(const char *, awesome_config *);

void uicb_reloadconfig(awesome_config *, const char *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
