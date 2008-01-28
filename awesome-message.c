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

#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/awesome-version.h"

static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: awmessage [-x xcoord] [-y ycoord] [-f fgcolor] [-b bgcolor] <message> <icon>\n");
    exit(exit_code);
}

int
main(int argc, char **argv)
{
    Display *disp;
    SimpleWindow *sw;
    DrawCtx *ctx;
    XColor fg, bg;
    XEvent ev;
    Bool running = True;
    const char *fg_color = "#000000";
    const char *bg_color = "#ffffff";
    int opt;
    Area geometry = { 0, 0, 200, 50, NULL },
         icon_geometry = { -1, -1, -1, -1, NULL };
    XftFont *font = NULL;

    if(!(disp = XOpenDisplay(NULL)))
        eprint("unable to open display");

    while((opt = getopt(argc, argv, "vhf:b:x:y:n:")) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version("awmessage");
            break;
          case 'f':
            fg_color = a_strdup(optarg);
            break;
          case 'b':
            bg_color = a_strdup(optarg);
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
          case 'n':
            font = XftFontOpenName(disp, DefaultScreen(disp), optarg);
            break;
        }

    if(argc - optind < 1)
        exit_help(EXIT_FAILURE);

    if(!font)
        font = XftFontOpenName(disp, DefaultScreen(disp), "vera-12");

    geometry.width = draw_textwidth(disp, font, argv[optind]);
    geometry.height = font->height * 1.5;
    
    if(argc - optind >= 2)
    {
        icon_geometry = draw_get_image_size(argv[optind + 1]);
        if(icon_geometry.width <= 0 || icon_geometry.height <= 0)
            eprint("invalid image\n");
        else
            geometry.width += icon_geometry.width * ((double) font->height / (double) icon_geometry.height);
    }

    sw = simplewindow_new(disp, DefaultScreen(disp),
                          geometry.x, geometry.y, geometry.width, geometry.height, 0);

    XStoreName(disp, sw->window, "awmessage");

    ctx = draw_context_new(disp, DefaultScreen(disp),
                           geometry.width, geometry.height, sw->drawable);


    bg = draw_color_new(disp, DefaultScreen(disp), bg_color);
    fg = draw_color_new(disp, DefaultScreen(disp), fg_color);

    geometry.x = geometry.y = 0;
    draw_text(ctx, geometry, AlignRight,
              0, font, argv[optind], fg, bg);

    if(icon_geometry.width > 0 && icon_geometry.height > 0)
        draw_image(ctx, 0, (geometry.height / 2) - (font->height / 2),
                   font->height,
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
