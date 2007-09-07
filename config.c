/* See LICENSE file for copyright and license details. */

/**
 * \defgroup ui_callback
 */

#include <stdlib.h>
#include <libconfig.h>
#include <X11/keysym.h>

#include "jdwm.h"
#include "util.h"
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
config_t jdwmlibconf;

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
static const KeyMod KeyModList[] = { {"Shift", ShiftMask},
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
static const NameFuncLink LayoutsList[] = { {"tile", tile},
{"tileleft", tileleft},
{"floating", floating},
{"grid", grid},
{"spiral", spiral},
{"dwindle", dwindle},
{"bstack", bstack},
{"bstackportrait", bstackportrait},
{NULL, NULL}
};

/** List of available UI bindable callbacks and functions */
static const NameFuncLink KeyfuncList[] = {
    /* util.c */
    {"spawn", spawn},
    /* client.c */
    {"killclient", uicb_killclient},
    {"moveresize", uicb_moveresize},
    {"settrans", uicb_settrans},
    /* config.c */
    {"reload", uicb_reload},
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
    {"incnmaster", uicb_incnmaster},
    /* jdwm.c */
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

/** \todo remove screen */
extern int screen;
/** \todo remove dc */
extern DC dc;

/** Reload configuration file
 * \param disp Display ref
 * \param arg unused
 * \ingroup ui_callback
 * \todo not really working nor safe I guess
 */
void
uicb_reload(Display *disp, jdwm_config *jdwmconf, const char *arg __attribute__ ((unused)))
{
    config_destroy(&jdwmlibconf);
    p_delete(&jdwmconf->rules);
    p_delete(&jdwmconf->tags);
    p_delete(&jdwmconf->selected_tags);
    p_delete(&jdwmconf->layouts);
    p_delete(&jdwmconf->tag_layouts);
    parse_config(disp, screen, &dc, jdwmconf);
}

/** Set default configuration
 * \param jdwmconf jdwm config ref
 */
static void
set_default_config(jdwm_config *jdwmconf)
{
    strcpy(jdwmconf->statustext, "jdwm-" VERSION);
    jdwmconf->statusbar.width = 0;
    jdwmconf->statusbar.height = 0;
}

/** Parse configuration file and initialize some stuff
 * \param disp Display ref
 * \param scr Screen number
 * \param drawcontext Draw context
 */
void
parse_config(Display * disp, int scr, DC * drawcontext, jdwm_config *jdwmconf)
{
    config_setting_t *conftags;
    config_setting_t *conflayouts, *confsublayouts;
    config_setting_t *confrules, *confsubrules;
    config_setting_t *confkeys, *confsubkeys, *confkeysmasks, *confkeymaskelem;
    int i, j;
    const char *tmp, *homedir;
    char *confpath;

    set_default_config(jdwmconf);

    homedir = getenv("HOME");
    confpath = p_new(char, strlen(homedir) + strlen(JDWM_CONFIG_FILE) + 2);
    strcpy(confpath, homedir);
    strcat(confpath, "/");
    strcat(confpath, JDWM_CONFIG_FILE);

    config_init(&jdwmlibconf);

    if(config_read_file(&jdwmlibconf, confpath) == CONFIG_FALSE)
        eprint("error parsing configuration file at line %d: %s\n",
               config_error_line(&jdwmlibconf), config_error_text(&jdwmlibconf));

    /* font */
    initfont(config_lookup_string(&jdwmlibconf, "jdwm.font"), disp, drawcontext);

    /* layouts */
    conflayouts = config_lookup(&jdwmlibconf, "jdwm.layouts");

    if(!conflayouts)
        eprint("layouts not found in configuration file\n");

    jdwmconf->nlayouts = config_setting_length(conflayouts);
    jdwmconf->layouts = p_new(Layout, jdwmconf->nlayouts + 1);
    for(i = 0; (confsublayouts = config_setting_get_elem(conflayouts, i)); i++)
    {
        jdwmconf->layouts[i].symbol = config_setting_get_string_elem(confsublayouts, 0);
        jdwmconf->layouts[i].arrange =
            name_func_lookup(config_setting_get_string_elem(confsublayouts, 1), LayoutsList);
        if(!jdwmconf->layouts[i].arrange)
            eprint("unknown layout in configuration file\n");

        j = textw(jdwmconf->layouts[i].symbol);
        if(j > jdwmconf->statusbar.width)
            jdwmconf->statusbar.width = j;
    }

    jdwmconf->layouts[i].symbol = NULL;
    jdwmconf->layouts[i].arrange = NULL;

    /** \todo put this in set_default_layout */
    jdwmconf->current_layout = jdwmconf->layouts;
    /* tags */
    conftags = config_lookup(&jdwmlibconf, "jdwm.tags");

    if(!conftags)
        eprint("tags not found in configuration file\n");

    jdwmconf->ntags = config_setting_length(conftags);
    jdwmconf->tags = p_new(const char *, jdwmconf->ntags);
    jdwmconf->selected_tags = p_new(Bool, jdwmconf->ntags);
    jdwmconf->prev_selected_tags = p_new(Bool, jdwmconf->ntags);
    jdwmconf->tag_layouts = p_new(Layout *, jdwmconf->ntags);

    for(i = 0; (tmp = config_setting_get_string_elem(conftags, i)); i++)
    {
        jdwmconf->tags[i] = tmp;
        jdwmconf->selected_tags[i] = False;
        jdwmconf->prev_selected_tags[i] = False;
        /** \todo add support for default tag/layout in configuration file */
        jdwmconf->tag_layouts[i] = jdwmconf->layouts;
    }

    /* select first tag by default */
    jdwmconf->selected_tags[0] = True;
    jdwmconf->prev_selected_tags[0] = True;

    /* rules */
    confrules = config_lookup(&jdwmlibconf, "jdwm.rules");

    if(!confrules)
        eprint("rules not found in configuration file\n");

    jdwmconf->nrules = config_setting_length(confrules);
    jdwmconf->rules = p_new(Rule, jdwmconf->nrules);
    for(i = 0; (confsubrules = config_setting_get_elem(confrules, i)); i++)
    {
        jdwmconf->rules[i].prop = config_setting_get_string(config_setting_get_member(confsubrules, "name"));
        jdwmconf->rules[i].tags = config_setting_get_string(config_setting_get_member(confsubrules, "tags"));
        if(jdwmconf->rules[i].tags && !strlen(jdwmconf->rules[i].tags))
            jdwmconf->rules[i].tags = NULL;
        jdwmconf->rules[i].isfloating =
            config_setting_get_bool(config_setting_get_member(confsubrules, "float"));
    }

    /* modkey */
    jdwmconf->modkey = key_mask_lookup(config_lookup_string(&jdwmlibconf, "jdwm.modkey"));

    /* find numlock mask */
    jdwmconf->numlockmask = get_numlockmask(disp);

    /* keys */
    confkeys = config_lookup(&jdwmlibconf, "jdwm.keys");

    if(!confkeys)
        eprint("keys not found in configuration file\n");

    jdwmconf->nkeys = config_setting_length(confkeys);
    jdwmconf->keys = p_new(Key, jdwmconf->nkeys);

    for(i = 0; (confsubkeys = config_setting_get_elem(confkeys, i)); i++)
    {
        confkeysmasks = config_setting_get_elem(confsubkeys, 0);
        for(j = 0; (confkeymaskelem = config_setting_get_elem(confkeysmasks, j)); j++)
            jdwmconf->keys[i].mod |= key_mask_lookup(config_setting_get_string(confkeymaskelem));
        jdwmconf->keys[i].keysym = XStringToKeysym(config_setting_get_string_elem(confsubkeys, 1));
        jdwmconf->keys[i].func =
            name_func_lookup(config_setting_get_string_elem(confsubkeys, 2), KeyfuncList);
        jdwmconf->keys[i].arg = config_setting_get_string_elem(confsubkeys, 3);
    }

    /* barpos */
    tmp = config_lookup_string(&jdwmlibconf, "jdwm.barpos");

    if(!strncmp(tmp, "BarTop", 6))
        jdwmconf->bpos = BarTop;
    else if(!strncmp(tmp, "BarBot", 6))
        jdwmconf->bpos = BarBot;
    else if(!strncmp(tmp, "BarOff", 6))
        jdwmconf->bpos = BarOff;

    jdwmconf->current_bpos = jdwmconf->bpos;

    /* borderpx */
    jdwmconf->borderpx = config_lookup_int(&jdwmlibconf, "jdwm.borderpx");

    /* opacity */
    jdwmconf->opacity_unfocused = config_lookup_int(&jdwmlibconf, "jdwm.opacity_unfocused");
    if(jdwmconf->opacity_unfocused >= 100)
        jdwmconf->opacity_unfocused = -1;

    /* snap */
    jdwmconf->snap = config_lookup_int(&jdwmlibconf, "jdwm.snap");

    /* nmaster */
    jdwmconf->nmaster = config_lookup_int(&jdwmlibconf, "jdwm.nmaster");

    /* mwfact */
    jdwmconf->mwfact = config_lookup_float(&jdwmlibconf, "jdwm.mwfact");

    /* colors */
    dc.norm[ColBorder] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.normal_border_color"),
                                   disp, scr);
    dc.norm[ColBG] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.normal_bg_color"), disp, scr);
    dc.norm[ColFG] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.normal_fg_color"), disp, scr);
    dc.sel[ColBorder] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.focus_border_color"),
                                  disp, scr);
    dc.sel[ColBG] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.focus_bg_color"), disp, scr);
    dc.sel[ColFG] = initcolor(config_lookup_string(&jdwmlibconf, "jdwm.focus_fg_color"), disp, scr);

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
            fprintf(stderr, "jdwm: missing fontset: %s\n", missing[n]);
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
