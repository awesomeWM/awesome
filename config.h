*
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
    void (*arrange) (awesome_config *);
} Layout;

typedef struct Key Key;
struct Key
{
    unsigned long mod;
    KeySym keysym;
    void (*func) (awesome_config *, char *);
    char *arg;
    Key *next;
};

typedef struct Button Button;
struct Button
{
    unsigned long mod;
    unsigned int button;
    void (*func) (awesome_config *, char *);
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
    int nlayouts;
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
    /** Border size */
    int borderpx;
    /** Number of pixels to snap windows */
    int snap;
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
    /** Cursors */
    Cursor cursor[CurLast];
    /** Padding */
    Padding padding;	
    /** Font */
    XftFont *font;
    /** Clients list */
    Client **clients;
    /** Path to config file */
    char *configpath;
};

#define AWESOME_DEFAULT_CONFIG \
"screen 0 \
{ \
    tags \
    { \
        tag one { } \
        tag two { } \
        tag three { } \
        tag four { } \
        tag five { } \
        tag six { } \
        tag seven { } \
        tag eight { } \
        tag nine { } \
    } \
    layouts \
    { \
        layout tile { symbol = \"[]=\" } \
        layout tileleft { symbol = \"=[]\" } \
        layout max { symbol = \"[ ]\" } \
        layout floating { symbol = \"><>\" } \
    } \
} \
 \
mouse \
{ \
    tag \
    { \
        button = \"1\" \
        command = \"tag_view\" \
    } \
    tag \
    { \
        button = \"1\" \
        modkey = {\"Mod4\"} \
        command = \"client_tag\" \
    } \
    tag \
    { \
        button = \"3\" \
        command = \"tag_toggleview\" \
    } \
    tag \
    { \
        button = \"3\" \
        modkey = {\"Mod4\"} \
        command = \"client_toggletag\" \
    } \
    tag \
    { \
        button = \"4\" \
        command = \"tag_viewnext\" \
    } \
    tag \
    { \
        button = \"5\" \
        command = \"tag_viewprev\" \
    } \
    layout \
    { \
        button = \"1\" \
        command = \"tag_setlayout\" \
        arg = \"+1\" \
    } \
    layout \
    { \
        button = \"4\" \
        command = \"tag_setlayout\" \
        arg = \"+1\" \
    } \
    layout \
    { \
        button = \"3\" \
        command = \"tag_setlayout\" \
        arg = \"-1\" \
    } \
    layout \
    { \
        button = \"5\" \
        command = \"tag_setlayout\" \
        arg = \"-1\" \
    } \
    root \
    { \
        button = \"3\" \
        command = \"spawn\" \
        arg = \"exec urxvt\" \
    } \
    root \
    { \
        button = \"4\" \
        command = \"tag_viewnext\" \
    } \
    root \
    { \
        button = \"5\" \
        command = \"tag_viewprev\" \
    } \
    client \
    { \
        modkey = {\"Mod4\"} \
        button = \"1\" \
        command = \"client_movemouse\" \
    } \
    client \
    { \
        modkey = {\"Mod4\"} \
        button = \"2\" \
        command = \"client_zoom\" \
    } \
    client \
    { \
        modkey = {\"Mod4\"} \
        button = \"3\" \
        command = \"client_resizemouse\" \
    } \
} \
 \
keys \
{ \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"Return\" \
        command = \"spawn\" \
        arg = \"exec xterm\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"space\" \
        command = \"tag_setlayout\" \
        arg = \"+1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"space\" \
        command = \"tag_setlayout\" \
        arg = \"-1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"b\" \
        command = \"togglebar\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"j\" \
        command = \"client_focusnext\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"k\" \
        command = \"client_focusprev\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"j\" \
        command = \"client_swapnext\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"k\" \
        command = \"client_swapprev\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"j\" \
        command = \"screen_focusnext\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"k\" \
        command = \"screen_focusprev\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"h\" \
        command = \"tag_setmwfact\" \
        arg = \"-0.05\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"l\" \
        command = \"tag_setmwfact\" \
        arg = \"+0.05\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"h\" \
        command = \"tag_setnmaster\" \
        arg = \"+1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"l\" \
        command = \"tag_setnmaster\" \
        arg = \"-1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"h\" \
        command = \"tag_setncol\" \
        arg = \"+1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"l\" \
        command = \"tag_setncol\" \
        arg = \"-1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"Escape\" \
        command = \"tag_viewprev_selected\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"Left\" \
        command = \"tag_viewprev\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"Right\" \
        command = \"tag_viewnext\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"m\" \
        command = \"client_togglemax\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"Return\" \
        command = \"client_zoom\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"space\" \
        command = \"client_togglefloating\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"c\" \
        command = \"client_kill\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"q\" \
        command = \"quit\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"r\" \
        command = \"reloadconfig\" \
    } \
    key \
    { \
       modkey = {\"Mod4\"} \
       key = \"0\" \
       command = \"tag_view\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"1\" \
        command = \"tag_view\" \
        arg = \"1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"2\" \
        command = \"tag_view\" \
        arg = \"2\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"3\" \
        command = \"tag_view\" \
        arg = \"3\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"4\" \
        command = \"tag_view\" \
        arg = \"4\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"5\" \
        command = \"tag_view\" \
        arg = \"5\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"6\" \
        command = \"tag_view\" \
        arg = \"6\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"7\" \
        command = \"tag_view\" \
        arg = \"7\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"8\" \
        command = \"tag_view\" \
        arg = \"8\" \
    } \
    key \
    { \
        modkey = {\"Mod4\"} \
        key = \"9\" \
        command = \"tag_view\" \
        arg = \"9\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"0\" \
        command = \"tag_toggleview\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"1\" \
        command = \"tag_toggleview\" \
        arg = \"1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"2\" \
        command = \"tag_toggleview\" \
        arg = \"2\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"3\" \
        command = \"tag_toggleview\" \
        arg = \"3\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"4\" \
        command = \"tag_toggleview\" \
        arg = \"4\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"5\" \
        command = \"tag_toggleview\" \
        arg = \"5\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"6\" \
        command = \"tag_toggleview\" \
        arg = \"6\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"7\" \
        command = \"tag_toggleview\" \
        arg = \"7\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"8\" \
        command = \"tag_toggleview\" \
        arg = \"8\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Control\"} \
        key = \"9\" \
        command = \"tag_toggleview\" \
        arg = \"9\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"0\" \
        command = \"client_tag\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"1\" \
        command = \"client_tag\" \
        arg = \"1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"2\" \
        command = \"client_tag\" \
        arg = \"2\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"3\" \
        command = \"client_tag\" \
        arg = \"3\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"4\" \
        command = \"client_tag\" \
        arg = \"4\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"5\" \
        command = \"client_tag\" \
        arg = \"5\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"6\" \
        command = \"client_tag\" \
        arg = \"6\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"7\" \
        command = \"client_tag\" \
        arg = \"7\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"8\" \
        command = \"client_tag\" \
        arg = \"8\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\"} \
        key = \"9\" \
        command = \"client_tag\" \
        arg = \"9\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"0\" \
        command = \"client_toggletag\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"1\" \
        command = \"client_toggletag\" \
        arg = \"1\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"2\" \
        command = \"client_toggletag\" \
        arg = \"2\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"3\" \
        command = \"client_toggletag\" \
        arg = \"3\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"4\" \
        command = \"client_toggletag\" \
        arg = \"4\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"5\" \
        command = \"client_toggletag\" \
        arg = \"5\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"6\" \
        command = \"client_toggletag\" \
        arg = \"6\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"7\" \
        command = \"client_toggletag\" \
        arg = \"7\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"8\" \
        command = \"client_toggletag\" \
        arg = \"8\" \
    } \
    key \
    { \
        modkey = {\"Mod4\", \"Shift\", \"Control\"} \
        key = \"9\" \
        command = \"client_toggletag\" \
        arg = \"9\" \
    } \
}"

void parse_config(const char *, awesome_config *);

void uicb_reloadconfig(awesome_config *, const char *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
