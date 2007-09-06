/* See LICENSE file for copyright and license details. */

#ifndef JDWM_CONFIG_H
#define JDWM_CONFIG_H

#define JDWM_CONFIG_FILE ".jdwmrc"

#include <X11/Xlib.h>

/** Bar possible position */
enum
{ BarTop, BarBot, BarOff };

enum
{ ColBorder, ColFG, ColBG, ColLast };   /* color */

typedef struct
{
    int x, y, w, h;
    unsigned long norm[ColLast]; 
    unsigned long sel[ColLast];
    Drawable drawable;
    GC gc;
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
    const char *prop;
    const char *tags;
    Bool isfloating;
} Rule;

typedef struct jdwm_config jdwm_config;

typedef struct
{
    const char *symbol;
    void (*arrange) (Display *, jdwm_config *);
} Layout;

typedef struct
{
    unsigned long mod;
    KeySym keysym;
    void (*func) (Display *, jdwm_config *, const char *);
    const char *arg;
} Key;

/** Main configuration structure */
struct jdwm_config
{
    /** Tag list */
    const char **tags;
    /** Selected tags */
    Bool *selected_tags;
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
    /** Bar position */
    int bpos;
    /** Current bar position */
    int current_bpos;
    /** Border size */
    int borderpx;
    /** Master width factor */
    double mwfact;
    /** Number of pixels to snap windows */
    int snap;
    /** Number of master windows */
    int nmaster;
    /** Transparency of unfocused clients */
    int opacity_unfocused;
    /** Text displayed in bar */
    char statustext[256];
    /** Current layout */
    Layout * current_layout;
};

void parse_config(Display *, int, DC *, jdwm_config *);        /* parse configuration file */
void uicb_reload(Display *, jdwm_config *, const char *);              /* reload configuration file */

#endif
