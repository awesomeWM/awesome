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

#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <ev.h>
#include <xcb/xcb.h>
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
#include "systray.h"
#include "common/socket.h"
#include "common/version.h"
#include "common/configopts.h"
#include "common/xutil.h"
#include "config.h"

awesome_t globalconf;

typedef struct
{
    xcb_window_t id;
    xcb_query_tree_cookie_t tree_cookie;
} root_win_t;

/** Scan X to find windows to manage
 */
static void
scan(void)
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
    xembed_info_t eminfo;

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

        /* Get the tree of the children windows of the current root window */
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

            if(xembed_info_get(globalconf.connection, wins[i], &eminfo))
                systray_request_handle(wins[i], screen, &eminfo);
            else
                client_manage(wins[i], geom_r, real_screen);

            p_delete(&geom_r);
            p_delete(&geom_wins[i]);
        }

        p_delete(&geom_wins);
        p_delete(&tree_r);
    }
    p_delete(&root_wins);
}

static void
a_xcb_check_cb(EV_P_ ev_check *w, int revents)
{
    xcb_generic_event_t *ev;

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

static void
a_xcb_io_cb(EV_P_ ev_io *w, int revents)
{
    /* Two level polling:
     * We need to first check we have an event to handle
     * and if so, we handle them all in a round.
     * Then when we have refresh()'ed stuff so maybe new XEvent
     * are available and select() won't tell us, so let's check
     * with xcb_poll_for_event() again.
     */
    /* empty */
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
exit_on_signal(EV_P_ ev_signal *w, int revents)
{
    ev_unloop(EV_A_ 1);
}

/** \brief awesome xerror function
 * There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 * \param data currently unused
 * \param c The connectiont to the X server.
 * \param e The error event
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
    const char *confpath = NULL;
    int xfd, i, screen_nbr, opt;
    ssize_t cmdlen = 1;
    const xcb_query_extension_reply_t *randr_query;
    client_t *c;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"config",  1, NULL, 'c'},
        {NULL,      0, NULL, 0}
    };

    /* event loop watchers */
    ev_io xio    = { .fd = -1 };
    ev_check xcheck;
    ev_signal sigint;
    ev_signal sigterm;
    ev_signal sighup;

    /* clear the globalconf structure */
    p_clear(&globalconf, 1);
    globalconf.keygrabber = LUA_REFNIL;

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
    globalconf.loop = ev_default_loop(0);
    ev_timer_init(&globalconf.timer, &luaA_on_timer, 0., 0.);

    /* register function for signals */
    ev_signal_init(&sigint, exit_on_signal, SIGINT);
    ev_signal_init(&sigterm, &exit_on_signal, SIGTERM);
    ev_signal_init(&sighup, &exit_on_signal, SIGHUP);
    ev_signal_start(globalconf.loop, &sigint);
    ev_signal_start(globalconf.loop, &sigterm);
    ev_signal_start(globalconf.loop, &sighup);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);

    /* X stuff */
    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        eprint("cannot open display");

    /* Get the file descriptor corresponding to the X connection */
    xfd = xcb_get_file_descriptor(globalconf.connection);
    ev_io_init(&xio, &a_xcb_io_cb, xfd, EV_READ);
    ev_io_start(globalconf.loop, &xio);
    ev_unref(globalconf.loop);
    ev_check_init(&xcheck, &a_xcb_check_cb);
    ev_check_start(globalconf.loop, &xcheck);
    ev_unref(globalconf.loop);

    /* Allocate a handler which will holds all errors and events */
    globalconf.evenths = xcb_alloc_event_handlers(globalconf.connection);
    xutil_set_error_handler_catch_all(globalconf.evenths, xerrorstart, NULL);

    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

        /* This causes an error if some other window manager is running */
        xcb_change_window_attributes(globalconf.connection,
                                     xcb_aux_get_screen(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK, &select_input_val);
    }

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

    /* init default font and colors */
    globalconf.font = draw_font_new(globalconf.connection, globalconf.default_screen, "sans 8");
    xcolor_new(globalconf.connection, globalconf.default_screen, "black", &globalconf.colors.fg);
    xcolor_new(globalconf.connection, globalconf.default_screen, "white", &globalconf.colors.bg);

    /* init cursors */
    globalconf.cursor[CurNormal] = xutil_cursor_new(globalconf.connection, CURSOR_LEFT_PTR);
    globalconf.cursor[CurResize] = xutil_cursor_new(globalconf.connection, CURSOR_SIZING);
    globalconf.cursor[CurResizeH] = xutil_cursor_new(globalconf.connection, CURSOR_DOUBLE_ARROW_HORIZ);
    globalconf.cursor[CurResizeV] = xutil_cursor_new(globalconf.connection, CURSOR_DOUBLE_ARROW_VERT);
    globalconf.cursor[CurMove] = xutil_cursor_new(globalconf.connection, CURSOR_FLEUR);
    globalconf.cursor[CurTopRight] = xutil_cursor_new(globalconf.connection, CURSOR_TOP_RIGHT_CORNER);
    globalconf.cursor[CurTopLeft] = xutil_cursor_new(globalconf.connection, CURSOR_TOP_LEFT_CORNER);
    globalconf.cursor[CurBotRight] = xutil_cursor_new(globalconf.connection, CURSOR_BOTTOM_RIGHT_CORNER);
    globalconf.cursor[CurBotLeft] = xutil_cursor_new(globalconf.connection, CURSOR_BOTTOM_LEFT_CORNER);

    /* init lua */
    luaA_init();

    /* parse config */
    if(!confpath)
        confpath = config_file();
    if(!luaA_parserc(confpath))
    {
        const char *default_confpath = AWESOME_CONF_PATH "/awesomerc.lua";
        warn("failed to load/parse configuration file %s", confpath);
        warn("falling back to: %s", default_confpath);
        if(!luaA_parserc(default_confpath))
            eprint("failed to load any configuration file");
    }

    /* do this only for real screen */
    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        /* select for events */
        const uint32_t change_win_vals[] =
        {
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
                | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
            globalconf.cursor[CurNormal]
        };

        xcb_change_window_attributes(globalconf.connection,
                                     xcb_aux_get_screen(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
                                     change_win_vals);
        ewmh_set_supported_hints(screen_nbr);
        systray_init(screen_nbr);
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

    /* check for randr extension */
    randr_query = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
    if((globalconf.have_randr = randr_query->present))
        xcb_set_event_handler(globalconf.evenths,
                              (randr_query->first_event + XCB_RANDR_SCREEN_CHANGE_NOTIFY),
                              (xcb_generic_event_handler_t) event_handle_randr_screen_change_notify,
                              NULL);

    xcb_aux_sync(globalconf.connection);

    luaA_cs_init();
    a_dbus_init();

    /* refresh everything before waiting events */
    layout_refresh();
    statusbar_refresh();

    /* main event loop */
    ev_loop(globalconf.loop, 0);

    /* cleanup event loop */
    ev_ref(globalconf.loop);
    ev_check_stop(globalconf.loop, &xcheck);
    ev_ref(globalconf.loop);
    ev_io_stop(globalconf.loop, &xio);
    a_dbus_cleanup();
    luaA_cs_cleanup();

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_aux_sync(globalconf.connection);

    xcb_disconnect(globalconf.connection);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
