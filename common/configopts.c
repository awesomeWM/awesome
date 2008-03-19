/*
 * configopts.c - configuration options
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <confuse.h>

#include "rules.h"
#include "common/configopts.h"

#define AWESOME_CONFIG_FILE ".awesomerc"

#define CFG_AWESOME_END() \
        CFG_FUNC((char *) "include", cfg_awesome_include), \
        CFG_END()

#define CFG_POSITION(name, value, flags) \
    CFG_PTR_CB(name, value, flags, \
               cfg_position_parse, cfg_value_free)

/** This is a better writing of cfg_include coming from libconfuse.
 * With this one, we do not treat errors as fatal.
 */
static int
cfg_awesome_include(cfg_t *cfg, cfg_opt_t *opt,
                    int argc, const char **argv)
{
    char *filename;
    FILE *fp;

    if(argc != 1 || !a_strlen(argv[0]))
    {
        cfg_error(cfg, "wrong number of arguments to cfg_awesome_include()");
        return 0;
    }

    filename = cfg_tilde_expand(argv[0]);

    if(!(fp = fopen(filename, "r")))
    {
        cfg_error(cfg, "cannot include configuration file %s: %s",
                  filename, strerror(errno));
        return 0;
    }

    p_delete(&filename);
    fclose(fp);

    return cfg_include(cfg, opt, argc, argv);
}

static int
cfg_position_parse(cfg_t *cfg, cfg_opt_t *opt,
                   const char *value, void *result)
{
    Position *p = p_new(Position, 1);
    
    if((*p = position_get_from_str(value)) == Off
       && a_strcmp(value, "off"))
    {
        cfg_error(cfg,
                  "position option '%s' must be top, bottom, right, left, auto or off in section '%s'",
                  opt->name, cfg->name);
        p_delete(&p);
        return -1;
    }
    *(void **) result = p;
    return 0;
}

static void
cfg_value_free(void *value)
{
    p_delete(&value);
}

cfg_opt_t titlebar_opts[] =
{
    CFG_POSITION((char *) "position", (char *) "auto", CFGF_NONE),
    CFG_STR((char *) "text_align", (char *) "center", CFGF_NONE),
    CFG_STR((char *) "icon", (char *) "left", CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t general_opts[] =
{
    CFG_INT((char *) "border", 1, CFGF_NONE),
    CFG_INT((char *) "snap", 8, CFGF_NONE),
    CFG_BOOL((char *) "resize_hints", cfg_true, CFGF_NONE),
    CFG_BOOL((char *) "sloppy_focus", cfg_true, CFGF_NONE),
    CFG_BOOL((char *) "sloppy_focus_raise", cfg_false, CFGF_NONE),
    CFG_BOOL((char *) "new_become_master", cfg_true, CFGF_NONE),
    CFG_BOOL((char *) "new_get_focus", cfg_true, CFGF_NONE),
    CFG_INT((char *) "opacity_unfocused", -1, CFGF_NONE),
    CFG_STR((char *) "floating_placement", (char *) "smart", CFGF_NONE),
    CFG_FLOAT((char *) "mwfact_lower_limit", 0.1, CFGF_NONE),
    CFG_FLOAT((char *) "mwfact_upper_limit", 0.9, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t style_opts[] =
{
    CFG_STR((char *) "border", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "bg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "fg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "shadow", (char *) NULL, CFGF_NONE),
    CFG_INT((char *) "shadow_offset", 0, CFGF_NONE),
    CFG_STR((char *) "font", (char *) NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t styles_opts[] =
{
    CFG_SEC((char *) "normal", style_opts, CFGF_NONE),
    CFG_SEC((char *) "focus", style_opts, CFGF_NONE),
    CFG_SEC((char *) "urgent", style_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t mouse_taglist_opts[] =
{
    CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
    CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t mouse_generic_opts[] =
{
    CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
    CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    CFG_STR((char *) "arg", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t widget_taglist_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_taglist_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t widget_iconbox_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_STR((char *) "image", (char *) NULL, CFGF_NONE),
    CFG_BOOL((char *) "resize", cfg_true, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_textbox_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_INT((char *) "width", 0, CFGF_NONE),
    CFG_STR((char *) "text", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "text_align", (char *) "center", CFGF_NONE),
    CFG_SEC((char *) "style", style_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_tasklist_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    CFG_STR((char *) "text_align", (char *) "left", CFGF_NONE),
    CFG_STR((char *) "show", (char *) "tags", CFGF_NONE),
    CFG_BOOL((char *) "show_icons", cfg_true, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_graph_data_opts[] =
{
    CFG_FLOAT((char *) "max", 100.0f, CFGF_NONE),
    CFG_BOOL((char *) "scale", cfg_false, CFGF_NONE),
    CFG_STR((char *) "fg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "fg_center", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "fg_end", (char *) NULL, CFGF_NONE),
    CFG_BOOL((char *) "vertical_gradient", cfg_false, CFGF_NONE),
    CFG_STR((char *) "draw_style", (char *) "bottom", CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_graph_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_SEC((char *) "data", widget_graph_data_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_INT((char *) "width", 100, CFGF_NONE),
    CFG_POSITION((char *) "grow", (char *) "left", CFGF_NONE),
    CFG_INT((char *) "padding_left", 0, CFGF_NONE),
    CFG_FLOAT((char *) "height", 0.67, CFGF_NONE),
    CFG_STR((char *) "bg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "bordercolor", (char *) NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_progressbar_data_opts[] =
{
    CFG_STR((char *) "fg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "fg_center", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "fg_end", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "bg", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "bordercolor", (char *) NULL, CFGF_NONE),
    CFG_BOOL((char *) "reverse", cfg_false, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t widget_progressbar_opts[] =
{
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_STR((char *) "align", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_SEC((char *) "data", widget_progressbar_data_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_INT((char *) "width", 100, CFGF_NONE),
    CFG_INT((char *) "gap", 2, CFGF_NONE),
    CFG_INT((char *) "padding", 0, CFGF_NONE),
    CFG_FLOAT((char *) "height", 0.67, CFGF_NONE),
    CFG_BOOL((char *) "vertical", cfg_false, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t statusbar_opts[] =
{
    CFG_POSITION((char *) "position", (char *) "top", CFGF_NONE),
    CFG_INT((char *) "height", 0, CFGF_NONE),
    CFG_INT((char *) "width", 0, CFGF_NONE),
    CFG_SEC((char *) "textbox", widget_textbox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "taglist", widget_taglist_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "layoutinfo", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "iconbox", widget_iconbox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "focusicon", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "progressbar", widget_progressbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "graph", widget_graph_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "tasklist", widget_tasklist_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_AWESOME_END()
};
cfg_opt_t tag_opts[] =
{
    CFG_STR((char *) "layout", (char *) "tile", CFGF_NONE),
    CFG_FLOAT((char *) "mwfact", 0.5, CFGF_NONE),
    CFG_INT((char *) "nmaster", 1, CFGF_NONE),
    CFG_INT((char *) "ncol", 1, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t tags_opts[] =
{
    CFG_SEC((char *) "tag", tag_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_AWESOME_END()
};
cfg_opt_t layout_opts[] =
{
    CFG_STR((char *) "image", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t layouts_opts[] =
{
    CFG_SEC((char *) "layout", layout_opts, CFGF_TITLE | CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t padding_opts[] =
{
    CFG_INT((char *) "top", 0, CFGF_NONE),
    CFG_INT((char *) "bottom", 0, CFGF_NONE),
    CFG_INT((char *) "right", 0, CFGF_NONE),
    CFG_INT((char *) "left", 0, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t screen_opts[] =
{
    CFG_SEC((char *) "general", general_opts, CFGF_NONE),
    CFG_SEC((char *) "titlebar", titlebar_opts, CFGF_NONE),
    CFG_SEC((char *) "statusbar", statusbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "tags", tags_opts, CFGF_NONE),
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    CFG_SEC((char *) "layouts", layouts_opts, CFGF_NONE),
    CFG_SEC((char *) "padding", padding_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t rule_opts[] =
{
    CFG_STR((char *) "xproperty_name", NULL, CFGF_NONE),
    CFG_STR((char *) "xproperty_value", NULL, CFGF_NONE),
    CFG_STR((char *) "name", NULL, CFGF_NONE),
    CFG_STR((char *) "tags", NULL, CFGF_NONE),
    CFG_STR((char *) "icon", NULL, CFGF_NONE),
    CFG_STR((char *) "float", (char *) "auto", CFGF_NONE),
    CFG_STR((char *) "master", (char *) "auto", CFGF_NONE),
    CFG_SEC((char *) "titlebar", titlebar_opts, CFGF_NONE),
    CFG_INT((char *) "screen", RULE_NOSCREEN, CFGF_NONE),
    CFG_FLOAT((char *) "opacity", -1.0f, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t rules_opts[] =
{
    CFG_SEC((char *) "rule", rule_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t key_opts[] =
{
    CFG_STR_LIST((char *) "modkey", (char *) "", CFGF_NONE),
    CFG_STR((char *) "key", (char *) "None", CFGF_NONE),
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    CFG_STR((char *) "arg", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t keylist_opts[] =
{
    CFG_STR_LIST((char *) "modkey", (char *) "", CFGF_NONE),
    CFG_STR_LIST((char *) "keylist", (char *) NULL, CFGF_NONE),
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    CFG_STR_LIST((char *) "arglist", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t keys_opts[] =
{
    CFG_SEC((char *) "key", key_opts, CFGF_MULTI),
    CFG_SEC((char *) "keylist", keylist_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t mouse_opts[] =
{
    CFG_SEC((char *) "root", mouse_generic_opts, CFGF_MULTI),
    CFG_SEC((char *) "client", mouse_generic_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
cfg_opt_t menu_opts[] =
{
    CFG_INT((char *) "width", 0, CFGF_NONE),
    CFG_INT((char *) "height", 0, CFGF_NONE),
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
cfg_opt_t awesome_opts[] =
{
    CFG_SEC((char *) "screen", screen_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_SEC((char *) "rules", rules_opts, CFGF_NONE),
    CFG_SEC((char *) "keys", keys_opts, CFGF_NONE),
    CFG_SEC((char *) "mouse", mouse_opts, CFGF_NONE),
    CFG_SEC((char *) "menu", menu_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_AWESOME_END()
};

/** Return default configuration file path
 * \return path to the default configuration file
 */
char *
config_file(void)
{
    const char *homedir;
    char * confpath;
    ssize_t confpath_len;

    homedir = getenv("HOME");
    confpath_len = a_strlen(homedir) + a_strlen(AWESOME_CONFIG_FILE) + 2;
    confpath = p_new(char, confpath_len);
    a_strcpy(confpath, confpath_len, homedir);
    a_strcat(confpath, confpath_len, "/");
    a_strcat(confpath, confpath_len, AWESOME_CONFIG_FILE);

    return confpath;
}

static int
config_validate_unsigned_int(cfg_t *cfg, cfg_opt_t *opt)
{
    int value = cfg_opt_getnint(opt, cfg_opt_size(opt) - 1);

    if(value < 0)
    {
        cfg_error(cfg, "integer option '%s' must be positive in section '%s'",
                  opt->name, cfg->name);
        return -1;
    }
    return 0;
}

static int
config_validate_supone_int(cfg_t *cfg, cfg_opt_t *opt)
{
    int value = cfg_opt_getnint(opt, cfg_opt_size(opt) - 1);

    if(value < 1)
    {
        cfg_error(cfg, "integer option '%s' must be at least 1 in section '%s'",
                  opt->name, cfg->name);
        return -1;
    }
    return 0;
}

static int
config_validate_zero_one_float(cfg_t *cfg, cfg_opt_t *opt)
{
    float value = cfg_opt_getnfloat(opt, cfg_opt_size(opt) - 1);

    if(value < 0.0 || value > 1.0)
    {
        cfg_error(cfg, "float option '%s' must be at least 0.0 and less than 1.0 in section '%s'",
                  opt->name, cfg->name);
        return -1;
    }
    return 0;
}

static int
config_validate_alignment(cfg_t *cfg, cfg_opt_t *opt)
{
    char *value = cfg_opt_getnstr(opt, cfg_opt_size(opt) - 1);

    if(draw_get_align(value) == Auto
       && a_strcmp(value, "auto"))
    {
        cfg_error(cfg,
                  "alignment option '%s' must be left, center, right, flex or auto in section '%s'",
                  opt->name, cfg->name);
        return -1;
    }
    return 0;
}

cfg_t *
cfg_new(void)
{
    cfg_t *cfg;

    cfg = cfg_init(awesome_opts, CFGF_NONE);

    /* Check integers values */
    cfg_set_validate_func(cfg, "screen|general|snap", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|general|border", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|general|opacity_unfocused", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|height", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|textbox|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|tags|tag|nmaster", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|tags|tag|ncol", config_validate_supone_int);

    /* Check float values */
    cfg_set_validate_func(cfg, "screen|general|mwfact_lower_limit", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|general|mwfact_upper_limit", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|tags|tag|mwfact", config_validate_zero_one_float);

    /* Check alignment values */
    cfg_set_validate_func(cfg, "screen|titlebar|text_align", config_validate_alignment);
    cfg_set_validate_func(cfg, "rules|rule|titlebar|text_align", config_validate_alignment);

    return cfg;
}

/** Check configuration file syntax in regard of libconfuse parsing
 * \param path to config file
 * \return status returned by cfg_parse()
 */
int
config_check(const char *confpatharg)
{
    cfg_t *cfg;
    int ret;
    char *confpath;

    cfg = cfg_new();

    if(confpatharg)
        confpath = a_strdup(confpatharg);
    else
        confpath = config_file();

    switch((ret = cfg_parse(cfg, confpath)))
    {
      case CFG_FILE_ERROR:
        perror("awesome: parsing configuration file failed");
        break;
      case CFG_PARSE_ERROR:
        cfg_error(cfg, "awesome: parsing configuration file %s failed.\n", confpath);
        break;
      case CFG_SUCCESS:
        printf("Configuration file OK.\n");
        break;
    }

    p_delete(&confpath);
    cfg_free(cfg);

    return ret;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
