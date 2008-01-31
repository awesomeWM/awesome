/*
 * awesome-message.c - message window for awesome
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>

#include <confuse.h>

#include <X11/Xlib.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/awesome-version.h"
#include "common/configopts.h"

#define PROGNAME "awesome-message"

/** Import awesome config file format */
extern cfg_opt_t awesome_opts[];

/** awesome-run global configuration structure */
typedef struct
{
    /** Display ref */
    Display *display;
    /** Font to use */
    XftFont *font;
    /** Foreground color */
    XColor fg;
    /** Background color */
    XColor bg;
} AwesomeMsgConf;

static AwesomeMsgConf globalconf;

static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: %s [-x xcoord] [-y ycoord] <message> <icon>\n",
            PROGNAME);
    exit(exit_code);
}

static void
config_parse(const char *confpatharg)
{
    int ret;
    char *confpath;
    cfg_t *cfg, *cfg_screen, *cfg_general, *cfg_colors;

    if(!confpatharg)
        confpath = config_file();
    else
        confpath = a_strdup(confpatharg);

    cfg = cfg_init(awesome_opts, CFGF_NONE);

    switch((ret = cfg_parse(cfg, confpath)))
    {
      case CFG_FILE_ERROR:
        perror("awesome-message: parsing configuration file failed");
        break;
      case CFG_PARSE_ERROR:
        cfg_error(cfg, "awesome: parsing configuration file %s failed.\n", confpath);
        break;
    }

    if(ret)
        exit(ret);

    /* get global screen section */
    cfg_screen = cfg_getsec(cfg, "screen");

    if(!cfg_screen)
        eprint("parsing configuration file failed, no screen section found\n");

    /* get colors and general section */
    cfg_general = cfg_getsec(cfg_screen, "general");
    cfg_colors = cfg_getsec(cfg_screen, "colors");

    /* colors */
    globalconf.fg = draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                                   cfg_getstr(cfg_colors, "normal_fg"));
    globalconf.bg = draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                                   cfg_getstr(cfg_colors, "normal_bg"));

    /* font */
    globalconf.font = XftFontOpenName(globalconf.display, DefaultScreen(globalconf.display),
                                      cfg_getstr(cfg_general, "font"));

    p_delete(&confpath);
}

int
main(int argc, char **argv)
{
    Display *disp;
    SimpleWindow *sw;
    DrawCtx *ctx;
    XEvent ev;
    Bool running = True;
    Area geometry = { 0, 0, 200, 50, NULL },
         icon_geometry = { -1, -1, -1, -1, NULL };
    int opt, option_index = 0;
    char *configfile = NULL;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {NULL,      0, NULL, 0}
    };

    if(!(disp = XOpenDisplay(NULL)))
        eprint("unable to open display");

    globalconf.display = disp;

    while((opt = getopt_long(argc, argv, "vhf:b:x:y:n:c:",
                             long_options, &option_index)) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version(PROGNAME);
            break;
          case 'x':
            geometry.x = atoi(optarg);
            break;
          case 'y':
            geometry.y = atoi(optarg);
            break;
          case 'h':
            exit_help(EXIT_SUCCESS);
            break;
          case 'c':
            configfile = a_strdup(optarg);
            break;
        }

    if(argc - optind < 1)
        exit_help(EXIT_FAILURE);

    config_parse(configfile);

    geometry.width = draw_textwidth(disp, globalconf.font, argv[optind]);
    geometry.height = globalconf.font->height * 1.5;

    if(argc - optind >= 2)
    {
        icon_geometry = draw_get_image_size(argv[optind + 1]);
        if(icon_geometry.width <= 0 || icon_geometry.height <= 0)
            eprint("invalid image\n");
        else
            geometry.width += icon_geometry.width
                * ((double) globalconf.font->height / (double) icon_geometry.height);
    }

    sw = simplewindow_new(disp, DefaultScreen(disp),
                          geometry.x, geometry.y, geometry.width, geometry.height, 0);

    XStoreName(disp, sw->window, "awmessage");

    ctx = draw_context_new(disp, DefaultScreen(disp),
                           geometry.width, geometry.height, sw->drawable);

    geometry.x = geometry.y = 0;
    draw_text(ctx, geometry, AlignRight,
              0, globalconf.font, argv[optind], globalconf.fg, globalconf.bg);

    if(icon_geometry.width > 0 && icon_geometry.height > 0)
        draw_image(ctx, 0, (geometry.height / 2) - (globalconf.font->height / 2),
                   globalconf.font->height,
                   argv[optind + 1]);

    p_delete(&ctx);

    simplewindow_refresh_drawable(sw, DefaultScreen(disp));

    XMapRaised(disp, sw->window);
    XSync(disp, False);

    while (running)
    {
       XNextEvent(disp, &ev);
       switch (ev.type)
       {
         case ButtonPress:
         case KeyPress:
           running = False;
         case Expose:
           simplewindow_refresh_drawable(sw, DefaultScreen(disp));
           break;
         default:
           break;
       }
    }

    simplewindow_delete(sw);
    XCloseDisplay(disp);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
