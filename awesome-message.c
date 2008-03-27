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
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xscreen.h"

#define PROGNAME "awesome-message"

static bool running = true;

/** Import awesome config file format */
extern cfg_opt_t awesome_opts[];

/** awesome-run global configuration structure */
typedef struct
{
    /** Display ref */
    xcb_connection_t *connection;
    /** Default screen number */
    int default_screen;
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
    draw_style_init(globalconf.connection, globalconf.default_screen,
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
    running = false;
}

int
main(int argc, char **argv)
{
    SimpleWindow *sw;
    DrawCtx *ctx;
    xcb_generic_event_t *ev;
    area_t geometry = { 0, 0, 200, 50, NULL, NULL },
         icon_geometry = { -1, -1, -1, -1, NULL, NULL };
    int opt, ret, screen = 0, delay = 0;
    xcb_query_pointer_reply_t *xqp = NULL;
    char *configfile = NULL;
    ScreensInfo *si;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {NULL,      0, NULL, 0}
    };

    while((opt = getopt_long(argc, argv, "vhf:b:x:y:n:c:d:",
                             long_options, NULL)) != -1)
        switch(opt)
        {
	  case 'd':
	    if((delay = atoi(optarg)) <= 0)
                delay = 1;
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

    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        eprint("unable to open display");

    si = screensinfo_new(globalconf.connection);
    if(si->xinerama_is_active)
    {
        if((xqp = xcb_query_pointer_reply(globalconf.connection,
                                          xcb_query_pointer(globalconf.connection,
                                                            xcb_aux_get_screen(globalconf.connection,
                                                                               globalconf.default_screen)->root),
                                          NULL)) != NULL)
        {
            screen = screen_get_bycoord(si, 0, xqp->root_x, xqp->root_y);
            p_delete(&xqp);
        }
    }
    else
        screen = globalconf.default_screen;

    if((ret = config_parse(screen, configfile)))
        return ret;

    geometry.width = draw_textwidth(globalconf.connection, globalconf.default_screen,
                                    globalconf.style.font, argv[optind]);
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

    sw = simplewindow_new(globalconf.connection, globalconf.default_screen,
                          geometry.x, geometry.y, geometry.width, geometry.height, 0);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE, sw->window,
                        WM_NAME, STRING, 8, strlen(PROGNAME), PROGNAME);

    ctx = draw_context_new(globalconf.connection, globalconf.default_screen,
                           geometry.width, geometry.height, sw->drawable);

    geometry.x = geometry.y = 0;
    draw_text(ctx, geometry, AlignRight, 0,argv[optind], globalconf.style);

    if(icon_geometry.width > 0 && icon_geometry.height > 0)
        draw_image(ctx, 0, (geometry.height / 2) - (globalconf.style.font->height / 2),
                   globalconf.style.font->height, argv[optind + 1]);

    p_delete(&ctx);

    simplewindow_refresh_drawable(sw, globalconf.default_screen);

    xutil_map_raised(globalconf.connection, sw->window);
    xcb_aux_sync(globalconf.connection);

    signal(SIGALRM, &exit_on_signal);
    alarm(delay);

    while(running)
    {
       while((ev = xcb_poll_for_event(globalconf.connection)))
       {
           /* Skip errors */
           if(ev->response_type == 0)
               continue;

           switch(ev->response_type & 0x7f)
           {
             case XCB_BUTTON_PRESS:
             case XCB_KEY_PRESS:
               running = false;
             case XCB_EXPOSE:
               simplewindow_refresh_drawable(sw, globalconf.default_screen);
               break;
             default:
               break;
           }

           p_delete(&ev);
       }
       usleep(100000);
    }

    simplewindow_delete(&sw);
    xcb_disconnect(globalconf.connection);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
