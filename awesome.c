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

#include "awesome.h"

#include "banning.h"
#include "common/atoms.h"
#include "common/backtrace.h"
#include "common/version.h"
#include "common/xutil.h"
#include "xkb.h"
#include "dbus.h"
#include "event.h"
#include "ewmh.h"
#include "globalconf.h"
#include "objects/client.h"
#include "objects/screen.h"
#include "spawn.h"
#include "systray.h"
#include "xwindow.h"

#include <getopt.h>

#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <xcb/bigreq.h>
#include <xcb/randr.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_event.h>
#include <xcb/xinerama.h>
#include <xcb/xtest.h>
#include <xcb/shape.h>
#include <xcb/xfixes.h>

#include <glib-unix.h>

awesome_t globalconf;

/** argv used to run awesome */
static char **awesome_argv;

/** time of last main loop wakeup */
static struct timeval last_wakeup;

/** current limit for the main loop's runtime */
static float main_loop_iteration_limit = 0.1;

/** A pipe that is used to asynchronously handle SIGCHLD */
static int sigchld_pipe[2];

/* Initialise various random number generators */
static void
init_rng(void)
{
    /* LuaJIT uses its own, internal RNG, so initialise that */
    lua_State *L = globalconf_get_lua_State();

    /* Get math.randomseed */
    lua_getglobal(L, "math");
    lua_getfield(L, -1, "randomseed");

    /* Push a seed */
    lua_pushnumber(L, g_random_int() + g_random_double());

    /* Call math.randomseed */
    lua_call(L, 1, 0);

    /* Remove "math" global */
    lua_pop(L, 1);

    /* Lua 5.1, Lua 5.2, and (sometimes) Lua 5.3 use rand()/srand() */
    srand(g_random_int());

    /* When Lua 5.3 is built with LUA_USE_POSIX, it uses random()/srandom() */
    srandom(g_random_int());
}

/** Call before exiting.
 */
void
awesome_atexit(bool restart)
{
    lua_State *L = globalconf_get_lua_State();
    lua_pushboolean(L, restart);
    signal_object_emit(L, &global_signals, "exit", 1);

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
    lua_close(L);

    /* X11 is a great protocol. There is a save-set so that reparenting WMs
     * don't kill clients when they shut down. However, when a focused windows
     * is saved, the focus will move to its parent with revert-to none.
     * Immediately afterwards, this parent is destroyed and the focus is gone.
     * Work around this by placing the focus where we like it to be.
     */
    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_POINTER_ROOT,
            XCB_NONE, globalconf.timestamp);
    xcb_aux_sync(globalconf.connection);

    xkb_free();

    /* Disconnect *after* closing lua */
    xcb_cursor_context_free(globalconf.cursor_ctx);
    xcb_disconnect(globalconf.connection);

    close(sigchld_pipe[0]);
    close(sigchld_pipe[1]);
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

    luaA_class_emit_signal(globalconf_get_lua_State(), &client_class, "list", 0);
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
    xcb_get_geometry_cookie_t geom_wins[tree_c_len];

    for(i = 0; i < tree_c_len; i++)
    {
        attr_wins[i] = xcb_get_window_attributes_unchecked(globalconf.connection,
                                                           wins[i]);

        state_wins[i] = xwindow_get_state_unchecked(wins[i]);
        geom_wins[i] = xcb_get_geometry_unchecked(globalconf.connection, wins[i]);
    }

    for(i = 0; i < tree_c_len; i++)
    {
        attr_r = xcb_get_window_attributes_reply(globalconf.connection,
                                                 attr_wins[i],
                                                 NULL);
        geom_r = xcb_get_geometry_reply(globalconf.connection, geom_wins[i], NULL);

        long state = xwindow_get_state_reply(state_wins[i]);

        if(!geom_r || !attr_r || attr_r->override_redirect
           || attr_r->map_state == XCB_MAP_STATE_UNMAPPED
           || state == XCB_ICCCM_WM_STATE_WITHDRAWN)
        {
            p_delete(&attr_r);
            p_delete(&geom_r);
            continue;
        }

        client_manage(wins[i], geom_r, attr_r);

        p_delete(&attr_r);
        p_delete(&geom_r);
    }

    p_delete(&tree_r);

    restore_client_order(prop_cookie);
}

static void
acquire_WM_Sn(bool replace)
{
    xcb_intern_atom_cookie_t atom_q;
    xcb_intern_atom_reply_t *atom_r;
    char *atom_name;
    xcb_get_selection_owner_reply_t *get_sel_reply;
    xcb_client_message_event_t ev;

    /* Get the WM_Sn atom */
    globalconf.selection_owner_window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.screen->root_depth,
                      globalconf.selection_owner_window, globalconf.screen->root,
                      -1, -1, 1, 1, 0,
                      XCB_COPY_FROM_PARENT, globalconf.screen->root_visual,
                      0, NULL);
    xwindow_set_class_instance(globalconf.selection_owner_window);
    xwindow_set_name_static(globalconf.selection_owner_window,
            "Awesome WM_Sn selection owner window");

    atom_name = xcb_atom_name_by_screen("WM_S", globalconf.default_screen);
    if(!atom_name)
        fatal("error getting WM_Sn atom name");

    atom_q = xcb_intern_atom_unchecked(globalconf.connection, false,
                                               a_strlen(atom_name), atom_name);

    p_delete(&atom_name);

    atom_r = xcb_intern_atom_reply(globalconf.connection, atom_q, NULL);
    if(!atom_r)
        fatal("error getting WM_Sn atom");

    globalconf.selection_atom = atom_r->atom;
    p_delete(&atom_r);

    /* Is the selection already owned? */
    get_sel_reply = xcb_get_selection_owner_reply(globalconf.connection,
            xcb_get_selection_owner(globalconf.connection, globalconf.selection_atom),
            NULL);
    if (!get_sel_reply)
        fatal("GetSelectionOwner for WM_Sn failed");
    if (!replace && get_sel_reply->owner != XCB_NONE)
        fatal("another window manager is already running (selection owned; use --replace)");

    /* Acquire the selection */
    xcb_set_selection_owner(globalconf.connection, globalconf.selection_owner_window,
                            globalconf.selection_atom, globalconf.timestamp);
    if (get_sel_reply->owner != XCB_NONE)
    {
        /* Wait for the old owner to go away */
        xcb_get_geometry_reply_t *geom_reply = NULL;
        do {
            p_delete(&geom_reply);
            geom_reply = xcb_get_geometry_reply(globalconf.connection,
                    xcb_get_geometry(globalconf.connection, get_sel_reply->owner),
                    NULL);
        } while (geom_reply != NULL);
    }
    p_delete(&get_sel_reply);

    /* Announce that we are the new owner */
    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = globalconf.screen->root;
    ev.format = 32;
    ev.type = MANAGER;
    ev.data.data32[0] = globalconf.timestamp;
    ev.data.data32[1] = globalconf.selection_atom;
    ev.data.data32[2] = globalconf.selection_owner_window;
    ev.data.data32[3] = ev.data.data32[4] = 0;

    xcb_send_event(globalconf.connection, false, globalconf.screen->root, 0xFFFFFF, (char *) &ev);
}

static void
acquire_timestamp(void)
{
    /* Getting a current timestamp is hard. ICCCM recommends a zero-length
     * append to a property, so let's do that.
     */
    xcb_generic_event_t *event;
    xcb_window_t win = globalconf.screen->root;
    xcb_atom_t atom = XCB_ATOM_RESOURCE_MANAGER; /* Just something random */
    xcb_atom_t type = XCB_ATOM_STRING; /* Equally random */

    xcb_grab_server(globalconf.connection);
    xcb_change_window_attributes(globalconf.connection, win,
            XCB_CW_EVENT_MASK, (uint32_t[]) { XCB_EVENT_MASK_PROPERTY_CHANGE });
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_APPEND, win,
            atom, type, 8, 0, "");
    xcb_change_window_attributes(globalconf.connection, win,
            XCB_CW_EVENT_MASK, (uint32_t[]) { 0 });
    xcb_ungrab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* Now wait for the event */
    while((event = xcb_wait_for_event(globalconf.connection)))
    {
        /* Is it the event we are waiting for? */
        if(XCB_EVENT_RESPONSE_TYPE(event) == XCB_PROPERTY_NOTIFY)
        {
            xcb_property_notify_event_t *ev = (void *) event;
            globalconf.timestamp = ev->time;
            p_delete(&event);
            break;
        }

        /* Hm, not the right event. */
        if (globalconf.pending_event != NULL)
        {
            event_handle(globalconf.pending_event);
            p_delete(&globalconf.pending_event);
        }
        globalconf.pending_event = event;
    }
}

static xcb_generic_event_t *poll_for_event(void)
{
    if (globalconf.pending_event) {
        xcb_generic_event_t *event = globalconf.pending_event;
        globalconf.pending_event = NULL;
        return event;
    }

    return xcb_poll_for_event(globalconf.connection);
}

static void
a_xcb_check(void)
{
    xcb_generic_event_t *mouse = NULL, *event;

    while((event = poll_for_event()))
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
    int saved_errno;
    lua_State *L = globalconf_get_lua_State();

    /* Do all deferred work now */
    awesome_refresh();

    /* Check if the Lua stack is the way it should be */
    if (lua_gettop(L) != 0) {
        warn("Something was left on the Lua stack, this is a bug!");
        luaA_dumpstack(L);
        lua_settop(L, 0);
    }

    /* Don't sleep if there is a pending event */
    assert(globalconf.pending_event == NULL);
    globalconf.pending_event = xcb_poll_for_event(globalconf.connection);
    if (globalconf.pending_event != NULL)
        timeout = 0;

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
    saved_errno = errno;
    gettimeofday(&last_wakeup, NULL);
    a_xcb_check();
    errno = saved_errno;

    return res;
}

static void
signal_fatal(int signum)
{
    buffer_t buf;
    backtrace_get(&buf);
    fatal("signal %d, dumping backtrace\n%s", signum, buf.s);
}

/* Signal handler for SIGCHLD. Causes reap_children() to be called. */
static void
signal_child(int signum)
{
    assert(signum == SIGCHLD);
    int res = write(sigchld_pipe[1], " ", 1);
    (void) res;
    assert(res == 1);
}

/* There was a SIGCHLD signal. Read from sigchld_pipe and reap children. */
static gboolean
reap_children(GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
    pid_t child;
    int status;
    char buffer[1024];
    ssize_t result = read(sigchld_pipe[0], &buffer[0], sizeof(buffer));
    if (result < 0)
        fatal("Error reading from signal pipe: %s", strerror(errno));

    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        spawn_child_exited(child, status);
    }
    if (child < 0 && errno != ECHILD)
        warn("waitpid(-1) failed: %s", strerror(errno));
    return TRUE;
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
    execvp(awesome_argv[0], awesome_argv);
    fatal("execv() failed: %s", strerror(errno));
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

static bool
true_config_callback(const char *unused)
{
    return true;
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
      --search DIR       add a directory to the library search path\n\
  -k, --check            check configuration file syntax\n\
  -a, --no-argb          disable client transparency support\n\
  -r, --replace          replace an existing window manager\n");
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
    string_array_t searchpath;
    int xfd, opt;
    xdgHandle xdg;
    bool no_argb = false;
    bool run_test = false;
    bool replace_wm = false;
    xcb_query_tree_cookie_t tree_c;
    static struct option long_options[] =
    {
        { "help",    0, NULL, 'h' },
        { "version", 0, NULL, 'v' },
        { "config",  1, NULL, 'c' },
        { "check",   0, NULL, 'k' },
        { "search",  1, NULL, 's' },
        { "no-argb", 0, NULL, 'a' },
        { "replace", 0, NULL, 'r' },
        { "reap",    1, NULL, '\1' },
        { NULL,      0, NULL, 0 }
    };

    /* Make stdout/stderr line buffered. */
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    /* clear the globalconf structure */
    p_clear(&globalconf, 1);
    globalconf.keygrabber = LUA_REFNIL;
    globalconf.mousegrabber = LUA_REFNIL;
    globalconf.exit_code = EXIT_SUCCESS;
    buffer_init(&globalconf.startup_errors);
    string_array_init(&searchpath);

    /* save argv */
    awesome_argv = argv;

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* check args */
    while((opt = getopt_long(argc, argv, "vhkc:ar",
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
            if (confpath != NULL)
                fatal("--config may only be specified once");
            confpath = a_strdup(optarg);
            break;
          case 's':
            string_array_append(&searchpath, a_strdup(optarg));
            break;
          case 'a':
            no_argb = true;
            break;
          case 'r':
            replace_wm = true;
            break;
          case '\1':
            /* Silently ignore --reap and its argument */
            break;
          default:
            exit_help(EXIT_FAILURE);
            break;
        }

    /* Get XDG basedir data */
    if(!xdgInitHandle(&xdg))
        fatal("Function xdgInitHandle() failed, is $HOME unset?");

    /* add XDG_CONFIG_DIR as include path */
    const char * const *xdgconfigdirs = xdgSearchableConfigDirectories(&xdg);
    for(; *xdgconfigdirs; xdgconfigdirs++)
    {
        /* Append /awesome to *xdgconfigdirs */
        const char *suffix = "/awesome";
        size_t len = a_strlen(*xdgconfigdirs) + a_strlen(suffix) + 1;
        char *entry = p_new(char, len);
        a_strcat(entry, len, *xdgconfigdirs);
        a_strcat(entry, len, suffix);
        string_array_append(&searchpath, entry);
    }

    if (run_test)
    {
        bool success = true;
        /* Get the first config that will be tried */
        const char *config = luaA_find_config(&xdg, confpath, true_config_callback);

        /* Try to parse it */
        lua_State *L = luaL_newstate();
        if(luaL_loadfile(L, config))
        {
            const char *err = lua_tostring(L, -1);
            fprintf(stderr, "%s\n", err);
            success = false;
        }
        p_delete(&config);
        lua_close(L);

        if(!success)
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

    /* Setup pipe for SIGCHLD processing */
    {
        if (!g_unix_open_pipe(sigchld_pipe, FD_CLOEXEC, NULL))
            fatal("Failed to create pipe");

        GIOChannel *channel = g_io_channel_unix_new(sigchld_pipe[0]);
        g_io_add_watch(channel, G_IO_IN, reap_children, NULL);
        g_io_channel_unref(channel);
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
    signal(SIGPIPE, SIG_IGN);

    sa.sa_handler = signal_child;
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD, &sa, 0);

    /* We have no clue where the input focus is right now */
    globalconf.focus.need_update = true;

    /* set the default preferred icon size */
    globalconf.preferred_icon_size = 0;

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

    /* Get a recent timestamp */
    acquire_timestamp();

    /* Prefetch all the extensions we might need */
    xcb_prefetch_extension_data(globalconf.connection, &xcb_big_requests_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_test_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_randr_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_xinerama_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_shape_id);
    xcb_prefetch_extension_data(globalconf.connection, &xcb_xfixes_id);

    if (xcb_cursor_context_new(globalconf.connection, globalconf.screen, &globalconf.cursor_ctx) < 0)
        fatal("Failed to initialize xcb-cursor");
    globalconf.xrmdb = xcb_xrm_database_from_default(globalconf.connection);
    if (globalconf.xrmdb == NULL)
        globalconf.xrmdb = xcb_xrm_database_from_string("");
    if (globalconf.xrmdb == NULL)
        fatal("Failed to initialize xcb-xrm");

    /* Did we get some usable data from the above X11 setup? */
    draw_test_cairo_xcb();

    /* Acquire the WM_Sn selection */
    acquire_WM_Sn(replace_wm);

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
            fatal("another window manager is already running (can't select SubstructureRedirect)");
    }

    /* Prefetch the maximum request length */
    xcb_prefetch_maximum_request_length(globalconf.connection);

    /* check for xtest extension */
    const xcb_query_extension_reply_t *query;
    query = xcb_get_extension_data(globalconf.connection, &xcb_test_id);
    globalconf.have_xtest = query && query->present;

    /* check for shape extension */
    query = xcb_get_extension_data(globalconf.connection, &xcb_shape_id);
    globalconf.have_shape = query && query->present;
    if (globalconf.have_shape)
    {
        xcb_shape_query_version_reply_t *reply =
            xcb_shape_query_version_reply(globalconf.connection,
                    xcb_shape_query_version_unchecked(globalconf.connection),
                    NULL);
        globalconf.have_input_shape = reply && (reply->major_version > 1 ||
                (reply->major_version == 1 && reply->minor_version >= 1));
        p_delete(&reply);
    }

    /* check for xfixes extension */
    query = xcb_get_extension_data(globalconf.connection, &xcb_xfixes_id);
    globalconf.have_xfixes = query && query->present;
    if (globalconf.have_xfixes)
        xcb_discard_reply(globalconf.connection,
                xcb_xfixes_query_version(globalconf.connection, 1, 0).sequence);

    event_init();

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);

    /* init atom cache */
    atoms_init(globalconf.connection);

    ewmh_init();
    systray_init();

    /* init spawn (sn) */
    spawn_init();

    /* init xkb */
    xkb_init();

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
    xwindow_set_class_instance(globalconf.focus.window_no_focus);
    xwindow_set_name_static(globalconf.focus.window_no_focus, "Awesome no input window");
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

    /* get the current wallpaper, from now on we are informed when it changes */
    root_update_wallpaper();

    /* init lua */
    luaA_init(&xdg, &searchpath);
    string_array_wipe(&searchpath);
    init_rng();

    ewmh_init_lua();

    /* init screens information */
    screen_scan();

    /* Parse and run configuration file */
    if (!luaA_parserc(&xdg, confpath))
        fatal("couldn't find any rc file");

    p_delete(&confpath);

    xdgWipeHandle(&xdg);

    /* scan existing windows */
    scan(tree_c);

    luaA_emit_startup();

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

    return globalconf.exit_code;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
