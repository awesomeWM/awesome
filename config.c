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
#include "draw.h"
#include "util.h"
#include "statusbar.h"
#include "screen.h"
#include "layouts/tile.h"
#include "layouts/max.h"
#include "layouts/floating.h"

static XColor initxcolor(Display *, int, const char *);
static unsigned int get_numlockmask(Display *);

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
    {"tile", layout_tile},
    {"tileleft", layout_tileleft},
    {"max", layout_max},
    {"floating", layout_floating},
    {NULL, NULL}
};

/** List of available UI bindable callbacks and functions */
static const NameFuncLink KeyfuncList[] = {
    /* util.c */
    {"spawn", uicb_spawn},
    {"exec", uicb_exec},
    /* client.c */
    {"killclient", uicb_killclient},
    {"moveresize", uicb_moveresize},
    {"settrans", uicb_settrans},
    {"setborder", uicb_setborder},
    {"swapnext", uicb_swapnext},
    {"swapprev", uicb_swapprev},
    /* tag.c */
    {"tag", uicb_tag},
    {"togglefloating", uicb_togglefloating},
    {"toggleview", uicb_toggleview},
    {"toggletag", uicb_toggletag},
    {"view", uicb_view},
    {"view_tag_prev_selected", uicb_tag_prev_selected},
    {"view_tag_previous", uicb_tag_viewprev},
    {"view_tag_next", uicb_tag_viewnext},
    /* layout.c */
    {"setlayout", uicb_setlayout},
    {"focusnext", uicb_focusnext},
    {"focusprev", uicb_focusprev},
    {"togglemax", uicb_togglemax},
    {"toggleverticalmax", uicb_toggleverticalmax},
    {"togglehorizontalmax", uicb_togglehorizontalmax},
    {"zoom", uicb_zoom},
    /* layouts/tile.c */
    {"setmwfact", uicb_setmwfact},
    {"setnmaster", uicb_setnmaster},
    {"setncols", uicb_setncols},
    /* screen.c */
    {"focusnextscreen", uicb_focusnextscreen},
    {"focusprevscreen", uicb_focusprevscreen},
    {"movetoscreen", uicb_movetoscreen},
    /* awesome.c */
    {"quit", uicb_quit},
    /* statusbar.c */
    {"togglebar", uicb_togglebar},
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
            if(!a_strcmp(keyname, KeyModList[i].name))
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
            if(!a_strcmp(funcname, list[i].name))
                return list[i].func;

    return NULL;
}

/** Parse configuration file and initialize some stuff
 * \param disp Display ref
 * \param scr Screen number
 * \param drawcontext Draw context
 */
void
parse_config(Display * disp, int scr, DC * drawcontext, const char *confpatharg, awesome_config *awesomeconf)
{
    /* Main configuration object for parsing*/
    config_t awesomelibconf;
    config_setting_t *conftags;
    config_setting_t *conflayouts, *confsublayouts;
    config_setting_t *confrules, *confsubrules;
    config_setting_t *confkeys, *confsubkeys, *confkeysmasks, *confkeymaskelem;
    int i = 0, j = 0;
    double f = 0.0;
    const char *tmp, *homedir;
    char *confpath;
    KeySym tmp_key;
    ssize_t confpath_len;
    XColor colorbuf;

    if(confpatharg)
        confpath = a_strdup(confpatharg);
    else
    {
        homedir = getenv("HOME");
        confpath_len = a_strlen(homedir) + a_strlen(AWESOME_CONFIG_FILE) + 2;
        confpath = p_new(char, confpath_len);
        a_strcpy(confpath, confpath_len, homedir);
        a_strcat(confpath, confpath_len, "/");
        a_strcat(confpath, confpath_len, AWESOME_CONFIG_FILE);
    }

    config_init(&awesomelibconf);

    a_strcpy(awesomeconf->statustext, sizeof(awesomeconf->statustext), "awesome-" VERSION);

    /* store display */
    awesomeconf->display = disp;

    /* set screen */
    awesomeconf->screen = scr;
    awesomeconf->phys_screen = get_phys_screen(disp, scr);

    if(config_read_file(&awesomelibconf, confpath) == CONFIG_FALSE)
        fprintf(stderr, "awesome: error parsing configuration file at line %d: %s\n",
               config_error_line(&awesomelibconf), config_error_text(&awesomelibconf));


    /* font */
    tmp = config_lookup_string(&awesomelibconf, "awesome.font");
    drawcontext->font = XftFontOpenName(disp, awesomeconf->phys_screen, tmp ? tmp : "sans-12");

    if(!drawcontext->font)
        eprint("awesome: cannot init font\n");

    /* layouts */
    conflayouts = config_lookup(&awesomelibconf, "awesome.layouts");

    if(!conflayouts)
    {
        fprintf(stderr, "awesome: layouts not found in configuration file, setting default\n");
        awesomeconf->nlayouts = 2;
        awesomeconf->layouts = p_new(Layout, awesomeconf->nlayouts + 1);
        awesomeconf->layouts[0].symbol = a_strdup("[]=");
        awesomeconf->layouts[0].arrange = layout_tile;
        awesomeconf->layouts[1].symbol = a_strdup("<><");
        awesomeconf->layouts[1].arrange = layout_floating;
        awesomeconf->layouts[2].symbol = NULL;
        awesomeconf->layouts[2].arrange = NULL;
    }
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
            awesomeconf->layouts[i].symbol = a_strdup(config_setting_get_string_elem(confsublayouts, 0));
        }
        awesomeconf->layouts[i].symbol = NULL;
        awesomeconf->layouts[i].arrange = NULL;
    }

    awesomeconf->current_layout = awesomeconf->layouts;

    if(!awesomeconf->nlayouts || !awesomeconf->current_layout->arrange)
        eprint("awesome: fatal: no default layout available\n");

    for(i = 0; i < awesomeconf->nlayouts; i++)
    {
        j = drawcontext->font->height +
            textwidth(disp, drawcontext->font,
                      awesomeconf->layouts[i].symbol, a_strlen(awesomeconf->layouts[i].symbol));
        if(j > awesomeconf->statusbar.width)
            awesomeconf->statusbar.width = j;
    }

    /* tags */
    conftags = config_lookup(&awesomelibconf, "awesome.tags");

    if(!conftags)
    {
        fprintf(stderr, "awesome: tags not found in configuration file, setting default\n");
        awesomeconf->ntags = 3;
        awesomeconf->tags = p_new(Tag, awesomeconf->ntags);
        awesomeconf->tags[0].name = a_strdup("this");
        awesomeconf->tags[1].name = a_strdup("is");
        awesomeconf->tags[2].name = a_strdup("awesome");
        awesomeconf->tags[0].selected = True;
        awesomeconf->tags[1].selected = False;
        awesomeconf->tags[2].selected = False;
        awesomeconf->tags[0].was_selected = False;
        awesomeconf->tags[1].was_selected = False;
        awesomeconf->tags[2].was_selected = False;
        awesomeconf->tags[0].layout = awesomeconf->layouts;
        awesomeconf->tags[1].layout = awesomeconf->layouts;
        awesomeconf->tags[2].layout = awesomeconf->layouts;
    }
    else
    {
        awesomeconf->ntags = config_setting_length(conftags);
        awesomeconf->tags = p_new(Tag, awesomeconf->ntags);

        for(i = 0; (tmp = config_setting_get_string_elem(conftags, i)); i++)
        {
            awesomeconf->tags[i].name = a_strdup(tmp);
            awesomeconf->tags[i].selected = False;
            awesomeconf->tags[i].was_selected = False;
            /** \todo add support for default tag/layout in configuration file */
            awesomeconf->tags[i].layout = awesomeconf->layouts;
        }
    }

    if(!awesomeconf->ntags)
        eprint("awesome: fatal: no tags found in configuration file\n");

    /* select first tag by default */
    awesomeconf->tags[0].selected = True;
    awesomeconf->tags[0].was_selected = True;

    /* rules */
    confrules = config_lookup(&awesomelibconf, "awesome.rules");

    if(!confrules)
    {
        awesomeconf->nrules = 0;
        fprintf(stderr, "awesome: no rules found in configuration file\n");
    }
    else
    {
        awesomeconf->nrules = config_setting_length(confrules);
        awesomeconf->rules = p_new(Rule, awesomeconf->nrules);
        for(i = 0; (confsubrules = config_setting_get_elem(confrules, i)); i++)
        {
            awesomeconf->rules[i].prop = a_strdup(config_setting_get_string(config_setting_get_member(confsubrules, "name")));
            awesomeconf->rules[i].tags = a_strdup(config_setting_get_string(config_setting_get_member(confsubrules, "tags")));
            if(awesomeconf->rules[i].tags && !a_strlen(awesomeconf->rules[i].tags))
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
    {
        awesomeconf->nkeys = 0;
        fprintf(stderr, "awesome: no keys found in configuration file\n");
    }
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
            awesomeconf->keys[i].arg = a_strdup(config_setting_get_string_elem(confsubkeys, 3));
        }
    }

    /* barpos */
    tmp = config_lookup_string(&awesomelibconf, "awesome.barpos");

    if(tmp && !a_strncmp(tmp, "off", 6))
        awesomeconf->statusbar_default_position = BarOff;
    else if(tmp && !a_strncmp(tmp, "bottom", 6))
        awesomeconf->statusbar_default_position = BarBot;
    else
        awesomeconf->statusbar_default_position = BarTop;

    awesomeconf->statusbar.position = awesomeconf->statusbar_default_position;

    /* borderpx */
    awesomeconf->borderpx = config_lookup_int(&awesomelibconf, "awesome.borderpx");

    /* opacity */
    awesomeconf->opacity_unfocused = config_lookup_int(&awesomelibconf, "awesome.opacity_unfocused");
    if(awesomeconf->opacity_unfocused >= 100 || awesomeconf->opacity_unfocused == 0)
        awesomeconf->opacity_unfocused = -1;

    /* snap */
    i = config_lookup_int(&awesomelibconf, "awesome.snap");
    awesomeconf->snap = i ? i : 8;

    /* nmaster */
    i = config_lookup_int(&awesomelibconf, "awesome.nmaster");
    awesomeconf->nmaster = i ? i : 1;

    /* ncols */
    i = config_lookup_int(&awesomelibconf, "awesome.ncols");
    awesomeconf->ncols = i ? i : 1;

    /* mwfact */
    f = config_lookup_float(&awesomelibconf, "awesome.mwfact");
    awesomeconf->mwfact = f ? f : 0.6;

    /* resize_hints */
    awesomeconf->resize_hints = config_lookup_bool(&awesomelibconf, "awesome.resize_hints");

    /* focus_move_pointer */
    awesomeconf->focus_move_pointer = config_lookup_bool(&awesomelibconf, "awesome.focus_move_pointer");

    /* colors */
    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_border_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#dddddd");
    drawcontext->norm[ColBorder] = colorbuf.pixel;

    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_bg_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#000000");
    drawcontext->norm[ColBG] = colorbuf.pixel;

    tmp = config_lookup_string(&awesomelibconf, "awesome.normal_fg_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#ffffff");
    drawcontext->norm[ColFG] = colorbuf.pixel;
    drawcontext->text_normal = colorbuf;

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_border_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#008b8b");
    drawcontext->sel[ColBorder] = colorbuf.pixel;

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_bg_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#008b8b");
    drawcontext->sel[ColBG] = colorbuf.pixel;

    tmp = config_lookup_string(&awesomelibconf, "awesome.focus_fg_color");
    colorbuf = initxcolor(disp, awesomeconf->phys_screen, tmp ? tmp : "#ffffff");
    drawcontext->sel[ColFG] = colorbuf.pixel;
    drawcontext->text_selected = colorbuf;

    config_destroy(&awesomelibconf);
    p_delete(&confpath);
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
static XColor
initxcolor(Display *disp, int scr, const char *colstr)
{
    XColor color;
    if(!XAllocNamedColor(disp, DefaultColormap(disp, scr), colstr, &color, &color))
        die("awesome: error, cannot allocate color '%s'\n", colstr);
    return color;
}
