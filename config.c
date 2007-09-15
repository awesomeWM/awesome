/*  
 * config.c - configuration management
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


/**
 * \defgroup ui_callback
 */

#include <libconfig.h>
#include <X11/keysym.h>

#include "awesome.h"
#include "layout.h"
#include "tag.h"
#include "layouts/tile.h"
#include "layouts/grid.h"
#include "layouts/spiral.h"
#include "layouts/floating.h"

/* static */
static void initfont(const char *, Display *, DC *);
static unsigned long initcolor(const char *colstr, Display *, int);
static unsigned int get_numlockmask(Display *);

/** Main configuration object for parsing*/
config_t awesomelibconf;

/** Link a name to a function */
typedef struct
{
    const char *name;
    void *func;
} NameFuncLink;

/** Link a name to a key symbol */
typedef struct
{
    const char *name;
    KeySym keysym;
} KeyMod;

/** List of keyname and corresponding X11 mask codes */
static const KeyMod KeyModList[] =
{
    {"Shift", ShiftMask},
    {"Lock", LockMask},
    {"Control", ControlMask},
    {"Mod1", Mod1Mask},
    {"Mod2", Mod2Mask},
    {"Mod3", Mod3Mask},
    {"Mod4", Mod4Mask},
    {"Mod5", Mod5Mask},
    {"None", 0}
};

/** List of available layouts and link between name and functions */
static const NameFuncLink LayoutsList[] =
{
    {"tile", tile},
    {"tileleft", tileleft},
    {"floating", floating},
    {"grid", grid},
    {"spiral", spiral},
    {"dwindle", dwindle},
    {NULL, NULL}
};

/** List of available UI bindable callbacks and functions */
static const NameFuncLink KeyfuncList[] = {
    /* util.c */
    {"spawn", uicb_spawn},
    /* client.c */
    {"killclient", uicb_killclient},
    {"moveresize", uicb_moveresize},
    {"settrans", uicb_settrans},
    /* tag.c */
    {"tag", uicb_tag},
    {"togglefloating", uicb_togglefloating},
    {"toggleview", uicb_toggleview},
    {"toggletag", uicb_toggletag},
    {"view", uicb_view},
    {"viewprevtags", uicb_viewprevtags},
    /* layout.c */
    {"setlayout", uicb_setlayout},
    {"togglebar", uicb_togglebar},
    {"focusnext", uicb_focusnext},
    {"focusprev", uicb_focusprev},
    {"togglemax", uicb_togglemax},
    {"toggleverticalmax", uicb_toggleverticalmax},
    {"togglehorizontalmax", uicb_togglehorizontalmax},
    {"zoom", uicb_zoom},
    /* layouts/tile.c */
    {"setmwfact", uicb_setmwfact},
    {"setnmaster", uicb_setnmaster},
    /* awesome.c */
    {"quit", uicb_quit},
    {NULL, NULL}
};

/** Lookup for a key mask from its name
 * \param keyname Key name
 * \return Key mask or 0 if not found
 */
static KeySym
key_mask_lookup(const char *keyname)
{
    int i;

    if(keyname)
        for(i = 0; KeyModList[i].name; i++)
            if(!strcmp(keyname, KeyModList[i].name))
                return KeyModList[i].keysym;

    return 0;
}

/** Lookup for a function pointer from its name
 * in the given NameFuncLink list
 * \param funcname Function name
 * \param list Function and name link list
 * \return function pointer
 */
static void *
name_func_lookup(const char *funcname, const NameFuncLink * list)
{
    int i;

    if(funcname && list)
        for(i = 0; list[i].name; i++)
            if(!strcmp(funcname, list[i].name))
                return list[i].func;

    return NULL;
}

/** Set default configuration
 * \param awesomeconf awesome config ref
 */
static void
set_default_config(awesome_config *awesomeconf)
{
    strcpy(awesomeconf->statustext, "awesome-" VERSION);
    awesomeconf->statusbar.width = 0;
    awesomeconf->statusbar.height = 0;
    awesomeconf->opacity_unfocused = -1;
    awesomeconf->nkeys = 0;
    awesomeconf->nrules = 0;
}

/** Parse configuration file and initialize some stuff
 * \param disp Display ref
 * \param scr Screen number
 * \param drawcontext Draw context
 */
void
parse_config(Display * disp, int scr, DC * drawcontext, awesome_config *awesomeconf)
{
    config_setting_t *conftags;
    config_setting_t *conflayouts, *confsublayouts;
    config_setting_t *confrules, *confsubrules;
    config_setting_t *confkeys, *confsubkeys, *confkeysmasks, *confkeymaskelem;
    int i = 0, j = 0;
    double f = 0.0;
    const char *tmp, *homedir;
    char *confpath;
    KeySym tmp_key;

    set_default_config(awesomeconf);

    homedir = getenv("HOME");
    confpath = p_new(char, strlen(homedir) + strlen(AWESOME_CONFIG_FILE) + 2);
    strcpy(confpath, homedir);
    strcat(confpath, "/");
    strcat(confpath, AWESOME_CONFIG_FILE);

    config_init(&awesomelibconf);

    if(config_read_file(&awesomelibconf, confpath) == CONFIG_FALSE)
        eprint("awesome: error parsing configuration file at line %d: %s\n",
               config_error_line(&awesomelibconf), config_error_text(&awesomelibconf));

    /* font */
    tmp = config_lookup_string(&awesomelibconf, "awesome.font");
    initfont(tmp ? tmp : "-*-fixed-medium-r-normal-*-13-*-*-*-*-*-*-*", disp, drawcontext);

    /* layouts */
    conflayouts = config_lookup(&awesomelibconf, "awesome.layouts");

    if(!conflayouts)
        fprintf(stderr, "layouts not found in configuration file\n");
    else
    {
        awesomeconf->nlayouts = config_setting_length(conflayouts);
        awesomeconf->layouts = p_new(Layout, awesomeconf->nlayouts + 1);
        for(i = 0; (confsublayouts = config_setting_get_elem(conflayouts, i)); i++)
        {
            awesomeconf->layouts[i].arrange = 
                name_func_lookup(config_setting_get_string_elem(confsublayouts, 1), LayoutsList);
            if(!awesomeconf->layouts[i].arrange)
            {
                fprintf(stderr, "awesome: unknown layout #%d in configuration file\n", i);
                awesomeconf->layouts[i].symbol = NULL;
                continue;
            }
            awesomeconf->layouts[i].symbol = config_setting_get_string_elem(confsublayouts, 0);

            j = textw(drawcontext->font.set, drawcontext->font.xfont, awesomeconf->layouts[i].symbol, drawcontext->font.height);
            if(j > awesomeconf->statusbar.width)
                awesomeconf->statusbar.width = j;
        }
        awesomeconf->layouts[i].symbol = NULL;
        awesomeconf->layouts[i].arrange = NULL;
    }


    if(!awesomeconf->layouts[0].arrange)
        eprint("awesome: fatal: no default layout available\n");
    /** \todo put this in set_default_layout */
    awesomeconf->current_layout = awesomeconf->layouts;

    /* tags */
    conftags = config_lookup(&awesomelibconf, "awesome.tags");

    if(!conftags)
        eprint("awesome: fatal: no tags found in configuration file\n");
    awesomeconf->ntags = config_setting_length(conftags);
    awesomeconf->tags = p_new(const char *, awesomeconf->ntags);
    awesomeconf->selected_tags = p_new(Bool, awesomeconf->ntags);
    awesomeconf->prev_selected_tags = p_new(Bool, awesomeconf->ntags);
    awesomeconf->tag_layouts = p_new(Layout *, awesomeconf->ntags);

    for(i = 0; (tmp = config_setting_get_string_elem(conftags, i)); i++)
    {
        awesomeconf->tags[i] = tmp;
        awesomeconf->selected_tags[i] = False;
        awesomeconf->prev_selected_tags[i] = False;
        /** \todo add support for default tag/layout in configuration file */
        awesomeconf->tag_layouts[i] = awesomeconf->layouts;
    }

    /* select first tag by default */
    awesomeconf->selected_tags[0] = True;
    awesomeconf->prev_selected_tags[0] = True;

    /* rules */
    confrules = config_lookup(&awesomelibconf, "awesome.rules");

    if(!confrules)
        fprintf(stderr, "awesome: no rules found in configuration file\n");
    else
    {
        awesomeconf->nrules = config_setting_length(confrules);
        awesomeconf->rules = p_new(Rule, awesomeconf->nrules);
        for(i = 0; (confsubrules = config_setting_get_elem(confrules, i)); i++)
        {
            awesomeconf->rules[i].prop = config_setting_get_string(config_setting_get_member(confsubrules, "name"));
            awesomeconf->rules[i].tags = config_setting_get_string(config_setting_get_member(confsubrules, "tags"));
            if(awesomeconf->rules[i].tags && !strlen(awesomeconf->rules[i].tags))
                awesomeconf->rules[i].tags = NULL;
            awesomeconf->rules[i].isfloating =
                config_setting_get_bool(config_setting_get_member(confsubrules, "float"));
        }
    }

    /* modkey */
    tmp_key = key_mask_lookup(config_lookup_string(&awesomelibconf, "awesome.modkey"));
    awesomeconf->modkey = tmp_key ? tmp_key : Mod1Mask;

    /* find numlock mask */
    awesomeconf->numlockmask = get_numlockmask(disp);

    /* keys */
    confkeys = config_lookup(&awesomelibconf, "awesome.keys");

    if(!confkeys)
        fprintf(stderr, "awesome: no keys found in configuration file\n");
    else
    {
        awesomeconf->nkeys = config_setting_length(confkeys);
        awesomeconf->keys = p_new(Key, awesomeconf->nkeys);

        for(i = 0; (confsubkeys = config_setting_get_elem(confkeys, i)); i++)
        {
            confkeysmasks = config_setting_get_elem(confsubkeys, 0);
            for(j = 0; (confkeymaskelem = config_setting_get_elem(confkeysmasks, j)); j++)
                awesomeconf->keys[i].mod |= key_mask_lookup(config_setting_get_string(confkeymaskelem));
            awesomeconf->keys[i].keysym = XStringToKeysym(config_setting_get_string_elem(confsubkeys, 1));
            awesomeconf->keys[i].func =
                name_func_lookup(config_setting_get_string_elem(confsubkeys, 2), KeyfuncList);
            awesomeconf->keys[i].arg = config_setting_get_string_elem(confsubkeys, 3);
        }
    }

    /* barpos */
    tmp = config_lookup_string(&awesomelibconf, "awesome.barpos");

    if(tmp && !strncmp(tmp, "BarOff", 6))
        awesomeconf->statusbar_default_position = BarOff;
    else if(tmp && !strncmp(tmp, "BarBot", 6))
        awesomeconf->statusbar_default_position = BarBot;
    else
        awesomeconf->statusbar_default_position = BarTop;

    awesomeconf->statusbar.position = awesomeconf->statusbar_default_position;

    /* borderpx */
    awesomeconf->borderpx = config_lookup_int(&awesomelibconf, "awesome.borderpx");

    /* opacity */
    awesomeconf->opacity_unfocused = config_lookup_int(&awesomelibconf, "awesome.opacity_unfocused");
    if(awesomeconf->opacity_unfocused >= 100)
        awesomeconf->opacity_unfocused = -1;

    /* snap */
    i = config_lookup_int(&awesomelibconf, "awesome.snap");
    awesomeconf->snap = i ? i : 8;

    /* nmaster */
    i = config_lookup_int(&awesomelibconf, "awesome.nmaster");
    awesomeconf->nmaster = i ? i : 1;

    /* mwfact */
    f = config_lookup_float(&awesomelibconf, "awesome.mwfact");
    awesomeconf->mwfact = f ? f : 0.6;

    /* resize_hints */
    awesomeconf->resize_hints = config_lookup_float(&awesomelibconf, "awesome.resize_hints");

    /* colors */
    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_border_color");
    drawcontext->norm[ColBorder] = initcolor(tmp ? tmp : "#dddddd", disp, scr);

    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_bg_color");
    drawcontext->norm[ColBG] = initcolor(tmp ? tmp : "#000000", disp, scr);

    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_fg_color");
    drawcontext->norm[ColFG] = initcolor(tmp ? tmp : "#ffffff", disp, scr);

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_border_color");
    drawcontext->sel[ColBorder] = initcolor(tmp ? tmp : "#008b8b", disp, scr);

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_bg_color");
    drawcontext->sel[ColBG] = initcolor(tmp ? tmp : "#008b8b", disp, scr);

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_fg_color");
    drawcontext->sel[ColFG] = initcolor(tmp ? tmp : "#ffffff", disp, scr);

    p_delete(&confpath);
}

/** Initialize font from X side and store in draw context
 * \param fontstr Font name
 * \param disp Display ref
 * \param drawcontext Draw context
 */
static void
initfont(const char *fontstr, Display * disp, DC * drawcontext)
{
    char *def, **missing;
    int i, n;

    missing = NULL;
    if(drawcontext->font.set)
        XFreeFontSet(disp, drawcontext->font.set);
    drawcontext->font.set = XCreateFontSet(disp, fontstr, &missing, &n, &def);
    if(missing)
    {
        while(n--)
            fprintf(stderr, "awesome: missing fontset: %s\n", missing[n]);
        XFreeStringList(missing);
    }
    if(drawcontext->font.set)
    {
        XFontSetExtents *font_extents;
        XFontStruct **xfonts;
        char **font_names;
        drawcontext->font.ascent = drawcontext->font.descent = 0;
        font_extents = XExtentsOfFontSet(drawcontext->font.set);
        n = XFontsOfFontSet(drawcontext->font.set, &xfonts, &font_names);
        for(i = 0, drawcontext->font.ascent = 0, drawcontext->font.descent = 0; i < n; i++)
        {
            if(drawcontext->font.ascent < (*xfonts)->ascent)
                drawcontext->font.ascent = (*xfonts)->ascent;
            if(drawcontext->font.descent < (*xfonts)->descent)
                drawcontext->font.descent = (*xfonts)->descent;
            xfonts++;
        }
    }
    else
    {
        if(drawcontext->font.xfont)
            XFreeFont(disp, drawcontext->font.xfont);
        drawcontext->font.xfont = NULL;
        if(!(drawcontext->font.xfont = XLoadQueryFont(disp, fontstr)))
            eprint("error, cannot load font: '%s'\n", fontstr);
        drawcontext->font.ascent = drawcontext->font.xfont->ascent;
        drawcontext->font.descent = drawcontext->font.xfont->descent;
    }
    drawcontext->font.height = drawcontext->font.ascent + drawcontext->font.descent;
}

static unsigned int
get_numlockmask(Display *disp)
{
    XModifierKeymap *modmap;
    unsigned int mask = 0;
    int i, j;

    modmap = XGetModifierMapping(disp);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
        {
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(disp, XK_Num_Lock))
                mask = (1 << i);
        }

    XFreeModifiermap(modmap);

    return mask;
}

/** Initialize color from X side
 * \param colorstr Color code
 * \param disp Display ref
 * \param scr Screen number
 * \return XColor pixel
 */
static unsigned long
initcolor(const char *colstr, Display * disp, int scr)
{
    Colormap cmap = DefaultColormap(disp, scr);
    XColor color;
    if(!XAllocNamedColor(disp, cmap, colstr, &color, &color))
        eprint("error, cannot allocate color '%s'\n", colstr);
    return color.pixel;
}
