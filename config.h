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

#define AWESOME_CONFIG_FILE ".awesomerc"

#include <X11/Xlib.h>

/** Bar possible position */
enum
{ BarTop, BarBot, BarOff };

enum
{ ColBorder, ColFG, ColBG, ColLast };   /* color */

enum 
{ CurNormal, CurResize, CurMove, CurLast };     /* cursor */ 

typedef struct
{
    int x, y, w, h;
    unsigned long norm[ColLast]; 
    unsigned long sel[ColLast];
    GC gc;
    Cursor cursor[CurLast];
    struct
    {
        int ascent;
        int descent;
        int height;
        XFontSet set;
        XFontStruct *xfont;
    } font;
} DC;

typedef struct
{
    char *prop;
    char *tags;
    Bool isfloating;
} Rule;

typedef struct awesome_config awesome_config;

typedef struct
{
    char *symbol;
    void (*arrange) (Display *, awesome_config *);
} Layout;

typedef struct
{
    unsigned long mod;
    KeySym keysym;
    void (*func) (Display *, DC *, awesome_config *, char *);
    char *arg;
} Key;

/** Status bar */
typedef struct
{
    /** Bar width */
    int width;
    /** Bar height */
    int height;
    /** Bar position */
    int position;
    /** Window */
    Window window;
    /** Drawable object */
    Drawable drawable;
    /** Screen */
    int screen;
} Statusbar;

/** Main configuration structure */
struct awesome_config
{
    /** Config screen number */
    int screen;
    /** Tag list */
    char **tags;
    /** Selected tags */
    Bool *selected_tags;
    /* Previously selected tags */
    Bool *prev_selected_tags;
    /** Number of tags in **tags */
    int ntags;
    /** Layout list */
    Layout *layouts;
    /** Number of layouts in *layouts */
    int nlayouts;
    /** Store layout for eatch tag */
    Layout **tag_layouts;
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
    int ncols;
    /** Transparency of unfocused clients */
    int opacity_unfocused;
    /** Respect resize hints */
    Bool resize_hints;
    /** Text displayed in bar */
    char statustext[256];
    /** Current layout */
    Layout * current_layout;
    /** Status bar */
    Statusbar statusbar;
    /** Check for XShape extension */
    Bool have_shape;
    /** Check for XRandR extension */
    Bool have_randr;
};

void parse_config(Display *, int, DC *, awesome_config *);        /* parse configuration file */

#endif
