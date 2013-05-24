/*
 * config.c - configuration management
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

#include <errno.h>
#include <X11/keysym.h>

#include "config.h"
#include "statusbar.h"
#include "tag.h"
#include "rules.h"
#include "screen.h"
#include "widget.h"
#include "defconfig.h"
#include "layouts/tile.h"
#include "common/configopts.h"
#include "common/xutil.h"

/* Permit to use mouse with many more buttons */
#ifndef Button6
#define Button6 6
#endif
#ifndef Button7
#define Button7 7
#endif
#ifndef Button8
#define Button8 8
#endif
#ifndef Button9
#define Button9 9
#endif

extern AwesomeConf globalconf;
extern cfg_opt_t awesome_opts[];

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

extern const name_func_link_t UicbList[];
extern const name_func_link_t WidgetList[];
extern const name_func_link_t LayoutList[];
extern const name_func_link_t FloatingPlacementList[];

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

    return 0;
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
        {"6", Button6},
        {"7", Button7},
        {"8", Button8},
        {"9", Button9},
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
    int i, j;
    cfg_t *cfgsectmp;
    Button *b = NULL, *head = NULL;

    /* Mouse: layout click bindings */
    for(i = cfg_size(cfg, secname) - 1; i >= 0; i--)
    {
        b = p_new(Button, 1);

        cfgsectmp = cfg_getnsec(cfg, secname, i);
        for(j = cfg_size(cfgsectmp, "modkey") - 1; j >= 0; j--)
            b->mod |= key_mask_lookup(cfg_getnstr(cfgsectmp, "modkey", j));
        b->button = mouse_button_lookup(cfg_getstr(cfgsectmp, "button"));
        b->func = name_func_lookup(cfg_getstr(cfgsectmp, "command"), UicbList);
        if(!b->func)
            warn("unknown command %s\n", cfg_getstr(cfgsectmp, "command"));
        if(handle_arg)
            b->arg = a_strdup(cfg_getstr(cfgsectmp, "arg"));
        else
            b->arg = NULL;

        button_list_push(&head, b);
    }

    return head;
}


static void
set_key_info(Key *key, cfg_t *cfg)
{
    unsigned int j;

    for(j = 0; j < cfg_size(cfg, "modkey"); j++)
        key->mod |= key_mask_lookup(cfg_getnstr(cfg, "modkey", j));
    key->func = name_func_lookup(cfg_getstr(cfg, "command"), UicbList);
    if(!key->func)
        warn("unknown command %s\n", cfg_getstr(cfg, "command"));
}

static void
config_key_store(Key *key, char *str)
{
    KeyCode kc;
    int ikc;

    if(!a_strlen(str))
        return;
    else if(a_strncmp(str, "#", 1))
        key->keysym = XStringToKeysym(str);
    else
    {
        ikc = atoi(str + 1);
        memcpy(&kc, &ikc, sizeof(KeyCode));
        key->keycode = kc;
    }
}

static Key *
section_keys(cfg_t *cfg_keys)
{
    Key *key = NULL, *head = NULL;
    int i, j, numkeys;
    cfg_t *cfgkeytmp;

    for(i = cfg_size(cfg_keys, "key") - 1; i >= 0; i--)
    {
        key = p_new(Key, 1);
        cfgkeytmp = cfg_getnsec(cfg_keys, "key", i);
        set_key_info(key, cfgkeytmp);
        config_key_store(key, cfg_getstr(cfgkeytmp, "key"));
        key->arg = a_strdup(cfg_getstr(cfgkeytmp, "arg"));
        key_list_push(&head, key);
    }

    for(i = cfg_size(cfg_keys, "keylist") - 1; i >= 0; i--)
    {
        cfgkeytmp = cfg_getnsec(cfg_keys, "keylist", i);
        numkeys = cfg_size(cfgkeytmp, "keylist");
        if(numkeys != (int) cfg_size(cfgkeytmp, "arglist"))
        {
            warn("number of keys != number of args in keylist");
            continue;
        }

        for(j = 0; j < numkeys; j++)
        {
            key = p_new(Key, 1);
            set_key_info(key, cfgkeytmp);
            config_key_store(key, cfg_getnstr(cfgkeytmp, "keylist", j));
            key->arg = a_strdup(cfg_getnstr(cfgkeytmp, "arglist", j));
            key_list_push(&head, key);
        }
    }

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
statusbar_widgets_create(cfg_t *cfg_statusbar, Statusbar *statusbar)
{
    cfg_t* widgets, *wptr;
    Widget *widget = NULL;
    unsigned int i, j, numwidgets = 0;
    WidgetConstructor *widget_new;

    for(i = 0; WidgetList[i].name; i++)
        numwidgets += cfg_size(cfg_statusbar, WidgetList[i].name);

    wptr = widgets = p_new(cfg_t, numwidgets);

    for(i = 0; WidgetList[i].name; i++)
        for (j = 0; j < cfg_size(cfg_statusbar, WidgetList[i].name); j++)
        {
            memcpy(wptr,
                   cfg_getnsec(cfg_statusbar, WidgetList[i].name, j),
                   sizeof(cfg_t));
            wptr++;
        }

    qsort(widgets, numwidgets, sizeof(cfg_t), cmp_widget_cfg);

    for(i = 0; i < numwidgets; i++)
    {
        wptr = &widgets[i];
        widget_new = name_func_lookup(cfg_name(wptr), WidgetList);
        if(widget_new)
        {
            widget = widget_new(statusbar, wptr);
            widget_list_append(&statusbar->widgets, widget);
            widget->buttons = parse_mouse_bindings(wptr, "mouse",
                                                   a_strcmp(cfg_name(wptr), "taglist") ? True : False);
        }
        else
            warn("ignoring unknown widget: %s.\n", cfg_name(widgets + i));
    }
    p_delete(&widgets);
}

static void
config_section_titlebar_init(cfg_t *cfg_titlebar, Titlebar *tb, int screen)
{
    int phys_screen = screen_virttophys(screen);
    cfg_t *cfg_styles = cfg_getsec(cfg_titlebar, "styles");

    tb->position = tb->dposition = cfg_getposition(cfg_titlebar, "position");
    tb->align = cfg_getalignment(cfg_titlebar, "align");
    tb->text_align = cfg_getalignment(cfg_titlebar, "text_align");
    tb->width = cfg_getint(cfg_titlebar, "width");
    tb->height = cfg_getint(cfg_titlebar, "height");
    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "normal"),
                    &tb->styles.normal,
                    &globalconf.screens[screen].styles.normal);
    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "focus"),
                    &tb->styles.focus,
                    &globalconf.screens[screen].styles.focus);
    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "urgent"),
                    &tb->styles.urgent,
                    &globalconf.screens[screen].styles.urgent);
}

static void
config_parse_screen(cfg_t *cfg, int screen)
{
    char buf[2];
    const char *tmp;
    FloatingPlacement flpl;
    Layout *layout = NULL;
    Tag *tag = NULL;
    Statusbar *statusbar = NULL;
    cfg_t *cfg_general, *cfg_styles, *cfg_screen, *cfg_tags,
          *cfg_layouts, *cfg_padding, *cfgsectmp, *cfg_titlebar,
          *cfg_styles_normal, *cfg_styles_focus, *cfg_styles_urgent;
    VirtScreen *virtscreen = &globalconf.screens[screen];
    int i, phys_screen = screen_virttophys(screen);

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
    cfg_styles = cfg_getsec(cfg_screen, "styles");
    cfg_general = cfg_getsec(cfg_screen, "general");
    cfg_titlebar = cfg_getsec(cfg_screen, "titlebar");
    cfg_layouts = cfg_getsec(cfg_screen, "layouts");
    cfg_padding = cfg_getsec(cfg_screen, "padding");


    /* General section */
    virtscreen->borderpx = cfg_getint(cfg_general, "border");
    virtscreen->snap = cfg_getint(cfg_general, "snap");
    virtscreen->resize_hints = cfg_getbool(cfg_general, "resize_hints");
    virtscreen->sloppy_focus = cfg_getbool(cfg_general, "sloppy_focus");
    virtscreen->sloppy_focus_raise = cfg_getbool(cfg_general, "sloppy_focus_raise");
    virtscreen->new_become_master = cfg_getbool(cfg_general, "new_become_master");
    virtscreen->new_get_focus = cfg_getbool(cfg_general, "new_get_focus");
    virtscreen->opacity_unfocused = cfg_getfloat(cfg_general, "opacity_unfocused");
    virtscreen->opacity_focused = cfg_getfloat(cfg_general, "opacity_focused");
    virtscreen->floating_placement =
        name_func_lookup(cfg_getstr(cfg_general, "floating_placement"),
                                    FloatingPlacementList);
    virtscreen->mwfact_lower_limit = cfg_getfloat(cfg_general, "mwfact_lower_limit");
    virtscreen->mwfact_upper_limit = cfg_getfloat(cfg_general, "mwfact_upper_limit");

    if(virtscreen->mwfact_upper_limit < virtscreen->mwfact_lower_limit)
    {
        warn("mwfact_upper_limit must be greater than mwfact_lower_limit\n");
        virtscreen->mwfact_upper_limit = 0.9;
        virtscreen->mwfact_lower_limit = 0.1;
    }

    if(!virtscreen->floating_placement)
    {
        warn("unknown floating placement: %s\n", cfg_getstr(cfg_general, "floating_placement"));
        virtscreen->floating_placement = FloatingPlacementList[0].func;
    }

    /* Colors */
    if(!cfg_styles)
        eprint("no colors section found");

    if(!(cfg_styles_normal = cfg_getsec(cfg_styles, "normal")))
       eprint("no normal colors section found\n");
    if(!(cfg_styles_focus = cfg_getsec(cfg_styles, "focus")))
       eprint("no focus colors section found\n");
    if(!(cfg_styles_urgent = cfg_getsec(cfg_styles, "urgent")))
       eprint("no urgent colors section found\n");

    draw_style_init(globalconf.display, phys_screen,
                    cfg_styles_normal, &virtscreen->styles.normal, NULL);
    draw_style_init(globalconf.display, phys_screen,
                    cfg_styles_focus, &virtscreen->styles.focus, &virtscreen->styles.normal);
    draw_style_init(globalconf.display, phys_screen,
                    cfg_styles_urgent, &virtscreen->styles.urgent, &virtscreen->styles.normal);

    if(!virtscreen->styles.normal.font)
        eprint("no font available\n");

    /* Titlebar */
    config_section_titlebar_init(cfg_titlebar, &virtscreen->titlebar_default, screen);

    /* Statusbar */
    statusbar_list_init(&virtscreen->statusbar);
    for(i = cfg_size(cfg_screen, "statusbar") - 1; i >= 0; i--)
    {
        statusbar = p_new(Statusbar, 1);
        cfgsectmp = cfg_getnsec(cfg_screen, "statusbar", i);
        statusbar->position = statusbar->dposition =
            cfg_getposition(cfgsectmp, "position");
        statusbar->height = cfg_getint(cfgsectmp, "height");
        statusbar->width = cfg_getint(cfgsectmp, "width");
        statusbar->name = a_strdup(cfg_title(cfgsectmp));
        statusbar->screen = screen;
        statusbar_preinit(statusbar);
        statusbar_widgets_create(cfgsectmp, statusbar);
        statusbar_list_push(&virtscreen->statusbar, statusbar);
    }

    /* Layouts */
    layout_list_init(&virtscreen->layouts);
    if((i = cfg_size(cfg_layouts, "layout")))
        for(--i; i >= 0; i--)
        {
            layout = p_new(Layout, 1);
            cfgsectmp = cfg_getnsec(cfg_layouts, "layout", i);
            layout->arrange = name_func_lookup(cfg_title(cfgsectmp), LayoutList);
            if(!layout->arrange)
            {
                warn("unknown layout %s in configuration file\n", cfg_title(cfgsectmp));
                layout->image = NULL;
                continue;
            }
            layout->image = a_strdup(cfg_getstr(cfgsectmp, "image"));

            layout_list_push(&virtscreen->layouts, layout);
        }
    else
    {
        warn("no default layout available\n");
        layout = p_new(Layout, 1);
        layout->arrange = layout_tile;
        layout_list_push(&virtscreen->layouts, layout);
    }

    /* Tags */
    tag_list_init(&virtscreen->tags);
    if((i = cfg_size(cfg_tags, "tag")))
        for(--i; i >= 0; i--)
        {
            cfgsectmp = cfg_getnsec(cfg_tags, "tag", i);

            tmp = cfg_getstr(cfgsectmp, "layout");
            for(layout = virtscreen->layouts;
                layout && layout->arrange != name_func_lookup(tmp, LayoutList);
                layout = layout->next);
            if(!layout)
                layout = virtscreen->layouts;

            tag = tag_new(cfg_title(cfgsectmp),
                          layout,
                          cfg_getfloat(cfgsectmp, "mwfact"),
                          cfg_getint(cfgsectmp, "nmaster"),
                          cfg_getint(cfgsectmp, "ncol"));

            tag_push_to_screen(tag, screen);
        }
    else
    {
        warn("fatal: no tags found in configuration file\n");
        tag = tag_new("default", virtscreen->layouts, 0.618, 1, 1);
        tag_push_to_screen(tag, screen);
    }

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
    cfg_t *cfg, *cfg_rules, *cfg_keys, *cfg_mouse, *cfgsectmp;
    int ret, screen, i;
    char *confpath;
    Rule *rule = NULL;
    FILE *defconfig = NULL;

    if(confpatharg)
        confpath = a_strdup(confpatharg);
    else
        confpath = config_file();

    globalconf.configpath = a_strdup(confpath);

    cfg = cfg_new();

    ret = cfg_parse(cfg, confpath);

    switch(ret)
    {
      case CFG_FILE_ERROR:
        warn("parsing configuration file failed: %s\n", strerror(errno));
        if(!(defconfig = fopen(confpath, "w")))
           warn("unable to create default configuration file: %s\n", strerror(errno));
        break;
      case CFG_PARSE_ERROR:
        cfg_error(cfg, "W: awesome: parsing configuration file %s failed.\n", confpath);
        break;
    }

    if(ret != CFG_SUCCESS)
    {
        warn("using default compile-time configuration\n");
        cfg_free(cfg);
        cfg = cfg_init(awesome_opts, CFGF_NONE);
        cfg_parse_buf(cfg, AWESOME_DEFAULT_CONFIG);
    }

    /* get the right screen section */
    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        config_parse_screen(cfg, screen);

    /* get general sections */
    cfg_rules = cfg_getsec(cfg, "rules");
    cfg_keys = cfg_getsec(cfg, "keys");
    cfg_mouse = cfg_getsec(cfg, "mouse");

    /* Rules */
    rule_list_init(&globalconf.rules);
    for(i = cfg_size(cfg_rules, "rule") - 1; i >= 0; i--)
    {
        rule = p_new(Rule, 1);
        cfgsectmp = cfg_getnsec(cfg_rules, "rule", i);
        rule->prop_r = rules_compile_regex(cfg_getstr(cfgsectmp, "name"));
        rule->tags_r = rules_compile_regex(cfg_getstr(cfgsectmp, "tags"));
        rule->xprop = a_strdup(cfg_getstr(cfgsectmp, "xproperty_name"));
        rule->xpropval_r = rules_compile_regex(cfg_getstr(cfgsectmp, "xproperty_value"));
        rule->icon = a_strdup(cfg_getstr(cfgsectmp, "icon"));
        rule->isfloating = rules_get_fuzzy_from_str(cfg_getstr(cfgsectmp, "float"));
        rule->screen = cfg_getint(cfgsectmp, "screen");
        rule->ismaster = rules_get_fuzzy_from_str(cfg_getstr(cfgsectmp, "master"));
        rule->opacity = cfg_getfloat(cfgsectmp, "opacity");
        config_section_titlebar_init(cfg_getsec(cfgsectmp, "titlebar"), &rule->titlebar, 0);
        if(rule->screen >= globalconf.screens_info->nscreen)
            rule->screen = 0;

        rule_list_push(&globalconf.rules, rule);
    }

    /* Mouse: root window click bindings */
    globalconf.buttons.root = parse_mouse_bindings(cfg_mouse, "root", True);

    /* Mouse: client windows click bindings */
    globalconf.buttons.client = parse_mouse_bindings(cfg_mouse, "client", True);

    /* Mouse: titlebar windows click bindings */
    globalconf.buttons.titlebar = parse_mouse_bindings(cfg_mouse, "titlebar", True);

    /* Keys */
    globalconf.numlockmask = xgetnumlockmask(globalconf.display);

    globalconf.keys = section_keys(cfg_keys);

    if(defconfig)
    {
        fwrite(AWESOME_DEFAULT_CONFIG, a_strlen(AWESOME_DEFAULT_CONFIG), 1, defconfig);
        fclose(defconfig);
    }

    /* Free! Like a river! */
    cfg_free(cfg);
    p_delete(&confpath);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
