/*
 * awesome-menu.c - menu window for awesome
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

#include <confuse.h>

#include <X11/Xlib.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xutil.h"

#define PROGNAME "awesome-menu"

#define CLEANMASK(mask) (mask & ~(globalconf.numlockmask | LockMask))

typedef enum
{
    STOP = 0,
    RUN = 1,
    CANCEL = 2
} status_t;

static status_t status = RUN;

/** Import awesome config file format */
extern cfg_opt_t awesome_opts[];

typedef struct item_t item_t;
struct item_t
{
    char *data;
    item_t *next;
    Bool match;
};

DO_SLIST(item_t, item, p_delete);

/** awesome-run global configuration structure */
typedef struct
{
    /** Display ref */
    Display *display;
    /** The window */
    SimpleWindow *sw;
    /** The draw contet */
    DrawCtx *ctx;
    /** Font to use */
    XftFont *font;
    /** Foreground color */
    XColor fg;
    /** Background color */
    XColor bg;
    /** List background */
    XColor fg_focus, bg_focus;
    /** Numlock mask */
    unsigned int numlockmask;
    /** The text */
    char text[PATH_MAX];
    /** Item list */
    item_t *items;
    /** Selected item */
    item_t *item_selected;
    /** What to do with the result text */
    char *exec;
    /** Prompt */
    char *prompt;
} AwesomeMenuConf;

static AwesomeMenuConf globalconf;

static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: %s [-x xcoord] [-y ycoord] [-e command] <message>\n",
            PROGNAME);
    exit(exit_code);
}

static int
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
        return ret;

    /* get global screen section */
    cfg_screen = cfg_getsec(cfg, "screen");

    if(!cfg_screen)
        eprint("parsing configuration file failed, no screen section found\n");

    /* get colors and general section */
    cfg_general = cfg_getsec(cfg_screen, "general");
    cfg_colors = cfg_getsec(cfg_screen, "colors");

    /* colors */
    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   cfg_getstr(cfg_colors, "normal_fg"),
                   &globalconf.fg);
    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   cfg_getstr(cfg_colors, "normal_bg"),
                   &globalconf.bg);
    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   cfg_getstr(cfg_colors, "focus_fg"),
                   &globalconf.fg_focus);
    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   cfg_getstr(cfg_colors, "focus_bg"),
                   &globalconf.bg_focus);

    /* font */
    globalconf.font = XftFontOpenName(globalconf.display, DefaultScreen(globalconf.display),
                                      cfg_getstr(cfg_general, "font"));

    p_delete(&confpath);

    return ret;
}

static void
complete(void)
{
    item_t *item = NULL;

    if(globalconf.item_selected)
        item = globalconf.item_selected->next;
    else
        for(item = globalconf.items; item; item = item->next)
            if(item->match)
            {
                globalconf.item_selected = item;
                break;
            }

    for(; item; item = item->next)
        if(item->match)
        {
            a_strcpy(globalconf.text, ssizeof(globalconf.text), item->data);
            globalconf.item_selected = item;
            return;
        }
}

static void
compute_match(void)
{
    item_t *item;

    /* reset the selected item to NULL */
    globalconf.item_selected = NULL;

    for(item = globalconf.items; item; item = item->next)
        if(!a_strncmp(globalconf.text, item->data, a_strlen(globalconf.text)))
            item->match = True;
        else
            item->match = False;
}

/* Why not? */
#define MARGIN 10

static void
redraw(void)
{
    item_t *item;
    Area geometry = globalconf.sw->geometry;
    unsigned int len;

    draw_text(globalconf.ctx, geometry, AlignLeft,
              MARGIN, globalconf.font, globalconf.prompt, globalconf.fg_focus, globalconf.bg_focus);

    len = MARGIN * 2 + draw_textwidth(globalconf.display, globalconf.font, globalconf.prompt);
    geometry.x += len;
    geometry.width -= len;

    draw_text(globalconf.ctx, geometry, AlignLeft,
              MARGIN, globalconf.font, globalconf.text, globalconf.fg, globalconf.bg);

    len = MARGIN * 2 + MAX(draw_textwidth(globalconf.display, globalconf.font, globalconf.text),
                           geometry.width / 20);
    geometry.x += len;
    geometry.width -= len;

    for(item = globalconf.items; item && geometry.width > 0; item = item->next)
        if(item->match)
        {
            if(item == globalconf.item_selected)
                draw_text(globalconf.ctx, geometry, AlignLeft,
                          MARGIN / 2, globalconf.font, item->data, globalconf.fg_focus, globalconf.bg_focus);
            else
                draw_text(globalconf.ctx, geometry, AlignLeft,
                          MARGIN / 2, globalconf.font, item->data, globalconf.fg, globalconf.bg);
            len = MARGIN + draw_textwidth(globalconf.display, globalconf.font, item->data);
            geometry.x += len;
            geometry.width -= len;
        }

    if(geometry.width)
        draw_rectangle(globalconf.ctx, geometry, True, globalconf.bg);

    simplewindow_refresh_drawable(globalconf.sw, DefaultScreen(globalconf.display));
    XSync(globalconf.display, False);
}

static void
handle_kpress(XKeyEvent *e)
{
    char buf[32];
    KeySym ksym;
    int num;
    ssize_t len;

    len = a_strlen(globalconf.text);
    num = XLookupString(e, buf, sizeof(buf), &ksym, 0);
    if(e->state & ControlMask)
        switch(ksym)
        {
          default:
            return;
          case XK_bracketleft:
            ksym = XK_Escape;
            break;
          case XK_h:
          case XK_H:
            ksym = XK_BackSpace;
            break;
          case XK_i:
          case XK_I:
            ksym = XK_Tab;
            break;
          case XK_j:
          case XK_J:
            ksym = XK_Return;
            break;
          case XK_c:
          case XK_C:
            status = CANCEL;
            break;
          case XK_w:
          case XK_W:
            /* erase word */
            if(len)
            {
                int i = len - 1;
                while(i >= 0 && globalconf.text[i] == ' ')
                    globalconf.text[i--] = '\0';
                while(i >= 0 && globalconf.text[i] != ' ')
                    globalconf.text[i--] = '\0';
                compute_match();
                redraw();
            }
            return;
        }
    else if(CLEANMASK(e->state) & Mod1Mask)
        switch(ksym)
        {
          default:
            return;
          case XK_h:
            ksym = XK_Left;
            break;
          case XK_l:
            ksym = XK_Right;
            break;
          case XK_j:
            ksym = XK_Next;
            break;
          case XK_k:
            ksym = XK_Prior;
            break;
          case XK_g:
            ksym = XK_Home;
            break;
          case XK_G:
            ksym = XK_End;
            break;
        }

    switch(ksym)
    {
      default:
        if(num && !iscntrl((int) buf[0]))
        {
            buf[num] = '\0';
            a_strncat(globalconf.text, sizeof(globalconf.text), buf, num);
            compute_match();
            redraw();
        }
        break;
      case XK_BackSpace:
        if(len)
        {
            globalconf.text[--len] = '\0';
            compute_match();
            redraw();
        }
        break;
      case XK_Right:
      case XK_Tab:
        complete();
        redraw();
        break;
      case XK_Escape:
        status= CANCEL;
        break;
      case XK_Return:
        status = STOP;
        break;
    }
}

static void
item_list_fill(void)
{
    char buf[PATH_MAX];
    item_t *newitem;

    item_list_init(&globalconf.items);

    while(fgets(buf, sizeof(buf), stdin))
    {
        buf[a_strlen(buf) - 1] = '\0';
        newitem = p_new(item_t, 1);
        newitem->data = a_strdup(buf);
        newitem->match = True;
        item_list_append(&globalconf.items, newitem);
    }
}

int
main(int argc, char **argv)
{
    Display *disp;
    XEvent ev;
    int opt, ret;
    char *configfile = NULL, *cmd;
    ssize_t len;
    const char *shell = getenv("SHELL");
    Area geometry = { 0, 0, 0, 0, NULL };
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"exec",    0, NULL, 'e'},
        {NULL,      0, NULL, 0}
    };

    if(!(disp = XOpenDisplay(NULL)))
        eprint("unable to open display");

    globalconf.display = disp;

    while((opt = getopt_long(argc, argv, "vhf:b:x:y:n:c:e:",
                             long_options, NULL)) != -1)
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
          case 'e':
            globalconf.exec = a_strdup(optarg);
            break;
        }

    if(argc - optind >= 1)
        globalconf.prompt = a_strdup(argv[optind]);

    if((ret = config_parse(configfile)))
        return ret;

    /* Get the numlock mask */
    globalconf.numlockmask = get_numlockmask(disp);

    /* Init the geometry */
    geometry.height = globalconf.font->height * 1.5;
    geometry.width = DisplayWidth(globalconf.display, DefaultScreen(globalconf.display));

    /* Create the window */
    globalconf.sw = simplewindow_new(disp, DefaultScreen(disp),
                                     geometry.x, geometry.y, geometry.width, geometry.height, 0);

    XStoreName(disp, globalconf.sw->window, PROGNAME);
    XMapRaised(disp, globalconf.sw->window);

    /* Create the drawing context */
    globalconf.ctx = draw_context_new(disp, DefaultScreen(disp),
                                      geometry.width, geometry.height,
                                      globalconf.sw->drawable);



    item_list_fill();

    if(XGrabKeyboard(disp, DefaultRootWindow(disp), True,
                     GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
        eprint("cannot grab keyboard");

    redraw();

    while(status == RUN)
    {
        XNextEvent(disp, &ev);
        switch(ev.type)
        {
          case ButtonPress:
            status = CANCEL;
            break;
          case KeyPress:
            handle_kpress(&ev.xkey);
            break;
          case Expose:
            if(!ev.xexpose.count)
                simplewindow_refresh_drawable(globalconf.sw, DefaultScreen(disp));
            break;
          default:
            break;
        }
    }

    if(status != CANCEL)
    {
        if(!globalconf.exec)
            printf("%s\n", globalconf.text);
        else
        {
            if(!shell)
                shell = a_strdup("/bin/sh");
            len = a_strlen(globalconf.exec) + a_strlen(globalconf.text) + 1;
            cmd = p_new(char, len);
            a_strcpy(cmd, len, globalconf.exec);
            a_strcat(cmd, len, globalconf.text);
            execl(shell, shell, "-c", cmd, NULL);
        }
    }

    simplewindow_delete(globalconf.sw);
    XCloseDisplay(disp);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
