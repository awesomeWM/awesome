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
#include <dirent.h>
#include <sys/types.h>

#include <confuse.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

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

static void
item_delete(item_t **item)
{
    p_delete(&(*item)->data);
    p_delete(item);
}

DO_SLIST(item_t, item, item_delete);

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
    /** Draw shadow_offset under text */
    Bool shadow_offset;
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
    fprintf(outfile, "Usage: %s [-e command] <message>\n",
            PROGNAME);
    exit(exit_code);
}

static int
config_parse(const char *confpatharg, const char *menu_title, Area *geometry)
{
    int ret, i;
    char *confpath, *opt;
    cfg_t *cfg, *cfg_menu = NULL, *cfg_screen, *cfg_general,
          *cfg_colors, *cfg_menu_colors = NULL;

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

    if(menu_title && !(cfg_menu = cfg_gettsec(cfg, "menu", menu_title)))
        warn("no definition for menu %s in configuration file: using default\n", menu_title);

    /* get global screen section */
    if(!(cfg_screen = cfg_getsec(cfg, "screen")))
        eprint("parsing configuration file failed, no screen section found\n");

    /* get colors and general section */
    if(cfg_menu)
        cfg_menu_colors = cfg_getsec(cfg_menu, "colors");
    cfg_general = cfg_getsec(cfg_screen, "general");
    cfg_colors = cfg_getsec(cfg_screen, "colors");

    /* colors */
    if(!cfg_menu_colors || !(opt = cfg_getstr(cfg_menu_colors, "normal_fg")))
       opt = cfg_getstr(cfg_colors, "normal_fg");

    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   opt, &globalconf.fg);

    if(!cfg_menu_colors || !(opt = cfg_getstr(cfg_menu_colors, "normal_bg")))
       opt = cfg_getstr(cfg_colors, "normal_bg");

    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   opt, &globalconf.bg);

    if(!cfg_menu_colors || !(opt = cfg_getstr(cfg_menu_colors, "focus_fg")))
       opt = cfg_getstr(cfg_colors, "focus_fg");

    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   opt, &globalconf.fg_focus);

    if(!cfg_menu_colors || !(opt = cfg_getstr(cfg_menu_colors, "focus_bg")))
       opt = cfg_getstr(cfg_colors, "focus_bg");

    draw_color_new(globalconf.display, DefaultScreen(globalconf.display),
                   opt, &globalconf.bg_focus);

    /* font */
    if(!cfg_menu || !(opt = cfg_getstr(cfg_menu, "font")))
        opt = cfg_getstr(cfg_general, "font");

    globalconf.font = XftFontOpenName(globalconf.display, DefaultScreen(globalconf.display),
                                      opt);

    globalconf.shadow_offset = cfg_getint(cfg_general, "text_shadow_offset");

    if(cfg_menu)
    {
        if((i = cfg_getint(cfg_menu, "x")) != (int) 0xffffffff)
           geometry->x = i;
        if((i = cfg_getint(cfg_menu, "y")) != (int) 0xffffffff)
           geometry->y = i;
        if((i = cfg_getint(cfg_menu, "width")) > 0)
           geometry->width = i;
        if((i = cfg_getint(cfg_menu, "height")) > 0)
           geometry->height = i;
    }

    p_delete(&confpath);

    return ret;
}

static char *
get_last_word(char *text)
{
    char *last_word;

    if((last_word = strrchr(text, ' ')))
        last_word++;
    else
        last_word = text;

    return last_word;
}

static Bool
item_list_fill_file(const char *directory)
{
    char cwd[PATH_MAX], *home;
    DIR *dir;
    struct dirent *dirinfo;
    item_t *item;
    ssize_t len;

    item_list_wipe(&globalconf.items);

    if(a_strlen(directory))
    {
        if(!a_strncmp(directory, "~/", 2)
           && (home = getenv("HOME")))
        {
            a_strcpy(cwd, sizeof(cwd), home);
            a_strcat(cwd, sizeof(cwd), directory + 1);
        }
        else
            a_strcpy(cwd, sizeof(cwd), directory);
    }
    else
        a_strcpy(cwd, sizeof(cwd), ".");

    if(!(dir = opendir(cwd)))
        return False;

    while((dirinfo = readdir(dir)))
    {
        item = p_new(item_t, 1);
        len = a_strlen(dirinfo->d_name) + a_strlen(cwd) + 1;
        item->data = p_new(char, len);
        if(a_strcmp(cwd, "."))
            a_strcpy(item->data, len, directory);
        a_strcat(item->data, len, dirinfo->d_name);
        item_list_push(&globalconf.items, item);
    }

    closedir(dir);

    return True;
}

static void
complete(Bool reverse)
{
    item_t *item = NULL;
    item_t *(*item_iter)(item_t **, item_t *) = item_list_next_cycle;

    if(reverse)
        item_iter = item_list_prev_cycle;

    if(globalconf.item_selected)
        item = item_iter(&globalconf.items, globalconf.item_selected);
    else
        for(item = globalconf.items; item; item = item_iter(&globalconf.items, item))
            if(item->match)
            {
                globalconf.item_selected = item;
                break;
            }

    for(; item; item = item_iter(&globalconf.items, item))
        if(item->match)
        {
            /* XXX sizeof wrong */
            a_strcpy(get_last_word(globalconf.text), ssizeof(globalconf.text), item->data);
            globalconf.item_selected = item;
            return;
        }
}

static void
compute_match(const char *word)
{
    ssize_t len = a_strlen(word);
    item_t *item;

    /* reset the selected item to NULL */
    globalconf.item_selected = NULL;

    if(len)
    {
        if(word[len - 1] == '/'
           || word[len - 1] == ' ')
            item_list_fill_file(word);

        for(item = globalconf.items; item; item = item->next)
            if(!a_strncmp(word, item->data, a_strlen(word)))
                item->match = True;
            else
                item->match = False;
    }
    else
    {
        item_list_fill_file(NULL);
        for(item = globalconf.items; item; item = item->next)
                item->match = True;
    }
}

/* Why not? */
#define MARGIN 10

static void
draw_item(item_t *item, Area geometry)
{
    if(item == globalconf.item_selected)
        draw_text(globalconf.ctx, geometry, AlignLeft,
                  MARGIN / 2, globalconf.font, item->data,
                  globalconf.shadow_offset,
                  globalconf.fg_focus, globalconf.bg_focus);
    else
        draw_text(globalconf.ctx, geometry, AlignLeft,
                  MARGIN / 2, globalconf.font, item->data,
                  globalconf.shadow_offset,
                  globalconf.fg, globalconf.bg);
}

static void
redraw(void)
{
    item_t *item;
    Area geometry = { 0, 0, 0, 0, NULL };
    unsigned int len;
    Bool selected_item_is_drawn = False;
    int prompt_len, x_of_previous_item;

    geometry.width = globalconf.sw->geometry.width;
    geometry.height = globalconf.sw->geometry.height;

    if(a_strlen(globalconf.prompt))
    {
        draw_text(globalconf.ctx, geometry, AlignLeft,
                  MARGIN, globalconf.font, globalconf.prompt,
                  globalconf.shadow_offset,
                  globalconf.fg_focus, globalconf.bg_focus);

        len = MARGIN * 2 + draw_textwidth(globalconf.display, globalconf.font, globalconf.prompt);
        geometry.x += len;
        geometry.width -= len;
    }

    draw_text(globalconf.ctx, geometry, AlignLeft,
              MARGIN, globalconf.font, globalconf.text,
              globalconf.shadow_offset,
              globalconf.fg, globalconf.bg);

    len = MARGIN * 2 + MAX(draw_textwidth(globalconf.display, globalconf.font, globalconf.text),
                           geometry.width / 20);
    geometry.x += len;
    geometry.width -= len;
    prompt_len = geometry.x;

    for(item = globalconf.items; item && geometry.width > 0; item = item->next)
        if(item->match)
        {
            if(item == globalconf.item_selected)
                selected_item_is_drawn = True;
            draw_item(item, geometry);
            len = MARGIN + draw_textwidth(globalconf.display, globalconf.font, item->data);
            geometry.x += len;
            geometry.width -= len;
        }

    /* we have an item selected but not drawn, so redraw in the other side */
    if(globalconf.item_selected && !selected_item_is_drawn)
    {
        geometry.x = globalconf.sw->geometry.width;

        for(item = globalconf.item_selected; item; item = item_list_prev(&globalconf.items, item))
            if(item->match)
            {
                x_of_previous_item = geometry.x;
                geometry.width = MARGIN + draw_textwidth(globalconf.display, globalconf.font, item->data);
                geometry.x -= geometry.width;

                if(geometry.x < prompt_len)
                    break;

                draw_item(item, geometry);
            }

        if(item)
        {
            geometry.x = prompt_len;
            geometry.width = x_of_previous_item - prompt_len;
            draw_rectangle(globalconf.ctx, geometry, True, globalconf.bg);
        }
    }
    else if(geometry.width)
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
                compute_match(get_last_word(globalconf.text));
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
            compute_match(get_last_word(globalconf.text));
            redraw();
        }
        break;
      case XK_BackSpace:
        if(len)
        {
            globalconf.text[--len] = '\0';
            compute_match(get_last_word(globalconf.text));
            redraw();
        }
        break;
      case XK_Left:
        complete(True);
        redraw();
        break;
      case XK_Right:
      case XK_Tab:
        complete(False);
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

static Bool
item_list_fill_stdin(void)
{
    char buf[PATH_MAX];
    item_t *newitem;
    Bool has_entry = False;

    item_list_init(&globalconf.items);

    if(fgets(buf, sizeof(buf), stdin))
        has_entry = True;

    if(has_entry)
        do
        {
            buf[a_strlen(buf) - 1] = '\0';
            newitem = p_new(item_t, 1);
            newitem->data = a_strdup(buf);
            newitem->match = True;
            item_list_append(&globalconf.items, newitem);
        }
        while(fgets(buf, sizeof(buf), stdin));

    return has_entry;
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

    if((ret = config_parse(configfile, globalconf.prompt, &geometry)))
        return ret;

    /* Get the numlock mask */
    globalconf.numlockmask = get_numlockmask(disp);

    /* Init the geometry */
    if(!geometry.height)
        geometry.height = globalconf.font->height * 1.5;

    /* XXX this must be replace with a common/ infra */
    if(XineramaIsActive(disp))
    {
        XineramaScreenInfo *si;
        int xinerama_screen_number, i, x, y;
        unsigned int ui;
        Window dummy;

        XQueryPointer(disp, RootWindow(disp, DefaultScreen(disp)),
                      &dummy, &dummy, &x, &y, &i, &i, &ui);

        si = XineramaQueryScreens(disp, &xinerama_screen_number);

        /* XXX use screen_get_bycoord() !!! */
        for(i = 0; i < xinerama_screen_number; i++)
            if((x < 0 || (x >= si[i].x_org && x < si[i].x_org + si[i].width))
               && (y < 0 || (y >= si[i].y_org && y < si[i].y_org + si[i].height)))
                break;

        if(!geometry.x)
            geometry.x = si[i].x_org;
        if(!geometry.y)
            geometry.y = si[i].y_org;
        if(!geometry.width)
            geometry.width = si[i].width;

        XFree(si);
    }
    else if(!geometry.width)
        geometry.width = DisplayWidth(disp, DefaultScreen(disp));

    /* Create the window */
    globalconf.sw = simplewindow_new(disp, DefaultScreen(disp),
                                     geometry.x, geometry.y, geometry.width, geometry.height, 0);

    XStoreName(disp, globalconf.sw->window, PROGNAME);
    XMapRaised(disp, globalconf.sw->window);

    /* Create the drawing context */
    globalconf.ctx = draw_context_new(disp, DefaultScreen(disp),
                                      geometry.width, geometry.height,
                                      globalconf.sw->drawable);


    if(!item_list_fill_stdin())
        item_list_fill_file(".");

    compute_match(NULL);

    for(opt = 1000; opt; opt--)
    {
        if(XGrabKeyboard(disp, DefaultRootWindow(disp), True,
                         GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
            break;
        usleep(1000);
    }
    if(!opt)
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

    draw_context_delete(globalconf.ctx);
    simplewindow_delete(globalconf.sw);
    XCloseDisplay(disp);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
