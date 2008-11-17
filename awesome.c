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

#include <getopt.h>

#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <ev.h>

#include <xcb/xcb_event.h>

#include "awesome.h"
#include "client.h"
#include "window.h"
#include "ewmh.h"
#include "dbus.h"
#include "systray.h"
#include "event.h"
#include "property.h"
#include "screen.h"
#include "common/version.h"
#include "common/atoms.h"
#include "config.h"

awesome_t globalconf;

typedef struct
{
    xcb_window_t id;
    xcb_query_tree_cookie_t tree_cookie;
} root_win_t;

/** Call before exiting.
 */
void
awesome_atexit(void)
{
    client_t *c;
    xembed_window_t *em;
    int screen_nbr;

    a_dbus_cleanup();
    luaA_cs_cleanup();

    /* reparent systray windows, otherwise they may die with their master */
    for(em = globalconf.embedded; em; em = em->next)
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection, em->phys_screen);
        xembed_window_unembed(globalconf.connection, em->win, s->root);
    }

    /* do this only for real screen */
    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
        systray_cleanup(screen_nbr);

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    xcb_flush(globalconf.connection);

    xcb_disconnect(globalconf.connection);
}

/** Scan X to find windows to manage.
 */
static void
scan(void)
{
    int i, screen, phys_screen, tree_c_len;
    const int screen_max = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
    root_win_t root_wins[screen_max];
    xcb_query_tree_reply_t *tree_r;
    xcb_window_t *wins = NULL;
    xcb_get_window_attributes_reply_t *attr_r;
    xcb_get_geometry_reply_t *geom_r;
    long state;

    for(phys_screen = 0; phys_screen < screen_max; phys_screen++)
    {
        /* Get the root window ID associated to this screen */
        root_wins[phys_screen].id = xutil_screen_get(globalconf.connection, phys_screen)->root;

        /* Get the window tree associated to this screen */
        root_wins[phys_screen].tree_cookie = xcb_query_tree_unchecked(globalconf.connection,
                                                                      root_wins[phys_screen].id);
    }

    for(phys_screen = 0; phys_screen < screen_max; phys_screen++)
    {
        tree_r = xcb_query_tree_reply(globalconf.connection,
                                      root_wins[phys_screen].tree_cookie,
                                      NULL);

        if(!tree_r)
            continue;

        /* Get the tree of the children windows of the current root window */
        if(!(wins = xcb_query_tree_children(tree_r)))
            fatal("E: cannot get tree children");

        tree_c_len = xcb_query_tree_children_length(tree_r);
        xcb_get_window_attributes_cookie_t attr_wins[tree_c_len];
        xcb_get_property_cookie_t state_wins[tree_c_len];

        for(i = 0; i < tree_c_len; i++)
        {
            attr_wins[i] = xcb_get_window_attributes_unchecked(globalconf.connection,
                                                               wins[i]);

            state_wins[i] = window_state_get_unchecked(wins[i]);
        }

        xcb_get_geometry_cookie_t *geom_wins[tree_c_len];

        for(i = 0; i < tree_c_len; i++)
        {
            bool has_awesome_prop;

            attr_r = xcb_get_window_attributes_reply(globalconf.connection,
                                                     attr_wins[i],
                                                     NULL);

            state = window_state_get_reply(state_wins[i]);

            has_awesome_prop = xutil_text_prop_get(globalconf.connection, wins[i],
                                                   _AWESOME_TAGS, NULL, NULL);

            if(!attr_r || attr_r->override_redirect
               || (attr_r->map_state != XCB_MAP_STATE_VIEWABLE && !has_awesome_prop)
               || (state == XCB_WM_STATE_WITHDRAWN && !has_awesome_prop))
            {
                geom_wins[i] = NULL;
                p_delete(&attr_r);
                continue;
            }

            p_delete(&attr_r);

            /* Get the geometry of the current window */
            geom_wins[i] = p_alloca(xcb_get_geometry_cookie_t, 1);
            *(geom_wins[i]) = xcb_get_geometry_unchecked(globalconf.connection, wins[i]);
        }

        for(i = 0; i < tree_c_len; i++)
        {
            if(!geom_wins[i] || !(geom_r = xcb_get_geometry_reply(globalconf.connection,
                                                                  *(geom_wins[i]), NULL)))
                continue;

            screen = screen_getbycoord(phys_screen, geom_r->x, geom_r->y);

            client_manage(wins[i], geom_r, phys_screen, screen);

            p_delete(&geom_r);
        }

        p_delete(&tree_r);
    }
}

static void
a_xcb_check_cb(EV_P_ ev_check *w, int revents)
{
    xcb_event_poll_for_event_loop(&globalconf.evenths);
    awesome_refresh(globalconf.connection);
}

static void
a_xcb_io_cb(EV_P_ ev_io *w, int revents)
{
    /* empty */
}

/** Startup Error handler to check if another window manager
 * is already running.
 * \param data Additional optional parameters data.
 * \param c X connection.
 * \param error Error event.
 */
static int __attribute__ ((noreturn))
xerrorstart(void * data __attribute__ ((unused)),
            xcb_connection_t * c  __attribute__ ((unused)),
            xcb_generic_error_t * error __attribute__ ((unused)))
{
    fatal("another window manager is already running");
}

/** Function to exit on some signals.
 * \param sig the signal received, unused
 */
static void
exit_on_signal(EV_P_ ev_signal *w, int revents)
{
    ev_unloop(EV_A_ 1);
}

void
awesome_restart(void)
{
    awesome_atexit();
    a_exec(globalconf.argv);
}

/** Function to restart aweome on some signals.
 * \param sig the signam received, unused
 */
static void
restart_on_signal(EV_P_ ev_signal *w, int revents)
{
    awesome_restart();
}

/** \brief awesome xerror function.
 * There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 * \param data Currently unused.
 * \param c The connectiont to the X server.
 * \param e The error event.
 * \return 0 if no error, or xerror's xlib return status.
 */
static int
xerror(void *data __attribute__ ((unused)),
       xcb_connection_t *c __attribute__ ((unused)),
       xcb_generic_error_t *e)
{
    xutil_error_t err;

    if(!xutil_error_init(e, &err))
        return 0;

    /* ignore this */
    if(e->error_code == XUTIL_BAD_WINDOW
       || (e->error_code == XUTIL_BAD_MATCH && err.request_code == XCB_SET_INPUT_FOCUS)
       || (e->error_code == XUTIL_BAD_VALUE && err.request_code == XCB_KILL_CLIENT)
       || (err.request_code == XCB_CONFIGURE_WINDOW && e->error_code == XUTIL_BAD_MATCH))
        goto bailout;

    warn("X error: request=%s, error=%s", err.request_label, err.error_label);

    /*
     * Xlib code was using default X error handler, namely
     * '_XDefaultError()', which displays more informations about the
     * error and also exit if 'error_code'' equals to
     * 'BadImplementation'
     *
     * \todo display more informations about the error (like the Xlib default error handler)
     */
    if(e->error_code == XUTIL_BAD_IMPLEMENTATION)
       fatal("X error: request=%s, error=%s", err.request_label, err.error_label);

  bailout:
    xutil_error_wipe(&err);
    return 0;
}

/** Print help and exit(2) with given exit_code.
 * \param exit_code The exit code.
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
  -k, --check            check configuration file syntax\n");
    exit(exit_code);
}

/** Hello, this is main.
 * \param argc Who knows.
 * \param argv Who knows.
 * \return EXIT_SUCCESS I hope.
 */
int
main(int argc, char **argv)
{
    const char *confpath = NULL;
    int xfd, i, screen_nbr, opt, colors_nbr;
    xcolor_init_request_t colors_reqs[2];
    xcb_get_modifier_mapping_cookie_t xmapping_cookie;
    ssize_t cmdlen = 1;
    static struct option long_options[] =
    {
        { "help",    0, NULL, 'h' },
        { "version", 0, NULL, 'v' },
        { "config",  1, NULL, 'c' },
        { "check",   0, NULL, 'k' },
        { NULL,      0, NULL, 0 }
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

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* init lua */
    luaA_init();

    /* check args */
    while((opt = getopt_long(argc, argv, "vhkc:",
                             long_options, NULL)) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version("awesome");
            break;
          case 'h':
            exit_help(EXIT_SUCCESS);
            break;
          case 'k':
            if(!luaA_parserc(confpath, false))
            {
                fprintf(stderr, "✘ Configuration file syntax error.\n");
                return EXIT_FAILURE;
            }
            else
            {
                fprintf(stderr, "✔ Configuration file syntax OK.\n");
                return EXIT_SUCCESS;
            }
          case 'c':
            if(a_strlen(optarg))
                confpath = a_strdup(optarg);
            else
                fatal("-c option requires a file name");
            break;
        }

    globalconf.loop = ev_default_loop(0);
    ev_timer_init(&globalconf.timer, &luaA_on_timer, 0., 0.);

    /* register function for signals */
    ev_signal_init(&sigint, exit_on_signal, SIGINT);
    ev_signal_init(&sigterm, exit_on_signal, SIGTERM);
    ev_signal_init(&sighup, restart_on_signal, SIGHUP);
    ev_signal_start(globalconf.loop, &sigint);
    ev_signal_start(globalconf.loop, &sigterm);
    ev_signal_start(globalconf.loop, &sighup);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);

    /* X stuff */
    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        fatal("cannot open display");

    /* Grab server */
    xcb_grab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* Get the file descriptor corresponding to the X connection */
    xfd = xcb_get_file_descriptor(globalconf.connection);
    ev_io_init(&xio, &a_xcb_io_cb, xfd, EV_READ);
    ev_io_start(globalconf.loop, &xio);
    ev_check_init(&xcheck, &a_xcb_check_cb);
    ev_check_start(globalconf.loop, &xcheck);
    ev_unref(globalconf.loop);

    /* Allocate a handler which will holds all errors and events */
    xcb_event_handlers_init(globalconf.connection, &globalconf.evenths);
    xutil_error_handler_catch_all_set(&globalconf.evenths, xerrorstart, NULL);

    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

        /* This causes an error if some other window manager is running */
        xcb_change_window_attributes(globalconf.connection,
                                     xutil_screen_get(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK, &select_input_val);
    }

    /* Need to xcb_flush to validate error handler */
    xcb_aux_sync(globalconf.connection);

    /* Process all errors in the queue if any */
    xcb_event_poll_for_event_loop(&globalconf.evenths);

    /* Set the default xerror handler */
    xutil_error_handler_catch_all_set(&globalconf.evenths, xerror, NULL);

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

    /* Send the request to get the NumLock, ShiftLock and CapsLock
       masks */
    xmapping_cookie = xcb_get_modifier_mapping_unchecked(globalconf.connection);

    /* init atom cache */
    atoms_init(globalconf.connection);

    /* init screens information */
    screen_scan();

    /* init default font and colors */
    colors_reqs[0] = xcolor_init_unchecked(&globalconf.colors.fg,
                                           "black", sizeof("black") - 1);

    colors_reqs[1] = xcolor_init_unchecked(&globalconf.colors.bg,
                                           "white", sizeof("white") - 1);

    globalconf.font = draw_font_new("sans 8");

    /* init cursors */
    globalconf.cursor[CurNormal] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_LEFT_PTR);
    globalconf.cursor[CurResize] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_SIZING);
    globalconf.cursor[CurResizeH] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_DOUBLE_ARROW_HORIZ);
    globalconf.cursor[CurResizeV] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_DOUBLE_ARROW_VERT);
    globalconf.cursor[CurMove] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_FLEUR);
    globalconf.cursor[CurTopRight] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_TOP_RIGHT_CORNER);
    globalconf.cursor[CurTopLeft] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_TOP_LEFT_CORNER);
    globalconf.cursor[CurBotRight] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_BOTTOM_RIGHT_CORNER);
    globalconf.cursor[CurBotLeft] = xutil_cursor_new(globalconf.connection, XUTIL_CURSOR_BOTTOM_LEFT_CORNER);

    for(colors_nbr = 0; colors_nbr < 2; colors_nbr++)
        xcolor_init_reply(colors_reqs[colors_nbr]);

    /* Process the reply of previously sent mapping request */
    xutil_lock_mask_get(globalconf.connection, xmapping_cookie,
                        globalconf.keysyms, &globalconf.numlockmask,
                        &globalconf.shiftlockmask, &globalconf.capslockmask);

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
                | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                | XCB_EVENT_MASK_PROPERTY_CHANGE,
            globalconf.cursor[CurNormal]
        };

        xcb_change_window_attributes(globalconf.connection,
                                     xutil_screen_get(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK | XCB_CW_CURSOR,
                                     change_win_vals);
        ewmh_init(screen_nbr);
        systray_init(screen_nbr);
    }

    /* Parse and run configuration file */
    luaA_parserc(confpath, true);

    /* scan existing windows */
    scan();

    /* process all errors in the queue if any */
    xcb_event_poll_for_event_loop(&globalconf.evenths);
    a_xcb_set_event_handlers();
    a_xcb_set_property_handlers();

    /* Grab server */
    xcb_ungrab_server(globalconf.connection);

    xcb_aux_sync(globalconf.connection);

    luaA_cs_init();
    a_dbus_init();

    /* refresh everything before waiting events */
    awesome_refresh(globalconf.connection);

    /* main event loop */
    ev_loop(globalconf.loop, 0);

    /* cleanup event loop */
    ev_ref(globalconf.loop);
    ev_check_stop(globalconf.loop, &xcheck);
    ev_ref(globalconf.loop);
    ev_io_stop(globalconf.loop, &xio);

    awesome_atexit();

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
