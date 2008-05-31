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
#include <sys/time.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>

#include <glib.h>

#include <xcb/xcb.h>
#include <xcb/shape.h>
#include <xcb/randr.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

#include "lua.h"
#include "event.h"
#include "layout.h"
#include "screen.h"
#include "window.h"
#include "client.h"
#include "focus.h"
#include "ewmh.h"
#include "tag.h"
#include "dbus.h"
#include "statusbar.h"
#include "common/socket.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xutil.h"

bool running = true;

awesome_t globalconf;

typedef struct
{
    xcb_window_t id;
    xcb_query_tree_cookie_t tree_cookie;
} root_win_t;

/** Scan X to find windows to manage
 */
static void
scan()
{
    int i, screen, real_screen, tree_c_len;
    const int screen_max = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
    root_win_t *root_wins = p_new(root_win_t, screen_max);
    xcb_query_tree_reply_t *tree_r;
    xcb_window_t *wins = NULL;
    xcb_get_window_attributes_cookie_t *attr_wins = NULL;
    xcb_get_geometry_cookie_t **geom_wins = NULL;
    xcb_get_window_attributes_reply_t *attr_r;
    xcb_get_geometry_reply_t *geom_r;

    for(screen = 0; screen < screen_max; screen++)
    {
        /* Get the root window ID associated to this screen */
        root_wins[screen].id = xcb_aux_get_screen(globalconf.connection, screen)->root;

        /* Get the window tree associated to this screen */
        root_wins[screen].tree_cookie = xcb_query_tree_unchecked(globalconf.connection,
                                                                 root_wins[screen].id);
    }

    for(screen = 0; screen < screen_max; screen++)
    {
        tree_r = xcb_query_tree_reply(globalconf.connection,
                                      root_wins[screen].tree_cookie,
                                      NULL);

        if(!tree_r)
            continue;

        /* Get the tree of the children Windows of the current root
         * Window */
        if(!(wins = xcb_query_tree_children(tree_r)))
            eprint("E: cannot get tree children");
        tree_c_len = xcb_query_tree_children_length(tree_r);
        attr_wins = p_new(xcb_get_window_attributes_cookie_t, tree_c_len);

        for(i = 0; i < tree_c_len; i++)
            attr_wins[i] = xcb_get_window_attributes_unchecked(globalconf.connection,
                                                               wins[i]);

        geom_wins = p_new(xcb_get_geometry_cookie_t *, tree_c_len);

        for(i = 0; i < tree_c_len; i++)
        {
            attr_r = xcb_get_window_attributes_reply(globalconf.connection,
                                                     attr_wins[i],
                                                     NULL);

            if(!attr_r || attr_r->override_redirect ||
               !(attr_r->map_state == XCB_MAP_STATE_VIEWABLE ||
                 window_getstate(wins[i]) == XCB_WM_ICONIC_STATE))
            {
                p_delete(&attr_r);
                continue;
            }

            p_delete(&attr_r);

            /* Get the geometry of the current window */
            geom_wins[i] = p_new(xcb_get_geometry_cookie_t, 1);
            *(geom_wins[i]) = xcb_get_geometry_unchecked(globalconf.connection, wins[i]);
        }

        p_delete(&attr_wins);

        for(i = 0; i < tree_c_len; i++)
        {
            if(!geom_wins[i])
                continue;

            if(!(geom_r = xcb_get_geometry_reply(globalconf.connection,
                                                 *(geom_wins[i]), NULL)))
                continue;

            real_screen = screen_get_bycoord(globalconf.screens_info, screen,
                                             geom_r->x, geom_r->y);

            client_manage(wins[i], geom_r, real_screen);

            p_delete(&geom_r);
            p_delete(&geom_wins[i]);
        }

        p_delete(&geom_wins);
        p_delete(&tree_r);
    }
    p_delete(&root_wins);
}

/** Equivalent to 'XCreateFontCursor()', error are handled by the
 * default current error handler
 * \param cursor_font type of cursor to use
 * \return allocated cursor font
 */
static xcb_cursor_t
create_font_cursor(unsigned int cursor_font)
{
    xcb_font_t font;
    xcb_cursor_t cursor;

    /* Get the font for the cursor*/
    font = xcb_generate_id(globalconf.connection);
    xcb_open_font(globalconf.connection, font, strlen("cursor"), "cursor");

    cursor = xcb_generate_id(globalconf.connection);
    xcb_create_glyph_cursor (globalconf.connection, cursor, font, font,
                             cursor_font, cursor_font + 1,
                             0, 0, 0,
                             65535, 65535, 65535);

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
    eprint("another window manager is already running");
}

/** Function to exit on some signals.
 * \param sig the signal received, unused
 */
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
    xutil_error_t *err = xutil_get_error(e);
    if(!err)
        return 0;

    if(e->error_code == BadWindow
       || (e->error_code == BadMatch && err->request_code == XCB_SET_INPUT_FOCUS)
       || (e->error_code == BadValue && err->request_code == XCB_KILL_CLIENT)
       || (err->request_code == XCB_CONFIGURE_WINDOW && e->error_code == BadMatch))
    {
        xutil_delete_error(err);
        return 0;
    }

    warn("fatal error: request=%s, error=%s", err->request_label, err->error_label);
    xutil_delete_error(err);

    /*
     * Xlib code was using default X error handler, namely
     * '_XDefaultError()', which displays more informations about the
     * error and also exit if 'error_code'' equals to
     * 'BadImplementation'
     *
     * \todo display more informations about the error (like the Xlib default error handler)
     */
    if(e->error_code == BadImplementation)
        exit(EXIT_FAILURE);

    return 0;
}

/** Print help and exit(2) with given exit_code.
 * \param exit_code the exit code
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile,
"Usage: awesome [OPTION]\n\
  -h, --help             show help\n\
  -v, --version          show version\n\
  -c, --config FILE      configuration file to use\n");
    exit(exit_code);
}

/** Hello, this is main.
 * \param argc who knows
 * \param argv who knows
 * \return EXIT_SUCCESS I hope
 */
int
main(int argc, char **argv)
{
    char buf[1024];
    const char *confpath = NULL;
    int r, xfd, csfd, dbusfd, i, screen_nbr, opt;
    ssize_t cmdlen = 1;
    const xcb_query_extension_reply_t *shape_query, *randr_query;
    fd_set rd;
    xcb_generic_event_t *ev;
    struct sockaddr_un *addr;
    client_t *c;
    struct timeval select_timeout, hook_lastrun, now, hook_nextrun;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"config",  1, NULL, 'c'},
        {NULL,      0, NULL, 0}
    };

    /* clear the globalconf structure */
    p_clear(&globalconf, 1);

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
    while((opt = getopt_long(argc, argv, "vhc:",
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
                eprint("-c option requires a file name");
            break;
        }

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* X stuff */
    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        eprint("cannot open display");

    /* Get the file descriptor corresponding to the X connection */
    xfd = xcb_get_file_descriptor(globalconf.connection);

    /* Allocate a handler which will holds all errors and events */
    globalconf.evenths = xcb_alloc_event_handlers(globalconf.connection);
    xutil_set_error_handler_catch_all(globalconf.evenths, xerrorstart, NULL);

    const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
        /* This causes an error if some other window manager is running */
        xcb_change_window_attributes(globalconf.connection,
                                     xcb_aux_get_screen(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK, &select_input_val);

    /* Need to xcb_flush to validate error handler */
    xcb_aux_sync(globalconf.connection);

    /* Process all errors in the queue if any */
    xcb_poll_for_event_loop(globalconf.evenths);

    /* Set the default xerror handler */
    xutil_set_error_handler_catch_all(globalconf.evenths, xerror, NULL);

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

    /* Get the NumLock, ShiftLock and CapsLock masks */
    xutil_getlockmask(globalconf.connection, globalconf.keysyms, &globalconf.numlockmask,
                      &globalconf.shiftlockmask, &globalconf.capslockmask);

    /* init Atoms cache and then EWMH atoms */
    atom_cache_list_init(&globalconf.atoms);
    ewmh_init_atoms();

    /* init screens struct */
    globalconf.screens_info = screensinfo_new(globalconf.connection);
    globalconf.screens = p_new(screen_t, globalconf.screens_info->nscreen);
    focus_client_push(NULL);

    /* parse config */
    if(!confpath)
        confpath = config_file();
    if (!luaA_parserc(confpath))
        eprint("failed to load/parse configuration file %s", confpath);

    /* init cursors */
    globalconf.cursor[CurNormal] = create_font_cursor(CURSOR_LEFT_PTR);
    globalconf.cursor[CurResize] = create_font_cursor(CURSOR_SIZING);
    globalconf.cursor[CurMove] = create_font_cursor(CURSOR_FLEUR);

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
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        xcb_change_window_attributes(globalconf.connection,
                                     xcb_aux_get_screen(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
                                     change_win_vals);
        ewmh_set_supported_hints(screen_nbr);
        /* call this to at least grab root window clicks */
        window_root_grabbuttons();
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
    set_map_request_event_handler(globalconf.evenths, event_handle_maprequest, NULL);
    set_property_notify_event_handler(globalconf.evenths, event_handle_propertynotify, NULL);
    set_unmap_notify_event_handler(globalconf.evenths, event_handle_unmapnotify, NULL);
    set_client_message_event_handler(globalconf.evenths, event_handle_clientmessage, NULL);

    /* check for shape extension */
    shape_query = xcb_get_extension_data(globalconf.connection, &xcb_shape_id);
    if((globalconf.have_shape = shape_query->present))
        xcb_set_event_handler(globalconf.evenths,
                              (shape_query->first_event + XCB_SHAPE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_shape,
                              NULL);

    /* check for randr extension */
    randr_query = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
    if((globalconf.have_randr = randr_query->present))
        xcb_set_event_handler(globalconf.evenths,
                              (randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

    xcb_aux_sync(globalconf.connection);

    /* get socket fd */
    csfd = socket_getclient();
    addr = socket_getaddr(getenv("DISPLAY"));

    if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
    {
        if(errno == EADDRINUSE)
        {
            if(unlink(addr->sun_path))
                warn("error unlinking existing file: %s", strerror(errno));
            if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                warn("error binding UNIX domain socket: %s", strerror(errno));
        }
        else
            warn("error binding UNIX domain socket: %s", strerror(errno));
    }

    if(!a_dbus_init(&dbusfd))
        dbusfd = -1;

    /* register function for signals */
    signal(SIGINT, &exit_on_signal);
    signal(SIGTERM, &exit_on_signal);
    signal(SIGHUP, &exit_on_signal);

    /* refresh everything before waiting events */
    layout_refresh();
    statusbar_refresh();

    /* main event loop */
    while(running)
    {
        FD_ZERO(&rd);
        if(csfd >= 0)
            FD_SET(csfd, &rd);
        if(dbusfd >= 0)
            FD_SET(dbusfd, &rd);
        FD_SET(xfd, &rd);
        if(timerisset(&globalconf.timer))
        {
            gettimeofday(&now, NULL);
            timeradd(&hook_lastrun, &globalconf.timer, &hook_nextrun);
            /* Need to do 2 tests, <= does not work with timercmp() */
            if(timercmp(&hook_nextrun, &now, <) || (timercmp(&hook_nextrun, &now, ==)))
            {
                gettimeofday(&hook_lastrun, NULL);
                luaA_dofunction(globalconf.L, globalconf.hooks.timer, 0);
                select_timeout = globalconf.timer;
            }
            else
                timersub(&hook_nextrun, &now, &select_timeout);
        }
        if(select(MAX(MAX(xfd, csfd), dbusfd) + 1, &rd,
                  NULL, NULL,
                  timerisset(&globalconf.timer) ? &select_timeout : NULL) == -1)
        {
            if(errno == EINTR)
                continue;
            eprint("select failed");
        }
        if(csfd >= 0 && FD_ISSET(csfd, &rd))
            switch(r = recv(csfd, buf, sizeof(buf)-1, MSG_TRUNC))
            {
              case -1:
                warn("error reading UNIX domain socket: %s", strerror(errno));
                csfd = -1;
                break;
              case 0:
                break;
              default:
                if(r >= ssizeof(buf))
                    break;
                buf[r] = '\0';
                luaA_docmd(buf);
            }

        if(dbusfd >= 0 && FD_ISSET(dbusfd, &rd))
            a_dbus_process_requests(&dbusfd);

        /* Two level polling:
         * We need to first check we have an event to handle
         * and if so, we handle them all in a round.
         * Then when we have refresh()'ed stuff so maybe new XEvent
         * are available and select() won't tell us, so let's check
         * with xcb_poll_for_event() again.
         */
        while((ev = xcb_poll_for_event(globalconf.connection)))
        {
            do
            {
                xcb_handle_event(globalconf.evenths, ev);
                /* need to resync */
                xcb_aux_sync(globalconf.connection);
                p_delete(&ev);
            } while((ev = xcb_poll_for_event(globalconf.connection)));

            layout_refresh();
            statusbar_refresh();

            /* need to resync */
            xcb_aux_sync(globalconf.connection);
        }
        layout_refresh();
        statusbar_refresh();
        xcb_aux_sync(globalconf.connection);
    }

    a_dbus_cleanup();

    if(csfd > 0 && close(csfd))
        warn("error closing UNIX domain socket: %s", strerror(errno));
    if(unlink(addr->sun_path))
        warn("error unlinking UNIX domain socket: %s", strerror(errno));
    p_delete(&addr);

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);

    xcb_disconnect(globalconf.connection);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
