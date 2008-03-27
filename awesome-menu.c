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

/* asprintf() */
#define _GNU_SOURCE

#define CHUNK_SIZE 4096

#include <getopt.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_keysyms.h>

/* XKeysymToString() */
#include <X11/Xlib.h>

#include "common/swindow.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xutil.h"
#include "common/xscreen.c"

#define PROGNAME "awesome-menu"

#define CLEANMASK(mask) (mask & ~(globalconf.numlockmask | XCB_MOD_MASK_LOCK))

/** awesome-menu run status */
typedef enum
{
    /** Stop awesome-menu */
    STOP = 0,
    /** Run awesome-menu */
    RUN = 1,
    /** Stop awesome-menu and cancel any operation */
    CANCEL = 2
} status_t;

/** Is awesome-menu running ? */
static status_t status = RUN;

/** Import awesome config file format */
extern cfg_opt_t awesome_opts[];

/** Item_t typedef */
typedef struct item_t item_t;
/** Item_t structure */
struct item_t
{
    /** Data */
    char *data;
    /** Previous and next elems in item_t list */
    item_t *prev, *next;
    /** True if the item currently matches */
    bool match;
};

/** Destructor for item structure
 * \param item item pointer
 */
static void
item_delete(item_t **item)
{
    p_delete(&(*item)->data);
    p_delete(item);
}

DO_SLIST(item_t, item, item_delete)

/** awesome-run global configuration structure */
typedef struct
{
    /** Connection ref */
    xcb_connection_t *connection;
    /** Default screen number */
    int default_screen;
    /** The window */
    SimpleWindow *sw;
    /** The draw contet */
    DrawCtx *ctx;
    /** Colors */
    struct
    {
        style_t normal;
        style_t focus;
    } styles;
    /** Key symbols */
    xcb_key_symbols_t *keysyms;
    /** Numlock mask */
    unsigned int numlockmask;
    /** Shiftlock mask */
    unsigned int shiftlockmask;
    /** Capslock mask */
    unsigned int capslockmask;
    /** The text */
    char *text;
    /** The text when we asked to complete */
    char *text_complete;
    /** The text length */
    size_t text_size;
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

/** Exit with given exit code
 * \param exit_code exit code
 * \return never returns
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: %s [-c config] [-e command] <message>\n",
            PROGNAME);
    exit(exit_code);
}

/** Parse configuration file and fill up AwesomeMenuConf
 * data structures with configuration directives.
 * \param screen screen number
 * \param confpatharg configuration file pathname, or NULL if auto
 * \param menu_title menu title
 * \param geometry geometry to fill up with supplied information from
 *        configuration file
 * \return cfg_parse status
 */
static int
config_parse(int screen, const char *confpatharg,
             const char *menu_title, area_t *geometry)
{
    int ret, i;
    char *confpath;
    cfg_t *cfg, *cfg_menu = NULL, *cfg_screen = NULL,
          *cfg_styles, *cfg_menu_styles = NULL;

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
        cfg_error(cfg, "W: awesome: parsing configuration file %s failed.\n", confpath);
        break;
    }

    if(ret)
        return ret;

    if(menu_title && !(cfg_menu = cfg_gettsec(cfg, "menu", menu_title)))
        warn("no definition for menu %s in configuration file: using default\n",
             menu_title);
        
    /* get global screen section */
    if(!(cfg_screen = cfg_getnsec(cfg, "screen", screen)))
        cfg_screen = cfg_getsec(cfg, "screen");

    if(cfg_menu)
    {
        cfg_menu_styles = cfg_getsec(cfg_menu, "styles");
        if((i = cfg_getint(cfg_menu, "x")) != (int) 0xffffffff)
           geometry->x = i;
        if((i = cfg_getint(cfg_menu, "y")) != (int) 0xffffffff)
           geometry->y = i;
        if((i = cfg_getint(cfg_menu, "width")) > 0)
           geometry->width = i;
        if((i = cfg_getint(cfg_menu, "height")) > 0)
           geometry->height = i;
    }

    if(cfg_screen
       && (cfg_styles = cfg_getsec(cfg_screen, "styles")))
    {
        /* Grab default styles */
        draw_style_init(globalconf.connection, globalconf.default_screen,
                        cfg_getsec(cfg_styles, "normal"),
                        &globalconf.styles.normal, NULL);
    
        draw_style_init(globalconf.connection, globalconf.default_screen,
                        cfg_getsec(cfg_styles, "focus"),
                        &globalconf.styles.focus, &globalconf.styles.normal);
    }

    /* Now grab menu styles if any */
    if(cfg_menu_styles)
    {
        draw_style_init(globalconf.connection, globalconf.default_screen,
                        cfg_getsec(cfg_menu_styles, "normal"),
                        &globalconf.styles.normal, NULL);

        draw_style_init(globalconf.connection, globalconf.default_screen,
                        cfg_getsec(cfg_menu_styles, "focus"),
                        &globalconf.styles.focus, &globalconf.styles.normal);
    }

    if(!globalconf.styles.normal.font)
        eprint("no default font available\n");

    p_delete(&confpath);

    return ret;
}

/** Return the last word for a text.
 * \param text the text to look into
 * \return a pointer to the last word position in text
 */
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

/** Fill the completion list for awesome-menu with file list.
 * \param directory directory to look into
 * \return always true
 */
static bool
item_list_fill_file(const char *directory)
{
    char *cwd, *home, *user, *filename;
    const char *file;
    DIR *dir;
    struct dirent *dirinfo;
    item_t *item;
    ssize_t len, lenfile;
    struct passwd *passwd = NULL;
    struct stat st;

    item_list_wipe(&globalconf.items);

    if(!directory)
        cwd = a_strdup("./");
    else if(a_strlen(directory) > 1 && directory[0] == '~')
    {
        if(directory[1] == '/')
        {
            home = getenv("HOME");
            asprintf(&cwd, "%s%s", (home ? home : ""), directory + 1);
        }
        else
        {
            if(!(file = strchr(directory, '/')))
                file = directory + a_strlen(directory);
            len = (file - directory) + 1;
            user = p_new(char, len);
            a_strncpy(user, len, directory + 1, (file - directory) - 1);
            if((passwd = getpwnam(user)))
            {
                asprintf(&cwd, "%s%s", passwd->pw_dir, file);
                p_delete(&user);
            }
            else
            {
                p_delete(&user);
                return false;
            }
        }
    }
    else
        cwd = a_strdup(directory);

    if(!(dir = opendir(cwd)))
    {
        p_delete(&cwd);
        return false;
    }

    while((dirinfo = readdir(dir)))
    {
        item = p_new(item_t, 1);

        /* + 1 for \0 + 1 for / if directory */
        len = a_strlen(directory) + a_strlen(dirinfo->d_name) + 2;

        item->data = p_new(char, len);
        if(a_strlen(directory))
            a_strcpy(item->data, len, directory);
        a_strcat(item->data, len, dirinfo->d_name);

        lenfile = a_strlen(cwd) + a_strlen(dirinfo->d_name) + 2;

        filename = p_new(char, lenfile);
        a_strcpy(filename, lenfile, cwd);
        a_strcat(filename, lenfile, dirinfo->d_name);

        if(!stat(filename, &st) && S_ISDIR(st.st_mode))
            a_strcat(item->data, len, "/");

        p_delete(&filename);

        item_list_push(&globalconf.items, item);
    }

    closedir(dir);
    p_delete(&cwd);

    return true;
}

static void
complete(bool reverse)
{
    int loop = 2;
    item_t *item = NULL;
    item_t *(*item_iter)(item_t **, item_t *) = item_list_next_cycle;

    if(reverse)
        item_iter = item_list_prev_cycle;

    if(globalconf.item_selected)
        item = item_iter(&globalconf.items, globalconf.item_selected);
    else
        item = globalconf.items;

    for(; item && loop; item = item_iter(&globalconf.items, item))
    {
        if(item->match)
        {
            a_strcpy(globalconf.text_complete,
                     globalconf.text_size - (globalconf.text_complete - globalconf.text),
                     item->data);
            globalconf.item_selected = item;
            return;
        }
        /* Since loop is 2, it will be 1 at first iter, and then 0 if we
         * get back before matching an item (i.e. no items match) to the
         * first elem: so it will break the loop, otherwise it loops for
         * ever
         */
        if(item == globalconf.items)
            loop--;
    }
}

/** Compute a match from completion list for word.
 * \param word the word to match
 */
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
                item->match = true;
            else
                item->match = false;
    }
    else
    {
        if(a_strlen(globalconf.text))
            item_list_fill_file(NULL);
        for(item = globalconf.items; item; item = item->next)
            item->match = true;
    }
}

/* Why not? */
#define MARGIN 10

/** Redraw the menu. */
static void
redraw(void)
{
    item_t *item;
    area_t geometry = { 0, 0, 0, 0, NULL, NULL };
    bool selected_item_is_drawn = false;
    int len, prompt_len, x_of_previous_item;
    style_t style;

    geometry.width = globalconf.sw->geometry.width;
    geometry.height = globalconf.sw->geometry.height;

    if(a_strlen(globalconf.prompt))
    {
        draw_text(globalconf.ctx, geometry, AlignLeft,
                  MARGIN, globalconf.prompt, globalconf.styles.focus);

        len = MARGIN * 2 + draw_textwidth(globalconf.connection, globalconf.default_screen,
                                          globalconf.styles.focus.font, globalconf.prompt);
        geometry.x += len;
        geometry.width -= len;
    }

    draw_text(globalconf.ctx, geometry, AlignLeft,
              MARGIN, globalconf.text, globalconf.styles.normal);

    len = MARGIN * 2 + MAX(draw_textwidth(globalconf.connection, globalconf.default_screen,
                                          globalconf.styles.normal.font, globalconf.text),
                           geometry.width / 20);
    geometry.x += len;
    geometry.width -= len;
    prompt_len = geometry.x;

    for(item = globalconf.items; item && geometry.width > 0; item = item->next)
        if(item->match)
        {
            style = item == globalconf.item_selected ? globalconf.styles.focus : globalconf.styles.normal;
            len = MARGIN + draw_textwidth(globalconf.connection, globalconf.default_screen,
                                          style.font, item->data);
            if(item == globalconf.item_selected)
            {
                if(len > geometry.width)
                    break;
                else
                    selected_item_is_drawn = true;
            }
            draw_text(globalconf.ctx, geometry, AlignLeft,
                      MARGIN / 2, item->data, style);
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
                style = item == globalconf.item_selected ? globalconf.styles.focus : globalconf.styles.normal;
                x_of_previous_item = geometry.x;
                geometry.width = MARGIN + draw_textwidth(globalconf.connection, globalconf.default_screen,
                                                         style.font, item->data);
                geometry.x -= geometry.width;

                if(geometry.x < prompt_len)
                    break;

                draw_text(globalconf.ctx, geometry, AlignLeft,
                          MARGIN / 2, item->data, style);
            }

        if(item)
        {
            geometry.x = prompt_len;
            geometry.width = x_of_previous_item - prompt_len;
            draw_rectangle(globalconf.ctx, geometry, 1.0, true, globalconf.styles.normal.bg);
        }
    }
    else if(geometry.width)
        draw_rectangle(globalconf.ctx, geometry, 1.0, true, globalconf.styles.normal.bg);

    simplewindow_refresh_drawable(globalconf.sw, globalconf.default_screen);
    xcb_aux_sync(globalconf.connection);
}

/** XCB equivalent of XLookupString which translate the keycode given
 * by PressEvent to a KeySym and a string
 * \todo use XKB!
 */
static void
key_press_lookup_string(xcb_key_press_event_t *e,
                        char **buf, size_t *buf_len,
                        xcb_keysym_t *ksym)
{
    xcb_keysym_t k0, k1;

    /* 'col' (third parameter) is used to get the proper KeySym
     * according to modifier (XCB doesn't provide an equivalent to
     * XLookupString()) */
    k0 = xcb_key_press_lookup_keysym(globalconf.keysyms, e, 0);
    k1 = xcb_key_press_lookup_keysym(globalconf.keysyms, e, 1);

    /* The numlock modifier is on and the second KeySym is a keypad
     * KeySym */
    if((e->state & globalconf.numlockmask) && xcb_is_keypad_key(k1))
    {
        /* The Shift modifier is on, or if the Lock modifier is on and
         * is interpreted as ShiftLock, use the first KeySym */ 
        if((e->state & XCB_MOD_MASK_SHIFT) ||
           (e->state & XCB_MOD_MASK_LOCK && (e->state & globalconf.shiftlockmask)))
            *ksym = k0;
        else
            *ksym = k1;
    }

    /* The Shift and Lock modifers are both off, use the first
     * KeySym */
    else if(!(e->state & XCB_MOD_MASK_SHIFT) && !(e->state & XCB_MOD_MASK_LOCK))
        *ksym = k0;

    /* The Shift modifier is off and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if(!(e->state & XCB_MOD_MASK_SHIFT) && 
            (e->state & XCB_MOD_MASK_LOCK && (e->state & globalconf.capslockmask)))
        /* The first Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        *ksym = k1;

    /* The Shift modifier is on, and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if((e->state & XCB_MOD_MASK_SHIFT) &&
            (e->state & XCB_MOD_MASK_LOCK && (e->state & globalconf.capslockmask)))
        /* The second Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        *ksym = k1;

    /* The Shift modifer is on, or the Lock modifier is on and is
     * interpreted as ShiftLock, or both */
    else if((e->state & XCB_MOD_MASK_SHIFT) ||
            (e->state & XCB_MOD_MASK_LOCK && (e->state & globalconf.shiftlockmask)))
        *ksym = k1;

    if(xcb_is_modifier_key(*ksym) || xcb_is_function_key(*ksym) ||
       xcb_is_pf_key(*ksym) || xcb_is_cursor_key(*ksym) ||
       xcb_is_misc_function_key(*ksym))
    {
        *buf_len = 0;
        return;
    }

    /* Latin1 keysym */
    if(*ksym < 0x80)
        asprintf(buf, "%c", (char) *ksym);
    else
        /* TODO: remove this call in favor of XCB */
        *buf = a_strdup(XKeysymToString(*ksym));

    *buf_len = a_strlen(*buf);
}

/** Handle keypress event in awesome-menu.
 * \param e received XKeyEvent
 */
static void
handle_kpress(xcb_key_press_event_t *e)
{
    char *buf;
    xcb_keysym_t ksym;
    ssize_t len;
    size_t text_dst_len, num;

    len = a_strlen(globalconf.text);

    key_press_lookup_string(e, &buf, &num, &ksym);
    /* Got a special key, see x_lookup_string() */
    if(!num)
        return;        

    if(e->state & XCB_MOD_MASK_CONTROL)
        switch(ksym)
        {
          default:
            p_delete(&buf);
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
            p_delete(&buf);
            return;
        }
    else if(CLEANMASK(e->state) & XCB_MOD_MASK_1)
        switch(ksym)
        {
          default:
            p_delete(&buf);
            return;
          case XK_h:
            ksym = XK_Left;
            break;
          case XK_l:
            ksym = XK_Right;
            break;
        }

    switch(ksym)
    {
      case XK_space:
        globalconf.text_complete = globalconf.text + a_strlen(globalconf.text) + 1;
      default:
        if(num && !iscntrl((int) buf[0]))
        {
            if(buf[0] != '/' || globalconf.text[len - 1] != '/')
            {
                /* Reallocate text string if needed to hold
                 * concatenation of text and buf */
                if((text_dst_len = (a_strlen(globalconf.text) + num - 1)) > globalconf.text_size)
                {
                    globalconf.text_size += ((int) (text_dst_len / globalconf.text_size)) * CHUNK_SIZE;
                    p_realloc(&globalconf.text, globalconf.text_size);
                }
                a_strncat(globalconf.text, globalconf.text_size, buf, num);
            }
            compute_match(get_last_word(globalconf.text));
            redraw();
        }

        p_delete(&buf);
        break;
      case XK_BackSpace:
        if(len)
        {
            globalconf.text[--len] = '\0';
            compute_match(get_last_word(globalconf.text));
            redraw();
        }
        break;
      case XK_ISO_Left_Tab:
      case XK_Left:
        complete(true);
        redraw();
        break;
      case XK_Right:
      case XK_Tab:
        complete(false);
        redraw();
        break;
      case XK_Escape:
        status= CANCEL;
        break;
      case XK_Return:
        status = STOP;
        break;
    }

    p_delete(&buf);
}

#ifndef HAVE_GETLINE
static int
getline(char ** buf, size_t* len, FILE* in)
{
    int i;
    if (*buf) {
        p_delete(buf);
        (*buf) = NULL;
    }
    if (*len)
        *len = 0;

    do {
        p_realloc(buf, *len + 10);
        (*len) += 10;
        for (i = 0; i < 10 && !feof(in); i++) {
            (*buf)[*len - 10 + i] = getchar();
            if ((*buf)[*len - 10 + i] == '\n' ||
                (*buf)[*len - 10 + i] == '\r') {
                return (*len - 10 + i + 1);
            }
        }
    }
    while(!feof(in));
    return -1;
}
#endif

/** Fill the completion by reading on stdin.
 * \return true if something has been filled up, false otherwise.
 */
static bool
item_list_fill_stdin(void)
{
    char *buf = NULL;
    size_t len = 0;
    ssize_t line_len;

    item_t *newitem;
    bool has_entry = false;

    item_list_init(&globalconf.items);

    if((line_len = getline(&buf, &len, stdin)) != -1)
        has_entry = true;

    if(has_entry)
        do
        {
            buf[line_len - 1] = '\0';
            newitem = p_new(item_t, 1);
            newitem->data = a_strdup(buf);
            newitem->match = true;
            item_list_append(&globalconf.items, newitem);
        }
        while((line_len = getline(&buf, &len, stdin)) != -1);

    if(buf)
        p_delete(&buf);

    return has_entry;
}

/** Grab the keyboard.
 */
static void
keyboard_grab(void)
{
    int i;
    xcb_grab_keyboard_reply_t *xgb = NULL;
    for(i = 1000; i; i--)
    {
        if((xgb = xcb_grab_keyboard_reply(globalconf.connection,
                                          xcb_grab_keyboard(globalconf.connection, true,
                                                            xcb_aux_get_screen(globalconf.connection, globalconf.default_screen)->root,
                                                            XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC,
                                                            XCB_GRAB_MODE_ASYNC),
                                          NULL)) != NULL)
        {
            p_delete(&xgb);
            break;
        }
        usleep(1000);
    }
    if(!i)
        eprint("cannot grab keyboard");
}

/** Main function of awesome-menu.
 * \param argc number of elements in argv
 * \param argv arguments array
 * \return EXIT_SUCCESS
 */
int
main(int argc, char **argv)
{
    xcb_generic_event_t *ev;
    int opt, ret, screen = 0;
    xcb_query_pointer_reply_t *xqp = NULL;
    char *configfile = NULL, *cmd;
    ssize_t len;
    const char *shell = getenv("SHELL");
    area_t geometry = { 0, 0, 0, 0, NULL, NULL };
    ScreensInfo *si;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"exec",    0, NULL, 'e'},
        {NULL,      0, NULL, 0}
    };

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

            geometry.x = si->geometry[screen].x;
            geometry.y = si->geometry[screen].y;
            geometry.width = si->geometry[screen].width;
        }
    }
    else
    {
        screen = globalconf.default_screen;
        if(!geometry.width)
            geometry.width = xcb_aux_get_screen(globalconf.connection, globalconf.default_screen)->width_in_pixels;
    }

    if((ret = config_parse(screen, configfile, globalconf.prompt, &geometry)))
        return ret;

    /* Init the geometry */
    if(!geometry.height)
        geometry.height = 1.5 * MAX(globalconf.styles.normal.font->height,
                                    globalconf.styles.focus.font->height);

    screensinfo_delete(&si);

    /* Create the window */
    globalconf.sw = simplewindow_new(globalconf.connection, globalconf.default_screen,
                                     geometry.x, geometry.y, geometry.width, geometry.height, 0);

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.sw->window, WM_NAME, STRING, 8,
                        strlen(PROGNAME), PROGNAME);

    /* Create the drawing context */
    globalconf.ctx = draw_context_new(globalconf.connection, globalconf.default_screen,
                                      geometry.width, geometry.height,
                                      globalconf.sw->drawable);

    /* Allocate a default size for the text on the heap instead of
     * using stack allocation with PATH_MAX (may not been defined
     * according to POSIX). This string size may be increased if
     * needed */
    globalconf.text_complete = globalconf.text = p_new(char, CHUNK_SIZE);
    globalconf.text_size = CHUNK_SIZE;

    if(isatty(STDIN_FILENO))
    {
        if(!item_list_fill_stdin())
            item_list_fill_file(NULL);
        keyboard_grab();
    }
    else
    {
        keyboard_grab();
        if(!item_list_fill_stdin())
            item_list_fill_file(NULL);
    }

    compute_match(NULL);

    redraw();

    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

    /* Get the numlock, capslock and shiftlock mask */
    xutil_get_lock_mask(globalconf.connection, globalconf.keysyms, &globalconf.numlockmask,
                        &globalconf.shiftlockmask, &globalconf.capslockmask);

    xutil_map_raised(globalconf.connection, globalconf.sw->window);

    while(status == RUN)
    {
        while((ev = xcb_poll_for_event(globalconf.connection)))
        {
            /* Skip errors */
            if(ev->response_type == 0)
                continue;

            switch(ev->response_type & 0x7f)
            {
              case XCB_BUTTON_PRESS:
                status = CANCEL;
                break;
              case XCB_KEY_PRESS:
                handle_kpress((xcb_key_press_event_t *) ev);
                break;
              case XCB_EXPOSE:
                if(!((xcb_expose_event_t *) ev)->count)
                    simplewindow_refresh_drawable(globalconf.sw, globalconf.default_screen);
                break;
              default:
                break;
            }

            p_delete(&ev);
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

    xcb_key_symbols_free(globalconf.keysyms);
    p_delete(&globalconf.text);
    draw_context_delete(&globalconf.ctx);
    simplewindow_delete(&globalconf.sw);
    xcb_disconnect(globalconf.connection);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
