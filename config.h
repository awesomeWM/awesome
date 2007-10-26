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
{ BarTop, BarBot, BarOff };

enum
{ ColBorder, ColFG, ColBG, ColLast };   /* color */

enum
{ CurNormal, CurResize, CurMove, CurLast };     /* cursor */

typedef struct
{
    char *prop;
    char *tags;
    Bool isfloating;
    regex_t *propregex;
    regex_t *tagregex;
} Rule;

typedef struct awesome_config awesome_config;

typedef struct
{
    char *symbol;
    void (*arrange) (awesome_config *);
} Layout;

typedef struct
{
    unsigned long mod;
    KeySym keysym;
    void (*func) (awesome_config *, char *);
    char *arg;
} Key;

/** Status bar */
typedef struct
{
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Layout txt width */
    int txtlayoutwidth;
    /** Bar position */
    int position;
    /** Window */
    Window window;
    /** Drawable object */
    Drawable drawable;
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
    /** Tabs support */
    struct
    {
          /** Next client in tab */
          Client *next;
          /** Previous client in tab */
          Client *prev;
          /** True if client is the visible one */
          Bool isvisible;
          /** True if client is tabbed */
    } tab;
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
} Tag;

/** Main configuration structure */
struct awesome_config
{
    /** Display ref */
    Display *display;
    /** Config virtual screen number */
    int screen;
    /** Config physical screen */
    int phys_screen;
    /** Tag list */
    Tag *tags;
    /** Number of tags in **tags */
    int ntags;
    /** Layout list */
    Layout *layouts;
    /** Number of layouts in *layouts */
    int nlayouts;
    /** Rules list */
    Rule *rules;
    /** Number of rules in *rules */
    int nrules;
    /** Keys binding list */
    Key *keys;
    /** Number of keys binding in *keys */
    int nkeys;
    /** Default modkey */
    KeySym modkey;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Default status bar position */
    int statusbar_default_position;
    /** Border size */
    int borderpx;
    /** Master width factor */
    double mwfact;
    /** Number of pixels to snap windows */
    int snap;
    /** Number of master windows */
    int nmaster;
    /** Number of columns in tile layout */
    int ncol;
    /** Transparency of unfocused clients */
    int opacity_unfocused;
    /** Focus move pointer */
    Bool focus_move_pointer;
    /** Allow floats to be lowered on focus change */
    Bool allow_lower_floats;
    /** Respect resize hints */
    Bool resize_hints;
    /** Text displayed in bar */
    char statustext[256];
    /** Status bar */
    Statusbar statusbar;
    /** Check for XShape extension */
    Bool have_shape;
    /** Check for XRandR extension */
    Bool have_randr;
    /** Normal colors */
    XColor colors_normal[ColLast];
    /** Selected colors */
    XColor colors_selected[ColLast];
    /** Tabbed colors */
    XColor colors_tab[ColLast];
    /** Cursors */
    Cursor cursor[CurLast];
    /** Font */
    XftFont *font;
    /** Clients list */
    Client **clients;
    /** Focused client */
    Client **client_sel;
};

void parse_config(Display *, int, const char *, awesome_config *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
