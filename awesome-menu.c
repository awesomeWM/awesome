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
    simple_window_t *sw;
    /** The draw contet */
    draw_context_t *ctx;
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
        draw_text(globalconf.ctx, geometry, globalconf.prompt, &globalconf.styles.focus);

        len = MARGIN * 2 + draw_text_extents(globalconf.connection, globalconf.default_screen,
                                             globalconf.styles.focus.font, globalconf.prompt).width;
        geometry.x += len;
        geometry.width -= len;
    }

    draw_text(globalconf.ctx, geometry, globalconf.text, &globalconf.styles.normal);

    len = MARGIN * 2 + MAX(draw_text_extents(globalconf.connection, globalconf.default_screen,
                                             globalconf.styles.normal.font, globalconf.text).width,
                           geometry.width / 20);
    geometry.x += len;
    geometry.width -= len;
    prompt_len = geometry.x;

    for(item = globalconf.items; item && geometry.width > 0; item = item->next)
        if(item->match)
        {
            style = item == globalconf.item_selected ? globalconf.styles.focus : globalconf.styles.normal;
            len = MARGIN + draw_text_extents(globalconf.connection, globalconf.default_screen,
                                             style.font, item->data).width;
            if(item == globalconf.item_selected)
            {
                if(len > geometry.width)
                    break;
                else
                    selected_item_is_drawn = true;
            }
            draw_text(globalconf.ctx, geometry, item->data, &style);
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
                geometry.width = MARGIN + draw_text_extents(globalconf.connection, globalconf.default_screen,
                                                            style.font, item->data).width;
                geometry.x -= geometry.width;

                if(geometry.x < prompt_len)
                    break;

                draw_text(globalconf.ctx, geometry, item->data, &style);
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

    simplewindow_refresh_drawable(globalconf.sw);
    xcb_aux_sync(globalconf.connection);
}

/** XCB equivalent of XLookupString which translate the keycode given
 * by PressEvent to a KeySym and a string
 * \todo use XKB!
 */
static unsigned short const
keysym_to_unicode_1a1_1ff[] =
{
            0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, /* 0x01a0-0x01a7 */
    0x0000, 0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, /* 0x01a8-0x01af */
    0x0000, 0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, /* 0x01b0-0x01b7 */
    0x0000, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, /* 0x01b8-0x01bf */
    0x0154, 0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, /* 0x01c0-0x01c7 */
    0x010c, 0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, /* 0x01c8-0x01cf */
    0x0110, 0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, /* 0x01d0-0x01d7 */
    0x0158, 0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, /* 0x01d8-0x01df */
    0x0155, 0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, /* 0x01e0-0x01e7 */
    0x010d, 0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, /* 0x01e8-0x01ef */
    0x0111, 0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, /* 0x01f0-0x01f7 */
    0x0159, 0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9  /* 0x01f8-0x01ff */
};

static unsigned short const
keysym_to_unicode_2a1_2fe[] =
{
            0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, /* 0x02a0-0x02a7 */
    0x0000, 0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, /* 0x02a8-0x02af */
    0x0000, 0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, /* 0x02b0-0x02b7 */
    0x0000, 0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, /* 0x02b8-0x02bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, /* 0x02c0-0x02c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02c8-0x02cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, /* 0x02d0-0x02d7 */
    0x011c, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, /* 0x02d8-0x02df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, /* 0x02e0-0x02e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02e8-0x02ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, /* 0x02f0-0x02f7 */
    0x011d, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d          /* 0x02f8-0x02ff */
};

static unsigned short const
keysym_to_unicode_3a2_3fe[] =
{
                    0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, /* 0x03a0-0x03a7 */
    0x0000, 0x0000, 0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, /* 0x03a8-0x03af */
    0x0000, 0x0000, 0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, /* 0x03b0-0x03b7 */
    0x0000, 0x0000, 0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, /* 0x03b8-0x03bf */
    0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, /* 0x03c0-0x03c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, /* 0x03c8-0x03cf */
    0x0000, 0x0145, 0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03d0-0x03d7 */
    0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, /* 0x03d8-0x03df */
    0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, /* 0x03e0-0x03e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, /* 0x03e8-0x03ef */
    0x0000, 0x0146, 0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03f0-0x03f7 */
    0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0169, 0x016b          /* 0x03f8-0x03ff */
};

static unsigned short const
keysym_to_unicode_4a1_4df[] =
{
            0x3002, 0x3008, 0x3009, 0x3001, 0x30fb, 0x30f2, 0x30a1, /* 0x04a0-0x04a7 */
    0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, /* 0x04a8-0x04af */
    0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, /* 0x04b0-0x04b7 */
    0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, /* 0x04b8-0x04bf */
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc, /* 0x04c0-0x04c7 */
    0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, /* 0x04c8-0x04cf */
    0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, /* 0x04d0-0x04d7 */
    0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c  /* 0x04d8-0x04df */
};

static unsigned short const
keysym_to_unicode_590_5fe[] =
{
    0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, /* 0x0590-0x0597 */
    0x06f8, 0x06f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x0598-0x059f */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x066a, 0x0670, 0x0679, /* 0x05a0-0x05a7 */
	
    0x067e, 0x0686, 0x0688, 0x0691, 0x060c, 0x0000, 0x06d4, 0x0000, /* 0x05ac-0x05af */
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, /* 0x05b0-0x05b7 */
    0x0668, 0x0669, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, /* 0x05b8-0x05bf */
    0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, /* 0x05c0-0x05c7 */
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, /* 0x05c8-0x05cf */
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, /* 0x05d0-0x05d7 */
    0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x05d8-0x05df */
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, /* 0x05e0-0x05e7 */
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, /* 0x05e8-0x05ef */
    0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0698, 0x06a4, /* 0x05f0-0x05f7 */
    0x06a9, 0x06af, 0x06ba, 0x06be, 0x06cc, 0x06d2, 0x06c1          /* 0x05f8-0x05fe */
};

static unsigned short const
keysym_to_unicode_680_6ff[] =
{
    0x0492, 0x0496, 0x049a, 0x049c, 0x04a2, 0x04ae, 0x04b0, 0x04b2, /* 0x0680-0x0687 */
    0x04b6, 0x04b8, 0x04ba, 0x0000, 0x04d8, 0x04e2, 0x04e8, 0x04ee, /* 0x0688-0x068f */
    0x0493, 0x0497, 0x049b, 0x049d, 0x04a3, 0x04af, 0x04b1, 0x04b3, /* 0x0690-0x0697 */
    0x04b7, 0x04b9, 0x04bb, 0x0000, 0x04d9, 0x04e3, 0x04e9, 0x04ef, /* 0x0698-0x069f */
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, /* 0x06a0-0x06a7 */
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0491, 0x045e, 0x045f, /* 0x06a8-0x06af */
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, /* 0x06b0-0x06b7 */
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0490, 0x040e, 0x040f, /* 0x06b8-0x06bf */
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, /* 0x06c0-0x06c7 */
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, /* 0x06c8-0x06cf */
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, /* 0x06d0-0x06d7 */
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, /* 0x06d8-0x06df */
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, /* 0x06e0-0x06e7 */
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, /* 0x06e8-0x06ef */
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, /* 0x06f0-0x06f7 */
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a  /* 0x06f8-0x06ff */
};

static unsigned short const
keysym_to_unicode_7a1_7f9[] =
{
            0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, /* 0x07a0-0x07a7 */
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, /* 0x07a8-0x07af */
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, /* 0x07b0-0x07b7 */
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07b8-0x07bf */
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, /* 0x07c0-0x07c7 */
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, /* 0x07c8-0x07cf */
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, /* 0x07d0-0x07d7 */
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07d8-0x07df */
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, /* 0x07e0-0x07e7 */
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, /* 0x07e8-0x07ef */
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, /* 0x07f0-0x07f7 */
    0x03c8, 0x03c9                                                  /* 0x07f8-0x07ff */
};

static unsigned short const
keysym_to_unicode_8a4_8fe[] =
{
                                    0x2320, 0x2321, 0x0000, 0x231c, /* 0x08a0-0x08a7 */
    0x231d, 0x231e, 0x231f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08a8-0x08af */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08b0-0x08b7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, /* 0x08b8-0x08bf */
    0x2234, 0x0000, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, /* 0x08c0-0x08c7 */
    0x2245, 0x2246, 0x0000, 0x0000, 0x0000, 0x0000, 0x22a2, 0x0000, /* 0x08c8-0x08cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, /* 0x08d0-0x08d7 */
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, /* 0x08d8-0x08df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e0-0x08e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e8-0x08ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, /* 0x08f0-0x08f7 */
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193          /* 0x08f8-0x08ff */
};

static unsigned short const
keysym_to_unicode_9df_9f8[] =
{
                                                            0x2422, /* 0x09d8-0x09df */
    0x2666, 0x25a6, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000, /* 0x09e0-0x09e7 */
    0x240a, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x2500, /* 0x09e8-0x09ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x251c, 0x2524, 0x2534, 0x252c, /* 0x09f0-0x09f7 */
    0x2502                                                          /* 0x09f8-0x09ff */
};

static unsigned short const
keysym_to_unicode_aa1_afe[] =
{
            0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, /* 0x0aa0-0x0aa7 */
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, /* 0x0aa8-0x0aaf */
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, /* 0x0ab0-0x0ab7 */
    0x2105, 0x0000, 0x0000, 0x2012, 0x2039, 0x2024, 0x203a, 0x0000, /* 0x0ab8-0x0abf */
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, /* 0x0ac0-0x0ac7 */
    0x0000, 0x2122, 0x2120, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25ad, /* 0x0ac8-0x0acf */
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, /* 0x0ad0-0x0ad7 */
    0x0000, 0x271d, 0x0000, 0x220e, 0x25c2, 0x2023, 0x25cf, 0x25ac, /* 0x0ad8-0x0adf */
    0x25e6, 0x25ab, 0x25ae, 0x25b5, 0x25bf, 0x2606, 0x2022, 0x25aa, /* 0x0ae0-0x0ae7 */
    0x25b4, 0x25be, 0x261a, 0x261b, 0x2663, 0x2666, 0x2665, 0x0000, /* 0x0ae8-0x0aef */
    0x2720, 0x2020, 0x2021, 0x2713, 0x2612, 0x266f, 0x266d, 0x2642, /* 0x0af0-0x0af7 */
    0x2640, 0x2121, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e          /* 0x0af8-0x0aff */
};

/* none of the APL keysyms match the Unicode characters */
static unsigned short const
keysym_to_unicode_cdf_cfa[] =
{
                                                            0x2017, /* 0x0cd8-0x0cdf */
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, /* 0x0ce0-0x0ce7 */
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, /* 0x0ce8-0x0cef */
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, /* 0x0cf0-0x0cf7 */
    0x05e8, 0x05e9, 0x05ea                                          /* 0x0cf8-0x0cff */
};

static unsigned short const
keysym_to_unicode_da1_df9[] =
{
            0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, /* 0x0da0-0x0da7 */
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, /* 0x0da8-0x0daf */
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, /* 0x0db0-0x0db7 */
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, /* 0x0db8-0x0dbf */
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, /* 0x0dc0-0x0dc7 */
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, /* 0x0dc8-0x0dcf */
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, /* 0x0dd0-0x0dd7 */
    0x0e38, 0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0e3e, 0x0e3f, /* 0x0dd8-0x0ddf */
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, /* 0x0de0-0x0de7 */
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, /* 0x0de8-0x0def */
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, /* 0x0df0-0x0df7 */
    0x0e58, 0x0e59                                                  /* 0x0df8-0x0dff */
};

static unsigned short const
keysym_to_unicode_ea0_eff[] =
{
    0x0000, 0x1101, 0x1101, 0x11aa, 0x1102, 0x11ac, 0x11ad, 0x1103, /* 0x0ea0-0x0ea7 */
    0x1104, 0x1105, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4, 0x11b5, /* 0x0ea8-0x0eaf */
    0x11b6, 0x1106, 0x1107, 0x1108, 0x11b9, 0x1109, 0x110a, 0x110b, /* 0x0eb0-0x0eb7 */
    0x110c, 0x110d, 0x110e, 0x110f, 0x1110, 0x1111, 0x1112, 0x1161, /* 0x0eb8-0x0ebf */
    0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168, 0x1169, /* 0x0ec0-0x0ec7 */
    0x116a, 0x116b, 0x116c, 0x116d, 0x116e, 0x116f, 0x1170, 0x1171, /* 0x0ec8-0x0ecf */
    0x1172, 0x1173, 0x1174, 0x1175, 0x11a8, 0x11a9, 0x11aa, 0x11ab, /* 0x0ed0-0x0ed7 */
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, /* 0x0ed8-0x0edf */
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, /* 0x0ee0-0x0ee7 */
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x0000, /* 0x0ee8-0x0eef */
    0x0000, 0x0000, 0x1140, 0x0000, 0x0000, 0x1159, 0x119e, 0x0000, /* 0x0ef0-0x0ef7 */
    0x11eb, 0x0000, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9, /* 0x0ef8-0x0eff */
};

static unsigned short const
keysym_to_unicode_12a1_12fe[] =
{
            0x1e02, 0x1e03, 0x0000, 0x0000, 0x0000, 0x1e0a, 0x0000, /* 0x12a0-0x12a7 */
    0x1e80, 0x0000, 0x1e82, 0x1e0b, 0x1ef2, 0x0000, 0x0000, 0x0000, /* 0x12a8-0x12af */
    0x1e1e, 0x1e1f, 0x0000, 0x0000, 0x1e40, 0x1e41, 0x0000, 0x1e56, /* 0x12b0-0x12b7 */
    0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, /* 0x12b8-0x12bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c0-0x12c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c8-0x12cf */
    0x0174, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6a, /* 0x12d0-0x12d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0176, 0x0000, /* 0x12d8-0x12df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e0-0x12e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e8-0x12ef */
    0x0175, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6b, /* 0x12f0-0x12f7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0177          /* 0x12f0-0x12ff */
};
		
static unsigned short const
keysym_to_unicode_13bc_13be[] =
{
                                    0x0152, 0x0153, 0x0178          /* 0x13b8-0x13bf */
};

static unsigned short const
keysym_to_unicode_14a1_14ff[] =
{
            0x2741, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab, /* 0x14a0-0x14a7 */
    0x2014, 0x002e, 0x055d, 0x002c, 0x2013, 0x058a, 0x2026, 0x055c, /* 0x14a8-0x14af */
    0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, /* 0x14b0-0x14b7 */
    0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, /* 0x14b8-0x14bf */
    0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, /* 0x14c0-0x14c7 */
    0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, /* 0x14c8-0x14cf */
    0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, /* 0x14d0-0x14d7 */
    0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, /* 0x14d8-0x14df */
    0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b, /* 0x14e0-0x14e7 */
    0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f, /* 0x14e8-0x14ef */
    0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583, /* 0x14f0-0x14f7 */
    0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x2019, 0x0027, /* 0x14f8-0x14ff */
};

static unsigned short const
keysym_to_unicode_15d0_15f6[] =
{
    0x10d0, 0x10d1, 0x10d2, 0x10d3, 0x10d4, 0x10d5, 0x10d6, 0x10d7, /* 0x15d0-0x15d7 */
    0x10d8, 0x10d9, 0x10da, 0x10db, 0x10dc, 0x10dd, 0x10de, 0x10df, /* 0x15d8-0x15df */
    0x10e0, 0x10e1, 0x10e2, 0x10e3, 0x10e4, 0x10e5, 0x10e6, 0x10e7, /* 0x15e0-0x15e7 */
    0x10e8, 0x10e9, 0x10ea, 0x10eb, 0x10ec, 0x10ed, 0x10ee, 0x10ef, /* 0x15e8-0x15ef */
    0x10f0, 0x10f1, 0x10f2, 0x10f3, 0x10f4, 0x10f5, 0x10f6          /* 0x15f0-0x15f7 */
};

static unsigned short const
keysym_to_unicode_16a0_16f6[] =
{
    0x0000, 0x0000, 0xf0a2, 0x1e8a, 0x0000, 0xf0a5, 0x012c, 0xf0a7, /* 0x16a0-0x16a7 */
    0xf0a8, 0x01b5, 0x01e6, 0x0000, 0x0000, 0x0000, 0x0000, 0x019f, /* 0x16a8-0x16af */
    0x0000, 0x0000, 0xf0b2, 0x1e8b, 0x01d1, 0xf0b5, 0x012d, 0xf0b7, /* 0x16b0-0x16b7 */
    0xf0b8, 0x01b6, 0x01e7, 0x0000, 0x0000, 0x01d2, 0x0000, 0x0275, /* 0x16b8-0x16bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x018f, 0x0000, /* 0x16c0-0x16c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16c8-0x16cf */
    0x0000, 0x1e36, 0xf0d2, 0xf0d3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d0-0x16d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d8-0x16df */
    0x0000, 0x1e37, 0xf0e2, 0xf0e3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e0-0x16e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e8-0x16ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0259          /* 0x16f0-0x16f6 */
};

static unsigned short const
keysym_to_unicode_1e9f_1eff[] =
{
                                                            0x0303,
    0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7, /* 0x1ea0-0x1ea7 */
    0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf, /* 0x1ea8-0x1eaf */
    0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, /* 0x1eb0-0x1eb7 */
    0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf, /* 0x1eb8-0x1ebf */
    0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7, /* 0x1ec0-0x1ec7 */
    0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf, /* 0x1ec8-0x1ecf */
    0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7, /* 0x1ed0-0x1ed7 */
    0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf, /* 0x1ed8-0x1edf */
    0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7, /* 0x1ee0-0x1ee7 */
    0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef, /* 0x1ee8-0x1eef */
    0x1ef0, 0x1ef1, 0x0300, 0x0301, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7, /* 0x1ef0-0x1ef7 */
    0x1ef8, 0x1ef9, 0x01a0, 0x01a1, 0x01af, 0x01b0, 0x0309, 0x0323  /* 0x1ef8-0x1eff */
};

static unsigned short const
keysym_to_unicode_20a0_20ac[] =
{
    0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7, /* 0x20a0-0x20a7 */
    0x20a8, 0x20a9, 0x20aa, 0x20ab, 0x20ac                          /* 0x20a8-0x20af */
};

static char *
keysym_to_utf8(const xcb_keysym_t ksym)
{
    unsigned int ksym_conv;
    int count;
    char *buf;

    /* Unicode keysym */
    if((ksym & 0xff000000) == 0x01000000)
        ksym_conv = ksym & 0x00ffffff;
    else if(ksym > 0 && ksym < 0x100)
	ksym_conv = ksym;
    else if(ksym > 0x1a0 && ksym < 0x200)
	ksym_conv = keysym_to_unicode_1a1_1ff[ksym - 0x1a1];
    else if(ksym > 0x2a0 && ksym < 0x2ff)
	ksym_conv = keysym_to_unicode_2a1_2fe[ksym - 0x2a1];
    else if(ksym > 0x3a1 && ksym < 0x3ff)
	ksym_conv = keysym_to_unicode_3a2_3fe[ksym - 0x3a2];
    else if(ksym > 0x4a0 && ksym < 0x4e0)
	ksym_conv = keysym_to_unicode_4a1_4df[ksym - 0x4a1];
    else if(ksym > 0x589 && ksym < 0x5ff)
	ksym_conv = keysym_to_unicode_590_5fe[ksym - 0x590];
    else if(ksym > 0x67f && ksym < 0x700)
	ksym_conv = keysym_to_unicode_680_6ff[ksym - 0x680];
    else if(ksym > 0x7a0 && ksym < 0x7fa)
	ksym_conv = keysym_to_unicode_7a1_7f9[ksym - 0x7a1];
    else if(ksym > 0x8a3 && ksym < 0x8ff)
	ksym_conv = keysym_to_unicode_8a4_8fe[ksym - 0x8a4];
    else if(ksym > 0x9de && ksym < 0x9f9)
	ksym_conv = keysym_to_unicode_9df_9f8[ksym - 0x9df];
    else if(ksym > 0xaa0 && ksym < 0xaff)
	ksym_conv = keysym_to_unicode_aa1_afe[ksym - 0xaa1];
    else if(ksym > 0xcde && ksym < 0xcfb)
	ksym_conv = keysym_to_unicode_cdf_cfa[ksym - 0xcdf];
    else if(ksym > 0xda0 && ksym < 0xdfa)
	ksym_conv = keysym_to_unicode_da1_df9[ksym - 0xda1];
    else if(ksym > 0xe9f && ksym < 0xf00)
	ksym_conv = keysym_to_unicode_ea0_eff[ksym - 0xea0];
    else if(ksym > 0x12a0 && ksym < 0x12ff)
	ksym_conv = keysym_to_unicode_12a1_12fe[ksym - 0x12a1];
    else if(ksym > 0x13bb && ksym < 0x13bf)
	ksym_conv = keysym_to_unicode_13bc_13be[ksym - 0x13bc];
    else if(ksym > 0x14a0 && ksym < 0x1500)
        ksym_conv = keysym_to_unicode_14a1_14ff[ksym - 0x14a1];
    else if(ksym > 0x15cf && ksym < 0x15f7)
	ksym_conv = keysym_to_unicode_15d0_15f6[ksym - 0x15d0];
    else if(ksym > 0x169f && ksym < 0x16f7)
	ksym_conv = keysym_to_unicode_16a0_16f6[ksym - 0x16a0];
    else if(ksym > 0x1e9e && ksym < 0x1f00)
	ksym_conv = keysym_to_unicode_1e9f_1eff[ksym - 0x1e9f];
    else if(ksym > 0x209f && ksym < 0x20ad)
	ksym_conv = keysym_to_unicode_20a0_20ac[ksym - 0x20a0];
    else 
        return NULL;

    if(ksym_conv < 0x80)
        count = 1;
    else if(ksym_conv < 0x800)
        count = 2;
    else if(ksym_conv < 0x10000)
        count = 3;
    else if(ksym_conv < 0x200000)
        count = 4;
    else if(ksym_conv < 0x4000000)
        count = 5;
    else if(ksym_conv <= 0x7fffffff)
        count = 6;
    else
        return NULL;

    buf = p_new(char, count + 1);

    switch (count)
    {
    case 6:
        buf[5] = 0x80 | (ksym_conv & 0x3f);
        ksym_conv = ksym_conv >> 6;
        ksym_conv |= 0x4000000;
    case 5:
        buf[4] = 0x80 | (ksym_conv & 0x3f);
        ksym_conv = ksym_conv >> 6;
        ksym_conv |= 0x200000;
    case 4:
        buf[3] = 0x80 | (ksym_conv & 0x3f);
        ksym_conv = ksym_conv >> 6;
        ksym_conv |= 0x10000;
    case 3:
        buf[2] = 0x80 | (ksym_conv & 0x3f);
        ksym_conv = ksym_conv >> 6;
        ksym_conv |= 0x800;
    case 2: 
        buf[1] = 0x80 | (ksym_conv & 0x3f);
        ksym_conv = ksym_conv >> 6;
        ksym_conv |= 0xc0;
    case 1:
        buf[0] = ksym_conv;
    }

    buf[count] = '\0';
    return buf;
}

static char *
keysym_to_str(const xcb_keysym_t ksym)
{
    char *buf;

    /* Try to convert to Latin-1, handling ctrl */
    if(!(((ksym >= XK_BackSpace) && (ksym <= XK_Clear)) ||
         (ksym == XK_Return) || (ksym == XK_Escape) ||
         (ksym == XK_KP_Space) || (ksym == XK_KP_Tab) ||
         (ksym == XK_KP_Enter) ||
         ((ksym >= XK_KP_Multiply) && (ksym <= XK_KP_9)) ||
         (ksym == XK_KP_Equal) ||
         (ksym == XK_Delete)))
        return NULL;

    buf = p_new(char, 2);

    /* If X KeySym, convert to ascii by grabbing low 7 bits */
    if(ksym == XK_KP_Space)
        /* Patch encoding botch */
        buf[0] = XK_space & 0x7F;
    else if(ksym == XK_hyphen)
        /* Map to equivalent character */
        buf[0] = (char)(XK_minus & 0xFF);
    else
        buf[0] = (char)(ksym & 0x7F);

    buf[1] = '\0';
    return buf;
}

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

    /* Handle special KeySym (Tab, Newline...) */
    if((*ksym & 0xffffff00) == 0xff00)
        *buf = keysym_to_str(*ksym);
    /* Handle other KeySym (like unicode...) */
    else
        *buf = keysym_to_utf8(*ksym);

    *buf_len = a_strlen(*buf);
}

/** Handle keypress event in awesome-menu.
 * \param e received XKeyEvent
 */
static void
handle_kpress(xcb_key_press_event_t *e)
{
    char *buf;
    xcb_keysym_t ksym = 0;
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
        status = CANCEL;
        break;
      case XK_Return:
        status = STOP;
        break;
    }

    p_delete(&buf);
}

#ifndef HAVE_GETLINE
static ssize_t
my_getline(char **lineptr, size_t *n, FILE *stream)
{
    char *ret = NULL;
    
    if(lineptr == NULL || n == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if(*n == 0)
    {
        *lineptr = p_new(char, CHUNK_SIZE);
        *n = CHUNK_SIZE;
    }

    ret = fgets(*lineptr, *n, stream);
    while(ret != NULL && (*lineptr)[a_strlen (*lineptr) - 1] != '\n')
    {
        *n += CHUNK_SIZE;
        *lineptr = realloc(*lineptr, *n);

        ret = fgets(*lineptr + a_strlen(*lineptr), CHUNK_SIZE, stream);
    }

    if (ret == NULL)
        return -1;

    return a_strlen(*lineptr);
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
    screens_info_t *si;
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

    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

    /* Get the numlock, capslock and shiftlock mask */
    xutil_getlockmask(globalconf.connection, globalconf.keysyms, &globalconf.numlockmask,
                      &globalconf.shiftlockmask, &globalconf.capslockmask);

    xcb_map_window(globalconf.connection, globalconf.sw->window);
    redraw();

    while(status == RUN)
    {
        ev = xcb_wait_for_event(globalconf.connection);
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
                simplewindow_refresh_drawable(globalconf.sw);
            break;
          default:
            break;
        }
        p_delete(&ev);
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
