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
 * @defgroup ui_callback User Interface Callbacks
 */

#include <X11/keysym.h>

#include "statusbar.h"
#include "util.h"
#include "rules.h"
#include "screen.h"
#include "widget.h"
#include "xutil.h"
#include "ewmh.h"
#include "defconfig.h"
#include "layouts/tile.h"

#define AWESOME_CONFIG_FILE ".awesomerc" 

extern AwesomeConf globalconf;

/** Link a name to a key symbol */
typedef struct
{
    const char *name;
    KeySym keysym;
} KeyMod;

/** Link a name to a mouse button symbol */
typedef struct
{
    const char *name;
    unsigned int button;
} MouseButton;

extern const NameFuncLink UicbList[];
extern const NameFuncLink WidgetList[];
extern const NameFuncLink LayoutsList[];


static unsigned int
get_numlockmask(Display *disp)
{
    XModifierKeymap *modmap;
    unsigned int mask = 0;
    int i, j;

    modmap = XGetModifierMapping(disp);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(disp, XK_Num_Lock))
                mask = (1 << i);

    XFreeModifiermap(modmap);

    return mask;
}

/** Lookup for a key mask from its name
 * \param keyname Key name
 * \return Key mask or 0 if not found
 */
static KeySym
key_mask_lookup(const char *keyname)
{
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
        {NULL, NoSymbol}
    };
    int i;

    if(keyname)
        for(i = 0; KeyModList[i].name; i++)
            if(!a_strcmp(keyname, KeyModList[i].name))
                return KeyModList[i].keysym;

    return NoSymbol;
}

/** Lookup for a mouse button from its name
 * \param button Mouse button name
 * \return Mouse button or 0 if not found
 */
static unsigned int
mouse_button_lookup(const char *button)
{
    /** List of button name and corresponding X11 mask codes */
    static const MouseButton MouseButtonList[] =
    {
        {"1", Button1},
        {"2", Button2},
        {"3", Button3},
        {"4", Button4},
        {"5", Button5},
        {NULL, 0}
    };
    int i;
    
    if(button)
        for(i = 0; MouseButtonList[i].name; i++)
            if(!a_strcmp(button, MouseButtonList[i].name))
                return MouseButtonList[i].button;

    return 0;
}

static Button *
parse_mouse_bindings(cfg_t * cfg, const char *secname, Bool handle_arg)
{
    unsigned int i, j;
    cfg_t *cfgsectmp;
    Button *b = NULL, *head = NULL;

    /* Mouse: layout click bindings */
    for(i = 0; i < cfg_size(cfg, secname); i++)
    {
        /* init first elem */
        if(i == 0)
            head = b = p_new(Button, 1);

        cfgsectmp = cfg_getnsec(cfg, secname, i);
        for(j = 0; j < cfg_size(cfgsectmp, "modkey"); j++)
            b->mod |= key_mask_lookup(cfg_getnstr(cfgsectmp, "modkey", j));
        b->button = mouse_button_lookup(cfg_getstr(cfgsectmp, "button"));
        b->func = name_func_lookup(cfg_getstr(cfgsectmp, "command"), UicbList);
        if(!b->func)
            warn("unknown command %s\n", cfg_getstr(cfgsectmp, "command"));
        if(handle_arg)
            b->arg = a_strdup(cfg_getstr(cfgsectmp, "arg"));
        else
            b->arg = NULL;

        /* switch to next elem or finalize the list */
        if(i < cfg_size(cfg, secname) - 1)
        {
            b->next = p_new(Button, 1);
            b = b->next;
        }
        else
            b->next = NULL;
    }

    return head;
}


static void set_key_info(Key *key, cfg_t *cfg)
{
    unsigned int j;

    for(j = 0; j < cfg_size(cfg, "modkey"); j++)
        key->mod |= key_mask_lookup(cfg_getnstr(cfg, "modkey", j));
    key->func = name_func_lookup(cfg_getstr(cfg, "command"), UicbList);
    if(!key->func)
        warn("unknown command %s\n", cfg_getstr(cfg, "command"));
}


static Key *section_keys(cfg_t *cfg_keys)
{
    Key *key, *head;
    unsigned int i, j, numkeys;
    cfg_t *cfgkeytmp;

    head = key = NULL;
    for(i = 0; i < cfg_size(cfg_keys, "key"); i++)
    {
        if (i == 0)
            key = head = p_new(Key, 1);
        else
        {
            key->next = p_new(Key, 1);
            key = key->next;
        }
        cfgkeytmp = cfg_getnsec(cfg_keys, "key", i);
        set_key_info(key, cfgkeytmp);
        key->keysym = XStringToKeysym(cfg_getstr(cfgkeytmp, "key"));
        key->arg = a_strdup(cfg_getstr(cfgkeytmp, "arg"));
    }

    for(i = 0; i < cfg_size(cfg_keys, "keylist"); i++)
    {
        cfgkeytmp = cfg_getnsec(cfg_keys, "keylist", i);
        numkeys = cfg_size(cfgkeytmp, "keylist");
        if (numkeys != cfg_size(cfgkeytmp, "arglist"))
        {
            warn("number of keys != number of args in keylist");
            continue;
        }
        for(j=0; j < numkeys; j++)
        {
            if (head == NULL)
            {
                key = p_new(Key, 1);
                head = key;
            }
            else
            {
                key->next = p_new(Key, 1);
                key = key->next;
            }
            set_key_info(key, cfgkeytmp);
            key->keysym = XStringToKeysym(cfg_getnstr(cfgkeytmp, "keylist", j));
            key->arg = a_strdup(cfg_getnstr(cfgkeytmp, "arglist", j));
            if(j < numkeys - 1)
            {
                key->next = p_new(Key, 1);
                key = key->next;
            }
        }
    }
    if (key)
        key->next = NULL;
    return head;
}


static int
cmp_widget_cfg(const void *a, const void *b)
{
    if (((cfg_t*)a)->line < ((cfg_t*)b)->line)
        return -1;

    if (((cfg_t*)a)->line > ((cfg_t*)b)->line)
        return 1;

    return 0;
}


static void
create_widgets(cfg_t* cfg_statusbar, Statusbar *statusbar)
{
    cfg_t* widgets, *wptr;
    Widget *widget = NULL;
    unsigned int i, j, numwidgets = 0;
    WidgetConstructor *widget_new;

    for(i = 0; WidgetList[i].name; i++)
        numwidgets += cfg_size(cfg_statusbar, WidgetList[i].name);

    widgets = p_new(cfg_t, numwidgets);
    wptr = widgets;

    for(i = 0; WidgetList[i].name; i++)
        for (j = 0; j < cfg_size(cfg_statusbar, WidgetList[i].name); j++)
        {
            memcpy(wptr,
                   cfg_getnsec(cfg_statusbar, WidgetList[i].name, j),
                   sizeof(cfg_t));
            wptr++;
        }

    qsort(widgets, numwidgets, sizeof(cfg_t), cmp_widget_cfg);

    for (i = 0; i < numwidgets; i++)
    {
        wptr = widgets + i;
        widget_new = name_func_lookup(cfg_name(wptr), WidgetList);
        if(widget_new)
        {
            if(!widget)
                statusbar->widgets = widget = widget_new(statusbar, wptr);
            else
            {
                widget->next = widget_new(statusbar, wptr);
                widget = widget->next;
            }
            widget->buttons = parse_mouse_bindings(wptr, "mouse", a_strcmp(cfg_name(wptr), "taglist"));
        }
        else
            warn("Ignoring unknown widget: %s.\n", cfg_name(widgets + i));
    }
}



static void
config_parse_screen(cfg_t *cfg, int screen)
{
    char buf[2];
    const char *tmp;
    Layout *layout = NULL;
    Tag *tag = NULL;
    Statusbar *statusbar = NULL;
    cfg_t *cfg_general, *cfg_colors, *cfg_screen, *cfg_tags,
          *cfg_layouts, *cfg_padding, *cfgsectmp;
    VirtScreen *virtscreen = &globalconf.screens[screen];
    unsigned int i;

    snprintf(buf, sizeof(buf), "%d", screen);
    cfg_screen = cfg_gettsec(cfg, "screen", buf);
    if(!cfg_screen)
        cfg_screen = cfg_getsec(cfg, "screen");

    if(!cfg_screen)
    {
        warn("parsing configuration file failed, no screen section found\n");
        cfg_parse_buf(cfg, AWESOME_DEFAULT_CONFIG);
        cfg_screen = cfg_getsec(cfg, "screen");
    }

    /* get screen specific sections */
    cfg_tags = cfg_getsec(cfg_screen, "tags");
    cfg_colors = cfg_getsec(cfg_screen, "colors");
    cfg_general = cfg_getsec(cfg_screen, "general");
    cfg_layouts = cfg_getsec(cfg_screen, "layouts");
    cfg_padding = cfg_getsec(cfg_screen, "padding");


    /* General section */
    virtscreen->borderpx = cfg_getint(cfg_general, "border");
    virtscreen->snap = cfg_getint(cfg_general, "snap");
    virtscreen->resize_hints = cfg_getbool(cfg_general, "resize_hints");
    virtscreen->opacity_unfocused = cfg_getint(cfg_general, "opacity_unfocused");
    virtscreen->focus_move_pointer = cfg_getbool(cfg_general, "focus_move_pointer");
    virtscreen->allow_lower_floats = cfg_getbool(cfg_general, "allow_lower_floats");
    virtscreen->font = XftFontOpenName(globalconf.display,
                                       get_phys_screen(screen),
                                       cfg_getstr(cfg_general, "font"));
    if(!virtscreen->font)
        eprint("awesome: cannot init font\n");

    /* Colors */
    virtscreen->colors_normal[ColBorder] = initxcolor(screen,
                                                      cfg_getstr(cfg_colors, "normal_border"));
    virtscreen->colors_normal[ColBG] = initxcolor(screen,
                                                  cfg_getstr(cfg_colors, "normal_bg"));
    virtscreen->colors_normal[ColFG] = initxcolor(screen,
                                                  cfg_getstr(cfg_colors, "normal_fg"));
    virtscreen->colors_selected[ColBorder] = initxcolor(screen,
                                                        cfg_getstr(cfg_colors, "focus_border"));
    virtscreen->colors_selected[ColBG] = initxcolor(screen,
                                                    cfg_getstr(cfg_colors, "focus_bg"));
    virtscreen->colors_selected[ColFG] = initxcolor(screen,
                                                    cfg_getstr(cfg_colors, "focus_fg"));
    virtscreen->colors_urgent[ColBG] = initxcolor(screen,
                                                  cfg_getstr(cfg_colors, "urgent_bg"));
    virtscreen->colors_urgent[ColFG] = initxcolor(screen,
                                                  cfg_getstr(cfg_colors, "urgent_fg"));

    /* Statusbar */

    if(cfg_size(cfg_screen, "statusbar"))
    {
        virtscreen->statusbar = statusbar = p_new(Statusbar, 1);
        for(i = 0; i < cfg_size(cfg_screen, "statusbar"); i++)
        {
            cfgsectmp = cfg_getnsec(cfg_screen, "statusbar", i);
            statusbar->position = statusbar->dposition =
                statusbar_get_position_from_str(cfg_getstr(cfgsectmp, "position"));
            statusbar->height = cfg_getint(cfgsectmp, "height");
            statusbar->width = cfg_getint(cfgsectmp, "width");
            statusbar->name = a_strdup(cfg_title(cfgsectmp));
            create_widgets(cfgsectmp, statusbar);

            if(i < cfg_size(cfg_screen, "statusbar") - 1)
                statusbar = statusbar->next = p_new(Statusbar, 1);
        }
    }

    /* Layouts */
    virtscreen->layouts = layout = p_new(Layout, 1);
    if(cfg_size(cfg_layouts, "layout"))
        for(i = 0; i < cfg_size(cfg_layouts, "layout"); i++)
        {
            cfgsectmp = cfg_getnsec(cfg_layouts, "layout", i);
            layout->arrange = name_func_lookup(cfg_title(cfgsectmp), LayoutsList);
            if(!layout->arrange)
            {
                warn("unknown layout %s in configuration file\n", cfg_title(cfgsectmp));
                layout->image = NULL;
                continue;
            }
            layout->image = a_strdup(cfg_getstr(cfgsectmp, "image"));

            if(i < cfg_size(cfg_layouts, "layout") - 1)
                layout = layout->next = p_new(Layout, 1);
        }
    else
    {
        warn("no default layout available\n");
        layout->arrange = layout_tile;
    }

    /* Tags */
    virtscreen->tags = tag = p_new(Tag, 1);
    if(cfg_size(cfg_tags, "tag"))
        for(i = 0; i < cfg_size(cfg_tags, "tag"); i++)
        {
            cfgsectmp = cfg_getnsec(cfg_tags, "tag", i);
            tag->name = a_strdup(cfg_title(cfgsectmp));
            tag->selected = False;
            tag->was_selected = False;
            tmp = cfg_getstr(cfgsectmp, "layout");
            for(layout = virtscreen->layouts;
                layout && layout->arrange != name_func_lookup(tmp, LayoutsList); layout = layout->next);
            if(!layout)
                tag->layout = virtscreen->layouts;
            else
                tag->layout = layout;
            tag->mwfact = cfg_getfloat(cfgsectmp, "mwfact");
            tag->nmaster = cfg_getint(cfgsectmp, "nmaster");
            tag->ncol = cfg_getint(cfgsectmp, "ncol");

            if(i < cfg_size(cfg_tags, "tag") - 1)
                tag = tag->next = p_new(Tag, 1);
            else
                tag->next = NULL;
        }
    else
    {
        warn("fatal: no tags found in configuration file\n");
        tag->name = a_strdup("default");
        tag->layout = virtscreen->layouts;
        tag->nmaster = 1;
        tag->ncol = 1;
        tag->mwfact = 0.5;
    }

    ewmh_update_net_numbers_of_desktop(get_phys_screen(screen));
    ewmh_update_net_current_desktop(get_phys_screen(screen));
    ewmh_update_net_desktop_names(get_phys_screen(screen));

    /* select first tag by default */
    virtscreen->tags[0].selected = True;
    virtscreen->tags[0].was_selected = True;

    /* padding */
    virtscreen->padding.top = cfg_getint(cfg_padding, "top");
    virtscreen->padding.bottom = cfg_getint(cfg_padding, "bottom");
    virtscreen->padding.left = cfg_getint(cfg_padding, "left");
    virtscreen->padding.right = cfg_getint(cfg_padding, "right");
}

/** Parse configuration file and initialize some stuff
 * \param confpatharg Path to configuration file
 */
void
config_parse(const char *confpatharg)
{
    static cfg_opt_t general_opts[] =
    {
        CFG_INT((char *) "border", 1, CFGF_NONE),
        CFG_INT((char *) "snap", 8, CFGF_NONE),
        CFG_BOOL((char *) "resize_hints", cfg_true, CFGF_NONE),
        CFG_INT((char *) "opacity_unfocused", 100, CFGF_NONE),
        CFG_BOOL((char *) "focus_move_pointer", cfg_false, CFGF_NONE),
        CFG_BOOL((char *) "allow_lower_floats", cfg_false, CFGF_NONE),
        CFG_STR((char *) "font", (char *) "mono-12", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t colors_opts[] =
    {
        CFG_STR((char *) "normal_border", (char *) "#111111", CFGF_NONE),
        CFG_STR((char *) "normal_bg", (char *) "#111111", CFGF_NONE),
        CFG_STR((char *) "normal_fg", (char *) "#eeeeee", CFGF_NONE),
        CFG_STR((char *) "focus_border", (char *) "#6666ff", CFGF_NONE),
        CFG_STR((char *) "focus_bg", (char *) "#6666ff", CFGF_NONE),
        CFG_STR((char *) "focus_fg", (char *) "#ffffff", CFGF_NONE),
        CFG_STR((char *) "urgent_bg", (char *) "#ff0000", CFGF_NONE),
        CFG_STR((char *) "urgent_fg", (char *) "#ffffff", CFGF_NONE),
        CFG_STR((char *) "tab_border", (char *) "#ff0000", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t mouse_taglist_opts[] =
    {
        CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
        CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
        CFG_STR((char *) "command", (char *) "", CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t mouse_generic_opts[] =
    {
        CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
        CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
        CFG_STR((char *) "command", (char *) "", CFGF_NONE),
        CFG_STR((char *) "arg", NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t widget_opts[] =
    {
        CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t widget_taglist_opts[] =
    {
        CFG_SEC((char *) "mouse", mouse_taglist_opts, CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t widget_iconbox_opts[] =
    {
        CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
        CFG_STR((char *) "image", (char *) NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t widget_textbox_focus_opts[] =
    {
        CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
        CFG_INT((char *) "width", 0, CFGF_NONE),
        CFG_STR((char *) "text", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "fg", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "bg", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "font", (char *) NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t widget_progressbar_bar_opts[] =
    {
        CFG_STR((char *) "fg", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "bg", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "bcolor", (char *) NULL, CFGF_NONE),
    };
    static cfg_opt_t widget_progressbar_opts[] =
    {
        CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
        CFG_SEC((char *) "bar", widget_progressbar_bar_opts, CFGF_MULTI),
        CFG_INT((char *) "width", 100, CFGF_NONE),
        CFG_INT((char *) "gap", 2, CFGF_NONE),
        CFG_INT((char *) "lpadding", 3, CFGF_NONE),
        CFG_FLOAT((char *) "height", 0.67, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t statusbar_opts[] =
    {
        CFG_STR((char *) "position", (char *) "top", CFGF_NONE),
        CFG_INT((char *) "height", 0, CFGF_NONE),
        CFG_INT((char *) "width", 0, CFGF_NONE),
        CFG_SEC((char *) "textbox", widget_textbox_focus_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "taglist", widget_taglist_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "focustitle", widget_textbox_focus_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "layoutinfo", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "iconbox", widget_iconbox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "netwmicon", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "progressbar", widget_progressbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_END()
    };
    static cfg_opt_t tag_opts[] =
    {
        CFG_STR((char *) "layout", (char *) "tile", CFGF_NONE),
        CFG_FLOAT((char *) "mwfact", 0.5, CFGF_NONE),
        CFG_INT((char *) "nmaster", 1, CFGF_NONE),
        CFG_INT((char *) "ncol", 1, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t tags_opts[] =
    {
        CFG_SEC((char *) "tag", tag_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_END()
    };
    static cfg_opt_t layout_opts[] =
    {
        CFG_STR((char *) "image", NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t layouts_opts[] =
    {
        CFG_SEC((char *) "layout", layout_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t padding_opts[] =
    {
        CFG_INT((char *) "top", 0, CFGF_NONE),
        CFG_INT((char *) "bottom", 0, CFGF_NONE),
        CFG_INT((char *) "right", 0, CFGF_NONE),
        CFG_INT((char *) "left", 0, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t screen_opts[] =
    {
        CFG_SEC((char *) "general", general_opts, CFGF_NONE),
        CFG_SEC((char *) "statusbar", statusbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "tags", tags_opts, CFGF_NONE),
        CFG_SEC((char *) "colors", colors_opts, CFGF_NONE),
        CFG_SEC((char *) "layouts", layouts_opts, CFGF_NONE),
        CFG_SEC((char *) "padding", padding_opts, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t rule_opts[] =
    {
        CFG_STR((char *) "name", (char *) "", CFGF_NONE),
        CFG_STR((char *) "tags", (char *) "", CFGF_NONE),
        CFG_STR((char *) "icon", (char *) "", CFGF_NONE),
        CFG_BOOL((char *) "float", cfg_false, CFGF_NONE),
        CFG_INT((char *) "screen", RULE_NOSCREEN, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t rules_opts[] =
    {
        CFG_SEC((char *) "rule", rule_opts, CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t key_opts[] =
    {
        CFG_STR_LIST((char *) "modkey", (char *) "{Mod4}", CFGF_NONE),
        CFG_STR((char *) "key", (char *) "None", CFGF_NONE),
        CFG_STR((char *) "command", (char *) "", CFGF_NONE),
        CFG_STR((char *) "arg", NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t keylist_opts[] =
    {
        CFG_STR_LIST((char *) "modkey", (char *) "{Mod4}", CFGF_NONE),
        CFG_STR_LIST((char *) "keylist", (char *) NULL, CFGF_NONE),
        CFG_STR((char *) "command", (char *) "", CFGF_NONE),
        CFG_STR_LIST((char *) "arglist", NULL, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t keys_opts[] =
    {
        CFG_SEC((char *) "key", key_opts, CFGF_MULTI),
        CFG_SEC((char *) "keylist", keylist_opts, CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t mouse_opts[] =
    {
        CFG_SEC((char *) "root", mouse_generic_opts, CFGF_MULTI),
        CFG_SEC((char *) "client", mouse_generic_opts, CFGF_MULTI),
        CFG_END()
    };
    static cfg_opt_t opts[] =
    {
        CFG_SEC((char *) "screen", screen_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
        CFG_SEC((char *) "rules", rules_opts, CFGF_NONE),
        CFG_SEC((char *) "keys", keys_opts, CFGF_NONE),
        CFG_SEC((char *) "mouse", mouse_opts, CFGF_NONE),
        CFG_END()
    };
    cfg_t *cfg, *cfg_rules, *cfg_keys, *cfg_mouse, *cfgsectmp;
    int ret, screen;
    unsigned int i = 0;
    const char *homedir;
    char *confpath;
    ssize_t confpath_len;
    Rule *rule = NULL;
    FILE *defconfig = NULL;

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

    globalconf.configpath = a_strdup(confpath);

    cfg = cfg_init(opts, CFGF_NONE);

    ret = cfg_parse(cfg, confpath);
    if(ret == CFG_FILE_ERROR)
    {
        perror("awesome: parsing configuration file failed");
        cfg_parse_buf(cfg, AWESOME_DEFAULT_CONFIG);
        if(!(defconfig = fopen(confpath, "w")))
           perror("awesome: unable to create default configuration file");
    }
    else if(ret == CFG_PARSE_ERROR)
        cfg_error(cfg, "awesome: parsing configuration file %s failed.\n", confpath);

    /* get the right screen section */
    for(screen = 0; screen < get_screen_count(); screen++)
        config_parse_screen(cfg, screen);

    /* get general sections */
    cfg_rules = cfg_getsec(cfg, "rules");
    cfg_keys = cfg_getsec(cfg, "keys");
    cfg_mouse = cfg_getsec(cfg, "mouse");

    /* Rules */
    if(cfg_size(cfg_rules, "rule"))
    {
        globalconf.rules = rule = p_new(Rule, 1);
        for(i = 0; i < cfg_size(cfg_rules, "rule"); i++)
        {
            cfgsectmp = cfg_getnsec(cfg_rules, "rule", i);
            rule->prop = a_strdup(cfg_getstr(cfgsectmp, "name"));
            rule->tags = a_strdup(cfg_getstr(cfgsectmp, "tags"));
            rule->icon = a_strdup(cfg_getstr(cfgsectmp, "icon"));
            rule->isfloating = cfg_getbool(cfgsectmp, "float");
            rule->screen = cfg_getint(cfgsectmp, "screen");
            if(rule->screen >= get_screen_count())
                rule->screen = 0;

            if(i < cfg_size(cfg_rules, "rule") - 1)
                rule = rule->next = p_new(Rule, 1);
            else
                rule->next = NULL;
        }
    }
    else
        globalconf.rules = NULL;

    compileregs(globalconf.rules);

    /* Mouse: root window click bindings */
    globalconf.buttons.root = parse_mouse_bindings(cfg_mouse, "root", True);

    /* Mouse: client windows click bindings */
    globalconf.buttons.client = parse_mouse_bindings(cfg_mouse, "client", True);

    /* Keys */
    globalconf.numlockmask = get_numlockmask(globalconf.display);

    globalconf.keys = section_keys(cfg_keys);

    if(defconfig)
    {
        cfg_print(cfg, defconfig);
        fclose(defconfig);
    }

    /* Free! Like a river! */
    cfg_free(cfg);
    p_delete(&confpath);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
