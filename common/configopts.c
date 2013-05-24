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

#define CFG_ALIGNMENT(name, value, flags) \
    CFG_PTR_CB(name, value, flags, \
               cfg_alignment_parse, cfg_value_free)

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

static int
cfg_alignment_parse(cfg_t *cfg, cfg_opt_t *opt,
                    const char *value, void *result)
{
    Alignment *p = p_new(Alignment, 1);
    
    if((*p = draw_align_get_from_str(value)) == Auto
       && a_strcmp(value, "auto"))
    {
        cfg_error(cfg,
                  "alignment option '%s' must be left, center, right, flex or auto in section '%s'",
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

Alignment
cfg_opt_getnalignment(cfg_opt_t *opt, unsigned int oindex)
{
    return * (Alignment *) cfg_opt_getnptr(opt, oindex);
}

Alignment
cfg_getnalignment(cfg_t *cfg, const char *name, unsigned int oindex)
{
    return cfg_opt_getnalignment(cfg_getopt(cfg, name), oindex);
}

Alignment
cfg_getalignment(cfg_t *cfg, const char *name)
{
    return cfg_getnalignment(cfg, name, 0);
}

Position
cfg_opt_getnposition(cfg_opt_t *opt, unsigned int oindex)
{
    return * (Position *) cfg_opt_getnptr(opt, oindex);
}

Position
cfg_getnposition(cfg_t *cfg, const char *name, unsigned int oindex)
{
    return cfg_opt_getnposition(cfg_getopt(cfg, name), oindex);
}

Position
cfg_getposition(cfg_t *cfg, const char *name)
{
    return cfg_getnposition(cfg, name, 0);
}

/** This section defines a style. */
cfg_opt_t style_opts[] =
{
    /** Windows border color. */
    CFG_STR((char *) "border", NULL, CFGF_NONE),
    /** Background color. */
    CFG_STR((char *) "bg", NULL, CFGF_NONE),
    /** Foreground color. */
    CFG_STR((char *) "fg", NULL, CFGF_NONE),
    /** Shadow color. */
    CFG_STR((char *) "shadow", NULL, CFGF_NONE),
    /** Shadow offset in pixels. */
    CFG_INT((char *) "shadow_offset", 0xffffffff, CFGF_NONE),
    /** Font to use. */
    CFG_STR((char *) "font", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines several styles in a row. */
cfg_opt_t styles_opts[] =
{
    /** Normal style. */
    CFG_SEC((char *) "normal", style_opts, CFGF_NONE),
    /** Style used for the focused window. */
    CFG_SEC((char *) "focus", style_opts, CFGF_NONE),
    /** Style used for windows with urgency hint. */
    CFG_SEC((char *) "urgent", style_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines a titlebar. */
cfg_opt_t titlebar_opts[] =
{
    /** Titlebar position. */
    CFG_POSITION((char *) "position", (char *) "auto", CFGF_NONE),
    /** Titlebar alignment around window. */
    CFG_ALIGNMENT((char *) "align", (char *) "left", CFGF_NONE),
    /** Titlebar width. Set to 0 for auto. */
    CFG_INT((char *) "width", 0, CFGF_NONE),
    /** Titlebar height. Set to 0 for auto. */
    CFG_INT((char *) "height", 0, CFGF_NONE),
    /** Text alignment. */
    CFG_ALIGNMENT((char *) "text_align", (char *) "center", CFGF_NONE),
    /** Titlebar styles. */
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines general options. */
cfg_opt_t general_opts[] =
{
    /** The border size of windows in pixels. */
    CFG_INT((char *) "border", 1, CFGF_NONE),
    /** The number of pixels to snap when moving floating windows. */
    CFG_INT((char *) "snap", 8, CFGF_NONE),
    /** Use resize hints when resizing tiled windows. Can produce gaps between windows. */
    CFG_BOOL((char *) "resize_hints", cfg_true, CFGF_NONE),
    /** Enable sloppy focus, also known as focus follows mouse. */
    CFG_BOOL((char *) "sloppy_focus", cfg_true, CFGF_NONE),
    /** Raise the window if it is given focus with the mouse. */
    CFG_BOOL((char *) "sloppy_focus_raise", cfg_false, CFGF_NONE),
    /** New windows become master windows. */
    CFG_BOOL((char *) "new_become_master", cfg_true, CFGF_NONE),
    /** New windows get focus. */
    CFG_BOOL((char *) "new_get_focus", cfg_true, CFGF_NONE),
    /** Opacity of windows when unfocused. */
    CFG_FLOAT((char *) "opacity_unfocused", -1, CFGF_NONE),
    /** Opacity of windows when focused. */
    CFG_FLOAT((char *) "opacity_focused", -1, CFGF_NONE),
    /** How to dispose floating windows. Can be smart or under_mouse. */
    CFG_STR((char *) "floating_placement", (char *) "smart", CFGF_NONE),
    /** Lower limit for the master window size factor. */
    CFG_FLOAT((char *) "mwfact_lower_limit", 0.1, CFGF_NONE),
    /** Upper limit for the master window size factor. */
    CFG_FLOAT((char *) "mwfact_upper_limit", 0.9, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines mouse bindings or the taglist widget. */
cfg_opt_t mouse_taglist_opts[] =
{
    /** Modifier keys. */
    CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
    /** Mouse button. */
    CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
    /** Uicb command to run. */
    CFG_STR((char *) "command", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines generic mouse bindings. */
cfg_opt_t mouse_generic_opts[] =
{
    /** Modifier keys. */
    CFG_STR_LIST((char *) "modkey", (char *) "{}", CFGF_NONE),
    /** Mouse button. */
    CFG_STR((char *) "button", (char *) "None", CFGF_NONE),
    /** Uicb command to run. */
    CFG_STR((char *) "command", NULL, CFGF_NONE),
    /** Argument to use for command. */
    CFG_STR((char *) "arg", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines common widgets options. */
cfg_opt_t widget_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines taglist widget options. */
cfg_opt_t widget_taglist_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_taglist_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines iconbox widget options. */
cfg_opt_t widget_iconbox_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Image to draw. */
    CFG_STR((char *) "image", NULL, CFGF_NONE),
    /** Enable automatic resize of the image. */
    CFG_BOOL((char *) "resize", cfg_true, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines textbox widget options. */
cfg_opt_t widget_textbox_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Widget width. Set to 0 for auto. */
    CFG_INT((char *) "width", 0, CFGF_NONE),
    /** Default printed text. */
    CFG_STR((char *) "text", NULL, CFGF_NONE),
    /** Text alignment. */
    CFG_ALIGNMENT((char *) "text_align", (char *) "center", CFGF_NONE),
    /** Style to use for drawing. */
    CFG_SEC((char *) "style", style_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines emptybox widget options. */
cfg_opt_t widget_emptybox_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Widget width. Set to 0 for auto. */
    CFG_INT((char *) "width", 0, CFGF_NONE),
    /** Style to use for drawing. */
    CFG_SEC((char *) "style", style_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines tasklist widget options */
cfg_opt_t widget_tasklist_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Styles to use for drawing. */
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    /** Text alignment. */
    CFG_ALIGNMENT((char *) "text_align", (char *) "left", CFGF_NONE),
    /** Which windows to show: tags, all or focus. */
    CFG_STR((char *) "show", (char *) "tags", CFGF_NONE),
    /** Show icons of windows. */
    CFG_BOOL((char *) "show_icons", cfg_true, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines graph data widget options. */
cfg_opt_t widget_graph_data_opts[] =
{
    /** Value of a full graph. */
    CFG_FLOAT((char *) "max", 100.0f, CFGF_NONE),
    /** Scale graph when values are bigger than 'max'. */
    CFG_BOOL((char *) "scale", cfg_false, CFGF_NONE),
    /** Foreground color. */
    CFG_STR((char *) "fg", NULL, CFGF_NONE),
    /** Foreground color in the center of the bar (as gradient). */
    CFG_STR((char *) "fg_center", NULL, CFGF_NONE),
    /** Foreground color at the end of a bar (as gradient). */
    CFG_STR((char *) "fg_end", NULL, CFGF_NONE),
    /** fg, fg_center and fg_end define a vertical gradient. */
    CFG_BOOL((char *) "vertical_gradient", cfg_false, CFGF_NONE),
    /**  Draw style. */
    CFG_STR((char *) "draw_style", (char *) "bottom", CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines graph widget options. */
cfg_opt_t widget_graph_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Data stream. */
    CFG_SEC((char *) "data", widget_graph_data_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Widget width. Set to 0 for auto. */
    CFG_INT((char *) "width", 100, CFGF_NONE),
    /** Put new values onto the 'left' or 'right'. */
    CFG_POSITION((char *) "grow", (char *) "left", CFGF_NONE),
    /** Set height (i.e. 0.9 = 90%). */
    CFG_FLOAT((char *) "height", 0.67, CFGF_NONE),
    /** Background color. */
    CFG_STR((char *) "bg", NULL, CFGF_NONE),
    /** Border color. */
    CFG_STR((char *) "bordercolor", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines graph widget options. */
cfg_opt_t widget_progressbar_data_opts[] =
{
    /** Foreground color. */
    CFG_STR((char *) "fg", NULL, CFGF_NONE),
    /** Foreground color in the center of the bar (as gradient). */
    CFG_STR((char *) "fg_center", NULL, CFGF_NONE),
    /** Foreground color at the end of a bar (as gradient). */
    CFG_STR((char *) "fg_end", NULL, CFGF_NONE),
    /** Foreground color of not filled bar/ticks. */
    CFG_STR((char *) "fg_off", NULL, CFGF_NONE),
    /** Background color. */
    CFG_STR((char *) "bg", NULL, CFGF_NONE),
    /** Border color. */
    CFG_STR((char *) "bordercolor", NULL, CFGF_NONE),
    /** Reverse/mirror the bar. */
    CFG_BOOL((char *) "reverse", cfg_false, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines progressbar widget options. */
cfg_opt_t widget_progressbar_opts[] =
{
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** Widget alignment. */
    CFG_ALIGNMENT((char *) "align", (char *) "auto", CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_generic_opts, CFGF_MULTI),
    /** Draws a bar for each data section. */
    CFG_SEC((char *) "data", widget_progressbar_data_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Widget width. */
    CFG_INT((char *) "width", 100, CFGF_NONE),
    /** Distance between individual bars. */
    CFG_INT((char *) "gap", 2, CFGF_NONE),
    /** Border width in pixels. */
    CFG_INT((char *) "border_width", 1, CFGF_NONE),
    /** Padding between border and ticks/bar. */
    CFG_INT((char *) "border_padding", 0, CFGF_NONE),
    /** Distance between the ticks. */
    CFG_INT((char *) "ticks_gap", 1, CFGF_NONE),
    /** Number of 'ticks' to draw. */
    CFG_INT((char *) "ticks_count", 0, CFGF_NONE),
    /** Set height (i.e. 0.9 = 90%). */
    CFG_FLOAT((char *) "height", 0.67, CFGF_NONE),
    /** Draw the bar(s) vertically. */
    CFG_BOOL((char *) "vertical", cfg_false, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines statusbar options. */
cfg_opt_t statusbar_opts[] =
{
    /** Statusbar position. */
    CFG_POSITION((char *) "position", (char *) "top", CFGF_NONE),
    /** Statusbar height. Set to 0 for auto. */
    CFG_INT((char *) "height", 0, CFGF_NONE),
    /** Statusbar width. Set to 0 for auto. */
    CFG_INT((char *) "width", 0, CFGF_NONE),
    /** Textbox widget(s). */
    CFG_SEC((char *) "textbox", widget_textbox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Emptybox widget(s). */
    CFG_SEC((char *) "emptybox", widget_emptybox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Taglist widget(s). */
    CFG_SEC((char *) "taglist", widget_taglist_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Layoutinfo widget(s). */
    CFG_SEC((char *) "layoutinfo", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Iconbox widget(s). */
    CFG_SEC((char *) "iconbox", widget_iconbox_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Focusicon widget(s). */
    CFG_SEC((char *) "focusicon", widget_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Progressbar widget(s). */
    CFG_SEC((char *) "progressbar", widget_progressbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Graph(s) widget(s). */
    CFG_SEC((char *) "graph", widget_graph_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Tasklist(s) widget(s). */
    CFG_SEC((char *) "tasklist", widget_tasklist_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_AWESOME_END()
};
/** This section defines tag options. */
cfg_opt_t tag_opts[] =
{
    /** Default layout for this tag. */
    CFG_STR((char *) "layout", (char *) "tile", CFGF_NONE),
    /** Default master width factor for this tag. */
    CFG_FLOAT((char *) "mwfact", 0.618, CFGF_NONE),
    /** Default number of master windows for this tag. */
    CFG_INT((char *) "nmaster", 1, CFGF_NONE),
    /** Default number of window columns for this tag. */
    CFG_INT((char *) "ncol", 1, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines tags options. */
cfg_opt_t tags_opts[] =
{
    /** Available tag(s). */
    CFG_SEC((char *) "tag", tag_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_AWESOME_END()
};
/** This section defines layout options. */
cfg_opt_t layout_opts[] =
{
    /** Image to represent layout in layoutinfo widget. */
    CFG_STR((char *) "image", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines layouts options. */
cfg_opt_t layouts_opts[] =
{
    /** Available layout(s). */
    CFG_SEC((char *) "layout", layout_opts, CFGF_TITLE | CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines padding options. */
cfg_opt_t padding_opts[] =
{
    /** Top padding in pixels. */
    CFG_INT((char *) "top", 0, CFGF_NONE),
    /** Bottom padding in pixels. */
    CFG_INT((char *) "bottom", 0, CFGF_NONE),
    /** Right padding in pixels. */
    CFG_INT((char *) "right", 0, CFGF_NONE),
    /** Left padding in pixels. */
    CFG_INT((char *) "left", 0, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines screen options. */
cfg_opt_t screen_opts[] =
{
    /** General options. */
    CFG_SEC((char *) "general", general_opts, CFGF_NONE),
    /** Titlebar definitions. */
    CFG_SEC((char *) "titlebar", titlebar_opts, CFGF_NONE),
    /** Statubar(s) definitions. */
    CFG_SEC((char *) "statusbar", statusbar_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** Tags definitions. */
    CFG_SEC((char *) "tags", tags_opts, CFGF_NONE),
    /** Styles definitions. */
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    /** Layouts definitions. */
    CFG_SEC((char *) "layouts", layouts_opts, CFGF_NONE),
    /** Paddings definitions. */
    CFG_SEC((char *) "padding", padding_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines rule options. */
cfg_opt_t rule_opts[] =
{
    /** Name of the xproperty to match. */
    CFG_STR((char *) "xproperty_name", NULL, CFGF_NONE),
    /** Regexp value of the xproperty above to match. */
    CFG_STR((char *) "xproperty_value", NULL, CFGF_NONE),
    /** Regexp to match the window against a string formatted like: class:name:title. */
    CFG_STR((char *) "name", NULL, CFGF_NONE),
    /** Tags matching that regexp to tag windows with. */
    CFG_STR((char *) "tags", NULL, CFGF_NONE),
    /** Icon to use for that window. */
    CFG_STR((char *) "icon", NULL, CFGF_NONE),
    /** Set this window floating. */
    CFG_STR((char *) "float", (char *) "auto", CFGF_NONE),
    /** Set this window as master. */
    CFG_STR((char *) "master", (char *) "auto", CFGF_NONE),
    /** Titlebar for this window. */
    CFG_SEC((char *) "titlebar", titlebar_opts, CFGF_NONE),
    /** Screen to start this window on. */
    CFG_INT((char *) "screen", RULE_NOSCREEN, CFGF_NONE),
    /** Opacity for this window. */
    CFG_FLOAT((char *) "opacity", -1, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines rules options. */
cfg_opt_t rules_opts[] =
{
    /** A rule. A window can match one rule. */
    CFG_SEC((char *) "rule", rule_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines a global key binding. */
cfg_opt_t key_opts[] =
{
    /** Modifier keys. */
    CFG_STR_LIST((char *) "modkey", (char *) "", CFGF_NONE),
    /** Key to press. */
    CFG_STR((char *) "key", (char *) "None", CFGF_NONE),
    /** Uicb command to run. */
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    /** Argument to use for command. */
    CFG_STR((char *) "arg", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines keylist options. */
cfg_opt_t keylist_opts[] =
{
    /** Modifier keys. */
    CFG_STR_LIST((char *) "modkey", (char *) "", CFGF_NONE),
    /** List of keys, order matters. */
    CFG_STR_LIST((char *) "keylist", (char *) NULL, CFGF_NONE),
    /** Uicb command to run. */
    CFG_STR((char *) "command", (char *) "", CFGF_NONE),
    /** List of arguments for command, order matters. */
    CFG_STR_LIST((char *) "arglist", NULL, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines keys options. */
cfg_opt_t keys_opts[] =
{
    /** A key binding. */
    CFG_SEC((char *) "key", key_opts, CFGF_MULTI),
    /** A list of key bindings. */
    CFG_SEC((char *) "keylist", keylist_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines mouse options. */
cfg_opt_t mouse_opts[] =
{
    /** Mouse bindings for the root window. */
    CFG_SEC((char *) "root", mouse_generic_opts, CFGF_MULTI),
    /** Mouse bindings for the clients' window. */
    CFG_SEC((char *) "client", mouse_generic_opts, CFGF_MULTI),
    /** Mouse bindings for the clients' titlebar. */
    CFG_SEC((char *) "titlebar", mouse_generic_opts, CFGF_MULTI),
    CFG_AWESOME_END()
};
/** This section defines menu options. */
cfg_opt_t menu_opts[] =
{
    /** Width of the menu. Set to 0 for auto. */
    CFG_INT((char *) "width", 0, CFGF_NONE),
    /** Height of the menu. Set to 0 for auto. */
    CFG_INT((char *) "height", 0, CFGF_NONE),
    /** X coordinate, do not set for auto. */
    CFG_INT((char *) "x", 0xffffffff, CFGF_NONE),
    /** Y coordinate, do not set for auto. */
    CFG_INT((char *) "y", 0xffffffff, CFGF_NONE),
    /** String matching mode (true to complete string) */
    CFG_BOOL((char *) "match_string", cfg_false, CFGF_NONE),
    /** Autocompletion (true or false) */
    CFG_BOOL((char *) "autocomplete", cfg_true, CFGF_NONE),
    /** Styles to use for this menu. */
    CFG_SEC((char *) "styles", styles_opts, CFGF_NONE),
    CFG_AWESOME_END()
};
/** This section defines global awesome options. */
cfg_opt_t awesome_opts[] =
{
    /** The screens section. Make one for each of your screens. */
    CFG_SEC((char *) "screen", screen_opts, CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    /** The rules section. This allows specific options for specific windows. */
    CFG_SEC((char *) "rules", rules_opts, CFGF_NONE),
    /** Key bindings. */
    CFG_SEC((char *) "keys", keys_opts, CFGF_NONE),
    /** Mouse bindings. */
    CFG_SEC((char *) "mouse", mouse_opts, CFGF_NONE),
    /** Menu options. */
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
config_validate_supzero_float(cfg_t *cfg, cfg_opt_t *opt)
{
    float value = cfg_opt_getnfloat(opt, cfg_opt_size(opt) - 1);

    if(value <= 0.0)
    {
        cfg_error(cfg, "float option '%s' must be greater than 0.0 section '%s'",
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
    cfg_set_validate_func(cfg, "screen|statusbar|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|height", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|tags|tag|nmaster", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|titlebar|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|titlebar|height", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "rules|rule|titlebar|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "rules|rule|titlebar|height", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "rules|rule|screen", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|textbox|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|emptybox|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|graph|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|border_width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|border_padding", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|ticks_gap", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|ticks_count", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "menu|width", config_validate_unsigned_int);
    cfg_set_validate_func(cfg, "menu|height", config_validate_unsigned_int);

    /* Check integers values > 1 */
    cfg_set_validate_func(cfg, "screen|tags|tag|ncol", config_validate_supone_int);

    /* Check float values */
    cfg_set_validate_func(cfg, "screen|general|mwfact_lower_limit", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|general|mwfact_upper_limit", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|tags|tag|mwfact", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|general|opacity_unfocused", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|general|opacity_focused", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "rules|rule|opacity", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|statusbar|graph|height", config_validate_zero_one_float);
    cfg_set_validate_func(cfg, "screen|statusbar|progressbar|height", config_validate_zero_one_float);

    /* Check float values > 0.0 */
    cfg_set_validate_func(cfg, "screen|statusbar|graph|data|max", config_validate_supzero_float);

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
        warn("parsing configuration file failed: %s\n", strerror(errno));
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
