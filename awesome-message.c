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

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Xlib.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xscreen.h"

#define PROGNAME "awesome-message"

static Bool running = True;

/** Import awesome config file format */
extern cfg_opt_t awesome_opts[];

/** awesome-run global configuration structure */
typedef struct
{
    /** Display ref */
    Display *display;
    /** Style */
    style_t style;
} AwesomeMsgConf;

static AwesomeMsgConf globalconf;

static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: %s [-x xcoord] [-y ycoord] [-d delay] <message> <icon>\n",
            PROGNAME);
    exit(exit_code);
}

static int
config_parse(int screen, const char *confpatharg)
{
    int ret;
    char *confpath;
    cfg_t *cfg, *cfg_screen;

    if(!confpatharg)
        confpath = config_file();
    else
        confpath = a_strdup(confpatharg);

    cfg = cfg_new();

    switch((ret = cfg_parse(cfg, confpath)))
    {
      case CFG_FILE_ERROR:
        warn("parsing configuration file failed: %s\n", strerror(errno));
        break;
      case CFG_PARSE_ERROR:
        cfg_error(cfg, "awesome: parsing configuration file %s failed.\n", confpath);
        break;
    }

    if(ret)
        return ret;

    /* get global screen section */
    if(!(cfg_screen = cfg_getnsec(cfg, "screen", screen)))
        if(!(cfg_screen = cfg_getsec(cfg, "screen")))
            eprint("parsing configuration file failed, no screen section found\n");

    /* style */
    draw_style_init(globalconf.display, DefaultScreen(globalconf.display),
                    cfg_getsec(cfg_getsec(cfg_screen, "styles"), "normal"),
                    &globalconf.style, NULL);

    if(!globalconf.style.font)
        eprint("no default font available\n");

    p_delete(&confpath);

    return ret;
}

static void
exit_on_signal(int sig __attribute__ ((unused)))
{
    running = False;
}

int
main(int argc, char **argv)
{
    Display *disp;
    SimpleWindow *sw;
    DrawCtx *ctx;
    XEvent ev;
    area_t geometry = { 0, 0, 200, 50, NULL, NULL },
         icon_geometry = { -1, -1, -1, -1, NULL, NULL };
    int opt, i, x, y, ret, screen = 0, delay = 0;
    unsigned int ui;
    char *configfile = NULL;
    ScreensInfo *si;
    Window dummy;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {NULL,      0, NULL, 0}
    };

    if(!(disp = XOpenDisplay(NULL)))
        eprint("unable to open display");

    globalconf.display = disp;

    while((opt = getopt_long(argc, argv, "vhf:b:x:y:n:c:d:",
                             long_options, NULL)) != -1)
        switch(opt)
        {
	  case 'd':
	    delay = atoi(optarg);
	    break;
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

    si = screensinfo_new(disp);
    if(si->xinerama_is_active)
    {
        if(XQueryPointer(disp, RootWindow(disp, DefaultScreen(disp)),
                         &dummy, &dummy, &x, &y, &i, &i, &ui))
            screen = screen_get_bycoord(si, 0, x, y);
    }
    else
        screen = DefaultScreen(disp);

    if((ret = config_parse(screen, configfile)))
        return ret;

    geometry.width = draw_textwidth(disp, globalconf.style.font, argv[optind]);
    geometry.height = globalconf.style.font->height * 1.5;

    if(argc - optind >= 2)
    {
        icon_geometry = draw_get_image_size(argv[optind + 1]);
        if(icon_geometry.width <= 0 || icon_geometry.height <= 0)
            eprint("invalid image\n");
        else
            geometry.width += icon_geometry.width
                * ((double) globalconf.style.font->height / (double) icon_geometry.height);
    }

    sw = simplewindow_new(disp, DefaultScreen(disp),
                          geometry.x, geometry.y, geometry.width, geometry.height, 0);

    XStoreName(disp, sw->window, PROGNAME);

    ctx = draw_context_new(disp, DefaultScreen(disp),
                           geometry.width, geometry.height, sw->drawable);

    geometry.x = geometry.y = 0;
    draw_text(ctx, geometry, AlignRight, 0,argv[optind], globalconf.style);

    if(icon_geometry.width > 0 && icon_geometry.height > 0)
        draw_image(ctx, 0, (geometry.height / 2) - (globalconf.style.font->height / 2),
                   globalconf.style.font->height, argv[optind + 1]);

    p_delete(&ctx);

    simplewindow_refresh_drawable(sw, DefaultScreen(disp));

    XMapRaised(disp, sw->window);
    XSync(disp, False);

    signal(SIGALRM, &exit_on_signal);
    alarm(delay);

    while(running)
    {
       if(XPending(disp))
       {
           XNextEvent(disp, &ev);
           switch(ev.type)
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
       usleep(100000);
    }

    simplewindow_delete(&sw);
    XCloseDisplay(disp);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
