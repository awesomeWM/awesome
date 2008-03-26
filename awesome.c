/*
 * awesome.c - awesome main functions
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

#define _GNU_SOURCE
#include <getopt.h>

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <alloca.h>

#include <xcb/xcb.h>
#include <xcb/shape.h>
#include <xcb/randr.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

#include "config.h"
#include "awesome.h"
#include "event.h"
#include "layout.h"
#include "screen.h"
#include "statusbar.h"
#include "uicb.h"
#include "window.h"
#include "client.h"
#include "focus.h"
#include "ewmh.h"
#include "xcb_event_handler.h"
#include "tag.h"
#include "common/socket.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xscreen.h"
#include "common/xutil.h"

static bool running = true;

AwesomeConf globalconf;

/** Get geometry informations like XCB version, and also set 'x' and
 * 'y' parameter on its parent window like Xlib does
 *
 * TODO: improve round-trip time?
 */
static xcb_get_geometry_reply_t *
x_get_geometry(xcb_connection_t *c, xcb_window_t w, xcb_get_geometry_reply_t *parent)
{
    xcb_get_geometry_reply_t *win_geom;

    win_geom = xcb_get_geometry_reply(c, xcb_get_geometry(c, w), NULL);

    if(!win_geom)
        return NULL;

    /* Unlike XCB, Xlib set 'x' and 'y' as parent window position */
    win_geom->x = parent->x;
    win_geom->y = parent->y;

    return win_geom;
}

typedef struct
{
    xcb_get_geometry_cookie_t geom;
    xcb_query_tree_cookie_t tree;
} screen_win_t;

/** Scan X to find windows to manage
 */
static void
scan()
{
    unsigned int i;
    int screen, real_screen;
    xcb_window_t w;
    xcb_window_t *wins = NULL;
    xcb_query_tree_reply_t *r_query_tree;
    xcb_get_geometry_reply_t *parent_geom, *win_geom;
    xcb_get_window_attributes_reply_t *reply_win;
    xcb_get_window_attributes_cookie_t *cookies_win;
    const int screen_max = xcb_setup_roots_length (xcb_get_setup (globalconf.connection));
    screen_win_t *parents = alloca(sizeof(screen_win_t) * screen_max);

    for(screen = 0; screen < screen_max; screen++)
    {
        w = xutil_root_window(globalconf.connection, screen);

        /* Get parent  geometry informations,  useful to get  the real
         * coordinates  of the  window because  Xlib set  'x'  and 'y'
         * fields to the relative position within the parent window */
        parents[screen].geom = xcb_get_geometry(globalconf.connection, w);

        /* Get the window tree */
        parents[screen].tree = xcb_query_tree_unchecked(globalconf.connection, w);
    }

    for(screen = 0; screen < screen_max; screen++)
    {
        parent_geom = xcb_get_geometry_reply (globalconf.connection,
                                              parents[screen].geom, NULL);

        if(!parent_geom)
            continue;

        r_query_tree = xcb_query_tree_reply(globalconf.connection,
                                            parents[screen].tree, NULL);

        if(!r_query_tree)
        {
            p_delete(&parent_geom);
            continue;
        }

        wins = xcb_query_tree_children(r_query_tree);

        /* Store the answers of 'xcb_get_window_attributes' for all
         * child windows */
        cookies_win = p_new(xcb_get_window_attributes_cookie_t,
                            r_query_tree->children_len);

        /* Send all the requests at the same time */
        for(i = 0; i < r_query_tree->children_len; i++)
            cookies_win[i] = xcb_get_window_attributes(globalconf.connection, wins[i]);

        /* Now process the answers */
        for(i = 0; i < r_query_tree->children_len; i++)
        {
            reply_win = xcb_get_window_attributes_reply(globalconf.connection,
                                                        cookies_win[i],
                                                        NULL);

            if(reply_win && !reply_win->override_redirect &&
               (reply_win->map_state == XCB_MAP_STATE_VIEWABLE ||
                window_getstate(wins[i]) == XCB_WM_ICONIC_STATE))
            {
                /* TODO: should maybe be asynchronous */
                win_geom = x_get_geometry(globalconf.connection, w, parent_geom);
                if(!win_geom)
                {
                    p_delete(&reply_win);
                    continue;
                }

                real_screen = screen_get_bycoord(globalconf.screens_info, screen, win_geom->x, win_geom->y);
                client_manage(wins[i], win_geom, real_screen);

                /* win_geom is not useful anymore */
                p_delete(&win_geom);
            }

            if(reply_win)
                p_delete(&reply_win);
        }

        p_delete(&parent_geom);
        p_delete(&r_query_tree);
        p_delete(&cookies_win);
    }
}

/** Equivalent to 'XCreateFontCursor()', error are handled by the
 * default current error handler
 * \param cursor_font type of cursor to use
 * \return allocated cursor font
 */
static xcb_cursor_t
create_font_cursor(unsigned int cursor_font)
{
    xcb_font_t           font;
    xcb_cursor_t         cursor;

    /* Get the font for the cursor*/
    font = xcb_generate_id(globalconf.connection);
    xcb_open_font(globalconf.connection, font, strlen ("cursor"), "cursor");

    cursor = xcb_generate_id(globalconf.connection);
    xcb_create_glyph_cursor (globalconf.connection, cursor, font, font,
                             cursor_font, cursor_font + 1,
                             0, 0, 0,
                             0, 0, 0);

    return cursor;
}

/** Startup Error handler to check if another window manager
 * is already running.
 * \param data Additional optional parameters data
 * \param c X connection
 * \param error Error event
 */
static int __attribute__ ((noreturn))
xerrorstart(void * data __attribute__ ((unused)),
            xcb_connection_t * c  __attribute__ ((unused)),
            xcb_generic_error_t * error __attribute__ ((unused)))
{
    eprint("another window manager is already running\n");
}

/** Quit awesome.
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_quit(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    running = false;
}

static void
exit_on_signal(int sig __attribute__ ((unused)))
{
    running = false;
}

/** \brief awesome xerror function
 * There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 * \param edpy display ref
 * \param ee XErrorEvent event
 * \return 0 if no error, or xerror's xlib return status
 */
static int
xerror(void *data __attribute__ ((unused)),
       xcb_connection_t *c __attribute__ ((unused)),
       xcb_generic_error_t *e)
{
    /*
     * Get the request code,  taken from 'xcb-util/wm'. I can't figure
     * out  how it  works BTW,  seems to  get a  byte in  'pad' member
     * (second byte in second element of the array)
     */
    uint8_t request_code = (e->response_type == 0 ? *((uint8_t *) e + 10) : e->response_type);

    if(e->error_code == BadWindow
       || (e->error_code == BadMatch && request_code == XCB_SET_INPUT_FOCUS)
       || (e->error_code == BadValue && request_code == XCB_KILL_CLIENT)
       || (request_code == XCB_CONFIGURE_WINDOW && e->error_code == BadMatch))
        return 0;

    warn("fatal error: request=%s, error=%s\n",
         x_label_request[request_code], x_label_error[e->error_code]);

    /*
     * Xlib code was using default X error handler, namely
     * '_XDefaultError()', which displays more informations about the
     * error and also exit if 'error_code'' equals to
     * 'BadImplementation'
     *
     * TODO: display more informations about the error (like the Xlib
     *       default error handler)
     */
    if(e->error_code == BadImplementation)
        exit(1);

    return 0;
}

/** Print help and exit(2) with given exit_code.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile,
"Usage: awesome [OPTION]\n\
  -h, --help             show help\n\
  -v, --version          show version\n\
  -c, --config FILE      configuration file to use\n\
  -k, --check            check configuration file syntax\n\
  -s  --sync             enable synchronization (X debug)\n");
    exit(exit_code);
}

/** Hello, this is main
 * \param argc who knows
 * \param argv who knows
 * \return EXIT_SUCCESS I hope
 */
int
main(int argc, char *argv[])
{
    char buf[1024];
    const char *confpath = NULL;
    int r, xfd, csfd, i, screen_nbr, opt;
    ssize_t cmdlen = 1;
    const xcb_query_extension_reply_t *shape_query, *randr_query;
    Statusbar *statusbar;
    fd_set rd;
    xcb_generic_event_t *ev;
    xcb_connection_t *conn;
    struct sockaddr_un *addr;
    Client *c;
    bool confcheck = false;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"check",   0, NULL, 'k'},
        {"config",  1, NULL, 'c'},
        {"sync",    0, NULL, 's'},
        {NULL,      0, NULL, 0}
    };

    /* save argv */
    for(i = 0; i < argc; i++)
        cmdlen += a_strlen(argv[i]) + 1;

    globalconf.argv = p_new(char, cmdlen);
    a_strcpy(globalconf.argv, cmdlen, argv[0]);

    for(i = 1; i < argc; i++)
    {
        a_strcat(globalconf.argv, cmdlen, " ");
        a_strcat(globalconf.argv, cmdlen, argv[i]);
    }

    /* check args */
    while((opt = getopt_long(argc, argv, "vhkc:s",
                             long_options, NULL)) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version("awesome");
            break;
          case 'h':
            exit_help(EXIT_SUCCESS);
            break;
          case 'c':
            if(a_strlen(optarg))
                confpath = a_strdup(optarg);
            else
                eprint("-c option requires a file name\n");
            break;
          case 'k':
            confcheck = true;
            break;
        }

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    if(confcheck)
        return config_check(confpath);

    /* X stuff */
    conn = xcb_connect(NULL, &(globalconf.default_screen));
    if(xcb_connection_has_error(conn))
        eprint("cannot open display\n");

    xfd = xcb_get_file_descriptor(conn);

    /* Allocate a handler which will holds all errors and events */
    globalconf.evenths = xcb_alloc_event_handlers(conn);
    xcb_set_error_handler_catch_all(globalconf.evenths, xerrorstart, NULL);

    const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(conn));
        screen_nbr++)
        /* this causes an error if some other window manager is running */
        xcb_change_window_attributes(conn,
                                     xutil_root_window(conn, screen_nbr),
                                     XCB_CW_EVENT_MASK, &select_input_val);

    /* need to xcb_flush to validate error handler */
    xcb_aux_sync(conn);

    /* process all errors in the queue if any */
    xcb_poll_for_event_loop(globalconf.evenths);

    /* set the default xerror handler */
    xcb_set_error_handler_catch_all(globalconf.evenths, xerror, NULL);

    /* TODO
    if(xsync)
        XSynchronize(dpy, true);
    */

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(conn);

    /* Get the NumLock, ShiftLock and CapsLock masks */
    xutil_get_lock_mask(conn, globalconf.keysyms, &globalconf.numlockmask,
                        &globalconf.shiftlockmask, &globalconf.capslockmask);

    /* store connection */
    globalconf.connection = conn;

    /* init EWMH atoms */
    ewmh_init_atoms();

    /* init screens struct */
    globalconf.screens_info = screensinfo_new(conn);
    globalconf.screens = p_new(VirtScreen, globalconf.screens_info->nscreen);
    focus_add_client(NULL);

    /* parse config */
    config_parse(confpath);

    /* init cursors */
    globalconf.cursor[CurNormal] = create_font_cursor(CURSOR_LEFT_PTR);
    globalconf.cursor[CurResize] = create_font_cursor(CURSOR_SIZING);
    globalconf.cursor[CurMove] = create_font_cursor(CURSOR_FLEUR);

    /* for each virtual screen */
    for(screen_nbr = 0; screen_nbr < globalconf.screens_info->nscreen; screen_nbr++)
    {
        /* view at least one tag */
        tag_view(globalconf.screens[screen_nbr].tags, true);

        for(statusbar = globalconf.screens[screen_nbr].statusbar; statusbar; statusbar = statusbar->next)
            statusbar_init(statusbar);
    }

    /* select for events */
    const uint32_t change_win_vals[] =
    {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
            | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
            | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
        globalconf.cursor[CurNormal]
    };

    /* do this only for real screen */
    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(conn));
        screen_nbr++)
    {
        xcb_change_window_attributes(globalconf.connection,
                                     xutil_root_window(globalconf.connection, screen_nbr),
                                     XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
                                     change_win_vals);
        ewmh_set_supported_hints(screen_nbr);
        /* call this to at least grab root window clicks */
        window_root_grabbuttons(screen_nbr);
        window_root_grabkeys(screen_nbr);
    }

    /* scan existing windows */
    scan();

    /* process all errors in the queue if any */
    xcb_poll_for_event_loop(globalconf.evenths);

    set_button_press_event_handler(globalconf.evenths, event_handle_buttonpress, NULL);
    set_configure_request_event_handler(globalconf.evenths, event_handle_configurerequest, NULL);
    set_configure_notify_event_handler(globalconf.evenths, event_handle_configurenotify, NULL);
    set_destroy_notify_event_handler(globalconf.evenths, event_handle_destroynotify, NULL);
    set_enter_notify_event_handler(globalconf.evenths, event_handle_enternotify, NULL);
    set_expose_event_handler(globalconf.evenths, event_handle_expose, NULL);
    set_key_press_event_handler(globalconf.evenths, event_handle_keypress, NULL);
    set_mapping_notify_event_handler(globalconf.evenths, event_handle_mappingnotify, NULL);
    set_map_request_event_handler(globalconf.evenths, event_handle_maprequest, NULL);
    set_property_notify_event_handler(globalconf.evenths, event_handle_propertynotify, NULL);
    set_unmap_notify_event_handler(globalconf.evenths, event_handle_unmapnotify, NULL);
    set_client_message_event_handler(globalconf.evenths, event_handle_clientmessage, NULL);

    /* check for shape extension */
    shape_query = xcb_get_extension_data(conn, &xcb_shape_id);
    if((globalconf.have_shape = shape_query->present))
        xcb_set_event_handler(globalconf.evenths, (shape_query->first_event + XCB_SHAPE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_shape, NULL);

    /* check for randr extension */
    randr_query = xcb_get_extension_data(conn, &xcb_randr_id);
    if((globalconf.have_randr = randr_query->present))
        xcb_set_event_handler(globalconf.evenths, (randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

    xcb_aux_sync(conn);

    /* get socket fd */
    csfd = socket_getclient();
    addr = socket_getaddr(getenv("DISPLAY"));

    if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
    {
        if(errno == EADDRINUSE)
        {
            if(unlink(addr->sun_path))
                warn("error unlinking existing file: %s\n", strerror(errno));
            if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                warn("error binding UNIX domain socket: %s\n", strerror(errno));
        }
        else
            warn("error binding UNIX domain socket: %s\n", strerror(errno));
    }

    /* register function for signals */
    signal(SIGINT, &exit_on_signal);
    signal(SIGTERM, &exit_on_signal);
    signal(SIGHUP, &exit_on_signal);

    /* refresh everything before waiting events */
    statusbar_refresh();
    layout_refresh();

    /* main event loop, also reads status text from socket */
    while(running)
    {
        FD_ZERO(&rd);
        if(csfd >= 0)
            FD_SET(csfd, &rd);
        FD_SET(xfd, &rd);
        if(select(MAX(xfd, csfd) + 1, &rd, NULL, NULL, NULL) == -1)
        {
            if(errno == EINTR)
                continue;
            eprint("select failed\n");
        }
        if(csfd >= 0 && FD_ISSET(csfd, &rd))
            switch (r = recv(csfd, buf, sizeof(buf)-1, MSG_TRUNC))
            {
              case -1:
                warn("error reading UNIX domain socket: %s\n", strerror(errno));
                csfd = -1;
                break;
              case 0:
                break;
              default:
                if(r >= ssizeof(buf))
                    break;
                buf[r] = '\0';
                parse_control(buf);
                statusbar_refresh();
                layout_refresh();
            }
        /* two level XPending:
         * we need to first check we have XEvent to handle
         * and if so, we handle them all in a round.
         * Then when we have refresh()'ed stuff so maybe new XEvent
         * are available and select() won't tell us, so let's check
         * with XPending() again.
         */
        while((ev = xcb_poll_for_event(conn)))
        {
            do
            {
                xcb_handle_event(globalconf.evenths, ev);

                /* need to resync */
                xcb_aux_sync(conn);
            } while((ev = xcb_poll_for_event(conn)));

            statusbar_refresh();
            layout_refresh();

            /* need to resync */
            xcb_aux_sync(conn);
        }
    }


    if(csfd > 0 && close(csfd))
        warn("error closing UNIX domain socket: %s\n", strerror(errno));
    if(unlink(addr->sun_path))
        warn("error unlinking UNIX domain socket: %s\n", strerror(errno));
    p_delete(&addr);

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);

    xcb_disconnect(conn);

    return EXIT_SUCCESS;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
