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
#include <sys/time.h>

#include <xcb/bigreq.h>
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#include <xcb/xinerama.h>
#include <xcb/xtest.h>
#include <xcb/shape.h>

#include <glib-unix.h>

#include "awesome.h"
#include "spawn.h"
#include "objects/client.h"
#include "xwindow.h"
#include "ewmh.h"
#include "dbus.h"
#include "systray.h"
#include "event.h"
#include "property.h"
#include "screen.h"
#include "luaa.h"
#include "common/version.h"
#include "common/atoms.h"
#include "common/xcursor.h"
#include "common/xutil.h"
#include "common/backtrace.h"

awesome_t globalconf;

/** argv used to run awesome */
static char *awesome_argv;

/** time of last main loop wakeup */
static struct timeval last_wakeup;

/** current limit for the main loop's runtime */
static float main_loop_iteration_limit = 0.1;

/** Call before exiting.
 */
void
awesome_atexit(bool restart)
{
    lua_pushboolean(globalconf.L, restart);
    signal_object_emit(globalconf.L, &global_signals, "exit", 1);

    /* Move clients where we want them to be and keep the stacking order intact */
    foreach(c, globalconf.stack)
    {
        xcb_reparent_window(globalconf.connection, (*c)->window, globalconf.screen->root,
                (*c)->geometry.x, (*c)->geometry.y);
    }

    /* Save the client order.  This is useful also for "hard" restarts. */
    xcb_window_t *wins = p_alloca(xcb_window_t, globalconf.clients.len);
    int n = 0;
    foreach(client, globalconf.clients)
        wins[n++] = (*client)->window;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        globalconf.screen->root,
                        AWESOME_CLIENT_ORDER, XCB_ATOM_WINDOW, 32, n, wins);

    a_dbus_cleanup();

    systray_cleanup();

    /* Close Lua */
    lua_close(globalconf.L);

    /* X11 is a great protocol. There is a save-set so that reparenting WMs
     * don't kill clients when they shut down. However, when a focused windows
     * is saved, the focus will move to its parent with revert-to none.
     * Immediately afterwards, this parent is destroyed and the focus is gone.
     * Work around this by placing the focus where we like it to be.
     */
    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
            XCB_NONE, XCB_CURRENT_TIME);
    xcb_aux_sync(globalconf.connection);

    /* Disconnect *after* closing lua */
    xcb_cursor_context_free(globalconf.cursor_ctx);
    xcb_disconnect(globalconf.connection);
}

/** Restore the client order after a restart */
static void
restore_client_order(xcb_get_property_cookie_t prop_cookie)
{
    int client_idx = 0;
    xcb_window_t *windows;
    xcb_get_property_reply_t *reply;

    reply = xcb_get_property_reply(globalconf.connection, prop_cookie, NULL);
    if (!reply || reply->format != 32 || reply->value_len == 0) {
        p_delete(&reply);
        return;
    }

    windows = xcb_get_property_value(reply);
    for (uint32_t i = 0; i < reply->value_len; i++)
        /* Find windows[i] and swap it to where it belongs */
        foreach(c, globalconf.clients)
            if ((*c)->window == windows[i])
            {
                client_t *tmp = *c;
                *c = globalconf.clients.tab[client_idx];
                globalconf.clients.tab[client_idx] = tmp;
                client_idx++;
            }

    luaA_class_emit_signal(globalconf.L, &client_class, "list", 0);
    p_delete(&reply);
}

/** Scan X to find windows to manage.
 */
static void
scan(xcb_query_tree_cookie_t tree_c)
{
    int i, tree_c_len;
    xcb_query_tree_reply_t *tree_r;
    xcb_window_t *wins = NULL;
    xcb_get_window_attributes_reply_t *attr_r;
    xcb_get_geometry_reply_t *geom_r;
    xcb_get_property_cookie_t prop_cookie;
    long state;

    tree_r = xcb_query_tree_reply(globalconf.connection,
                                  tree_c,
                                  NULL);

    if(!tree_r)
        return;

    /* This gets the property and deletes it */
    prop_cookie = xcb_get_property_unchecked(globalconf.connection, true,
                          globalconf.screen->root, AWESOME_CLIENT_ORDER,
                          XCB_ATOM_WINDOW, 0, UINT_MAX);

    /* Get the tree of the children windows of the current root window */
    if(!(wins = xcb_query_tree_children(tree_r)))
        fatal("cannot get tree children");

    tree_c_len = xcb_query_tree_children_length(tree_r);
    xcb_get_window_attributes_cookie_t attr_wins[tree_c_len];
    xcb_get_property_cookie_t state_wins[tree_c_len];

    for(i = 0; i < tree_c_len; i++)
    {
        attr_wins[i] = xcb_get_window_attributes_unchecked(globalconf.connection,
                                                           wins[i]);

        state_wins[i] = xwindow_get_state_unchecked(wins[i]);
    }

    xcb_get_geometry_cookie_t *geom_wins[tree_c_len];

    for(i = 0; i < tree_c_len; i++)
    {
        attr_r = xcb_get_window_attributes_reply(globalconf.connection,
                                                 attr_wins[i],
                                                 NULL);

        state = xwindow_get_state_reply(state_wins[i]);

        if(!attr_r || attr_r->override_redirect
           || attr_r->map_state == XCB_MAP_STATE_UNMAPPED
           || state == XCB_ICCCM_WM_STATE_WITHDRAWN)
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

        client_manage(wins[i], geom_r, true);

        p_delete(&geom_r);
    }

    p_delete(&tree_r);

    restore_client_order(prop_cookie);
}

static void
a_xcb_check(void)
{
    xcb_generic_event_t *mouse = NULL, *event;

    while((event = xcb_poll_for_event(globalconf.connection)))
    {
        /* We will treat mouse events later.
         * We cannot afford to treat all mouse motion events,
         * because that would be too much CPU intensive, so we just
         * take the last we get after a bunch of events. */
        if(XCB_EVENT_RESPONSE_TYPE(event) == XCB_MOTION_NOTIFY)
        {
            p_delete(&mouse);
            mouse = event;
        }
        else
        {
            uint8_t type = XCB_EVENT_RESPONSE_TYPE(event);
            if(mouse && (type == XCB_ENTER_NOTIFY || type == XCB_LEAVE_NOTIFY
                        || type == XCB_BUTTON_PRESS || type == XCB_BUTTON_RELEASE))
            {
                /* Make sure enter/motion/leave/press/release events are handled
                 * in the correct order */
                event_handle(mouse);
                p_delete(&mouse);
            }
            event_handle(event);
            p_delete(&event);
        }
    }

    if(mouse)
    {
        event_handle(mouse);
        p_delete(&mouse);
    }
}

static gboolean
a_xcb_io_cb(GIOChannel *source, GIOCondition cond, gpointer data)
{
    /* a_xcb_check() already handled all events */

    if(xcb_connection_has_error(globalconf.connection))
        fatal("X server connection broke (error %d)",
                xcb_connection_has_error(globalconf.connection));

    return TRUE;
}

static gint
a_glib_poll(GPollFD *ufds, guint nfsd, gint timeout)
{
    guint res;
    struct timeval now, length_time;
    float length;

    /* Do all deferred work now */
    awesome_refresh();

    /* Check if the Lua stack is the way it should be */
    if (lua_gettop(globalconf.L) != 0) {
        warn("Something was left on the Lua stack, this is a bug!");
        luaA_dumpstack(globalconf.L);
        lua_settop(globalconf.L, 0);
    }

    /* Check how long this main loop iteration took */
    gettimeofday(&now, NULL);
    timersub(&now, &last_wakeup, &length_time);
    length = length_time.tv_sec + length_time.tv_usec * 1.0f / 1e6;
    if (length > main_loop_iteration_limit) {
        warn("Last main loop iteration took %.6f seconds! Increasing limit for "
                "this warning to that value.", length);
        main_loop_iteration_limit = length;
    }

    /* Actually do the polling, record time of wakeup and check for new xcb events */
    res = g_poll(ufds, nfsd, timeout);
    gettimeofday(&last_wakeup, NULL);
    a_xcb_check();

    return res;
}

static void
signal_fatal(int signum)
{
    buffer_t buf;
    backtrace_get(&buf);
    fatal("signal %d, dumping backtrace\n%s", signum, buf.s);
}

/** Function to exit on some signals.
 * \param data currently unused
 */
static gboolean
exit_on_signal(gpointer data)
{
    g_main_loop_quit(globalconf.loop);
    return TRUE;
}

void
awesome_restart(void)
{
    awesome_atexit(true);
    a_exec(awesome_argv);
}

/** Function to restart awesome on some signals.
 * \param data currently unused
 */
static gboolean
restart_on_signal(gpointer data)
{
    awesome_restart();
    return TRUE;
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
    char *confpath = NULL;
    int xfd, i, opt;
    ssize_t cmdlen = 1;
    xdgHandle xdg;
    bool no_argb = false;
    bool run_test = false;
    xcb_query_tree_cookie_t tree_c;
    static struct option long_options[] =
    {
        { "help",    0, NULL, 'h' },
        { "version", 0, NULL, 'v' },
        { "config",  1, NULL, 'c' },
        { "check",   0, NULL, 'k' },
        { "no-argb", 0, NULL, 'a' },
        { NULL,      0, NULL, 0 }
    };

    /* Make stdout/stderr line buffered. */
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    /* clear the globalconf structure */
    p_clear(&globalconf, 1);
    globalconf.keygrabber = LUA_REFNIL;
    globalconf.mousegrabber = LUA_REFNIL;
    buffer_init(&globalconf.startup_errors);

    /* save argv */
    for(i = 0; i < argc; i++)
        cmdlen += a_strlen(argv[i]) + 1;

    awesome_argv = p_new(char, cmdlen);
    a_strcpy(awesome_argv, cmdlen, argv[0]);

    for(i = 1; i < argc; i++)
    {
        a_strcat(awesome_argv, cmdlen, " ");
        a_strcat(awesome_argv, cmdlen, argv[i]);
    }

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* Get XDG basedir data */
    xdgInitHandle(&xdg);

    /* init lua */
    luaA_init(&xdg);

    /* check args */
    while((opt = getopt_long(argc, argv, "vhkc:a",
                             long_options, NULL)) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version();
            break;
          case 'h':
            exit_help(EXIT_SUCCESS);
            break;
          case 'k':
            run_test = true;
            break;
          case 'c':
            if(a_strlen(optarg))
                confpath = a_strdup(optarg);
            else
                fatal("-c option requires a file name");
            break;
          case 'a':
            no_argb = true;
            break;
        }

    if (run_test)
    {
        if(!luaA_parserc(&xdg, confpath, false))
        {
            fprintf(stderr, "✘ Configuration file syntax error.\n");
            return EXIT_FAILURE;
        }
        else
        {
            fprintf(stderr, "✔ Configuration file syntax OK.\n");
            return EXIT_SUCCESS;
        }
    }

    /* register function for signals */
    g_unix_signal_add(SIGINT, exit_on_signal, NULL);
    g_unix_signal_add(SIGTERM, exit_on_signal, NULL);
    g_unix_signal_add(SIGHUP, restart_on_signal, NULL);

    struct sigaction sa = { .sa_handler = signal_fatal, .sa_flags = SA_RESETHAND };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGABRT, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGFPE, &sa, 0);
    sigaction(SIGILL, &sa, 0);
    sigaction(SIGSEGV, &sa, 0);

    /* We have no clue where the input focus is right now */
    globalconf.focus.need_update = true;

    /* X stuff */
    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        fatal("cannot open display (error %d)", xcb_connection_has_error(globalconf.connection));

    globalconf.screen = xcb_aux_get_screen(globalconf.connection, globalconf.default_screen);
    globalconf.default_visual = draw_default_visual(globalconf.screen);
    if(!no_argb)
        globalconf.visual = draw_argb_visual(globalconf.screen);
    if(!globalconf.visual)
        globalconf.visual = globalconf.default_visual;
    globalconf.default_depth = draw_visual_depth(globalconf.screen, globalconf.visual->visual_id);
    globalconf.default_cmap = globalconf.screen->default_colormap;
    if(globalconf.default_depth != globalconf.screen->root_depth)
    {
        // We need our own color map if we aren't using the default depth
        globalconf.default_cmap = xcb_generate_id(globalconf.connection);
        xcb_create_colormap(globalconf.connection, XCB_COLORMAP_ALLOC_NONE,
                globalconf.default_cmap, globalconf.screen->root,
                globalconf.visual->visual_id);
    }

    /* Prefetch all the extensions we might need */
    xcb_prefetch_extension_data(globalconf.connection, &xcb_big_requests_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_test_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_randr_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_xinerama_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_shape_id);

    if (xcb_cursor_context_new(globalconf.connection, globalconf.screen, &globalconf.cursor_ctx) < 0)
        fatal("Failed to initialize xcb-cursor");

    /* initialize dbus */
    a_dbus_init();

    /* Get the file descriptor corresponding to the X connection */
    xfd = xcb_get_file_descriptor(globalconf.connection);
    GIOChannel *channel = g_io_channel_unix_new(xfd);
    g_io_add_watch(channel, G_IO_IN, a_xcb_io_cb, NULL);
    g_io_channel_unref(channel);

    /* Grab server */
    xcb_grab_server(globalconf.connection);

    {
        const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_void_cookie_t cookie;

        /* This causes an error if some other window manager is running */
        cookie = xcb_change_window_attributes_checked(globalconf.connection,
                                                      globalconf.screen->root,
                                                      XCB_CW_EVENT_MASK, &select_input_val);
        if (xcb_request_check(globalconf.connection, cookie))
            fatal("another window manager is already running");
    }

    /* Prefetch the maximum request length */
    xcb_prefetch_maximum_request_length(globalconf.connection);

    /* check for xtest extension */
    const xcb_query_extension_reply_t *query;
    query = xcb_get_extension_data(globalconf.connection, &xcb_test_id);
    globalconf.have_xtest = query->present;

    /* check for shape extension */
    query = xcb_get_extension_data(globalconf.connection, &xcb_shape_id);
    globalconf.have_shape = query->present;

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);
    xcb_get_modifier_mapping_cookie_t xmapping_cookie =
        xcb_get_modifier_mapping_unchecked(globalconf.connection);

    /* init atom cache */
    atoms_init(globalconf.connection);

    /* init screens information */
    screen_scan();

    xutil_lock_mask_get(globalconf.connection, xmapping_cookie,
                        globalconf.keysyms, &globalconf.numlockmask,
                        &globalconf.shiftlockmask, &globalconf.capslockmask,
                        &globalconf.modeswitchmask);

    /* do this only for real screen */
    ewmh_init();
    systray_init();

    /* init spawn (sn) */
    spawn_init();

    /* The default GC is just a newly created associated with a window with
     * depth globalconf.default_depth.
     * The window_no_focus is used for "nothing has the input focus". */
    globalconf.focus.window_no_focus = xcb_generate_id(globalconf.connection);
    globalconf.gc = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth,
                      globalconf.focus.window_no_focus, globalconf.screen->root,
                      -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
                      XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
                      XCB_CW_OVERRIDE_REDIRECT | XCB_CW_COLORMAP,
                      (const uint32_t [])
                      {
                          globalconf.screen->black_pixel,
                          globalconf.screen->black_pixel,
                          1,
                          globalconf.default_cmap
                      });
    xcb_map_window(globalconf.connection, globalconf.focus.window_no_focus);
    xcb_create_gc(globalconf.connection, globalconf.gc, globalconf.focus.window_no_focus,
                  XCB_GC_FOREGROUND | XCB_GC_BACKGROUND,
                  (const uint32_t[]) { globalconf.screen->black_pixel, globalconf.screen->white_pixel });

    /* Get the window tree associated to this screen */
    tree_c = xcb_query_tree_unchecked(globalconf.connection,
                                      globalconf.screen->root);

    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 ROOT_WINDOW_EVENT_MASK);

    /* we will receive events, stop grabbing server */
    xcb_ungrab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* Parse and run configuration file */
    if (!luaA_parserc(&xdg, confpath, true))
        fatal("couldn't find any rc file");

    p_delete(&confpath);

    xdgWipeHandle(&xdg);

    /* scan existing windows */
    scan(tree_c);

    xcb_flush(globalconf.connection);

    /* Setup the main context */
    g_main_context_set_poll_func(g_main_context_default(), &a_glib_poll);
    gettimeofday(&last_wakeup, NULL);

    /* main event loop (if not NULL, awesome.quit() was already called) */
    if (globalconf.loop == NULL)
    {
        globalconf.loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(globalconf.loop);
    }
    g_main_loop_unref(globalconf.loop);
    globalconf.loop = NULL;

    awesome_atexit(false);

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
