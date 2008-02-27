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
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

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
#include "tag.h"
#include "common/awclient.h"
#include "common/util.h"
#include "common/version.h"
#include "common/configopts.h"

static int (*xerrorxlib) (Display *, XErrorEvent *);
static Bool running = True;

AwesomeConf globalconf;

/** Scan X to find windows to manage
 */
static void
scan()
{
    unsigned int i, num;
    int screen, real_screen;
    Window *wins = NULL, d1, d2;
    XWindowAttributes wa;

    for(screen = 0; screen < ScreenCount(globalconf.display); screen++)
    {
        if(XQueryTree(globalconf.display, RootWindow(globalconf.display, screen), &d1, &d2, &wins, &num))
        {
            for(i = 0; i < num; i++)
            {
                /* XGetWindowAttributes return 1 on success */
                if(!XGetWindowAttributes(globalconf.display, wins[i], &wa)
                   || wa.override_redirect
                   || XGetTransientForHint(globalconf.display, wins[i], &d1))
                    continue;
                if(wa.map_state == IsViewable || window_getstate(wins[i]) == IconicState)
                {
                    real_screen = screen_get_bycoord(screen, wa.x, wa.y);
                    client_manage(wins[i], &wa, real_screen);
                }
            }
            /* now the transients */
            for(i = 0; i < num; i++)
            {
                if(!XGetWindowAttributes(globalconf.display, wins[i], &wa))
                    continue;
                if(XGetTransientForHint(globalconf.display, wins[i], &d1)
                   && (wa.map_state == IsViewable || window_getstate(wins[i]) == IconicState))
                {
                    real_screen = screen_get_bycoord(screen, wa.x, wa.y);
                    client_manage(wins[i], &wa, real_screen);
                }
            }
        }
        if(wins)
            XFree(wins);
    }
}

/** Setup everything before running
 * \param screen Screen number
 * \todo clean things...
 */
static void
setup(int screen)
{
    XSetWindowAttributes wa;
    Statusbar *statusbar;
    int phys_screen = get_phys_screen(screen);

    /* init cursors */
    globalconf.cursor[CurNormal] = XCreateFontCursor(globalconf.display, XC_left_ptr);
    globalconf.cursor[CurResize] = XCreateFontCursor(globalconf.display, XC_sizing);
    globalconf.cursor[CurMove] = XCreateFontCursor(globalconf.display, XC_fleur);

    /* select for events */
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = globalconf.cursor[CurNormal];

    XChangeWindowAttributes(globalconf.display,
                            RootWindow(globalconf.display, phys_screen),
                            CWEventMask | CWCursor, &wa);

    XSelectInput(globalconf.display,
                 RootWindow(globalconf.display, phys_screen),
                 wa.event_mask);

    grabkeys(phys_screen);

    /* view at least one tag */
    tag_view(globalconf.screens[screen].tags, True);

    for(statusbar = globalconf.screens[screen].statusbar; statusbar; statusbar = statusbar->next)
        statusbar_init(statusbar);
}

/** Startup Error handler to check if another window manager
 * is already running.
 * \param disp Display
 * \param ee Error event
 */
static int __attribute__ ((noreturn))
xerrorstart(Display * disp __attribute__ ((unused)),
            XErrorEvent * ee __attribute__ ((unused)))
{
    eprint("another window manager is already running\n");
}

/** Quit awesome
 * \param screen Screen ID
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_quit(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    running = False;
}

static void
exit_on_signal(int sig __attribute__ ((unused)))
{
    running = False;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 */
static int
xerror(Display *edpy, XErrorEvent *ee)
{
    if(ee->error_code == BadWindow
       || (ee->error_code == BadMatch && ee->request_code == X_SetInputFocus)
       || (ee->error_code == BadValue && ee->request_code == X_KillClient))
        return 0;
    warn("fatal error: request code=%d, error code=%d\n", ee->request_code, ee->error_code);
    return xerrorxlib(edpy, ee);        /* may call exit */
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
  -k, --check            check configuration file syntax\n");
    exit(exit_code);
}

/** Hello, this is main
 * \param argc who knows
 * \param argv who knows
 * \return EXIT_SUCCESS I hope
 */
typedef void event_handler (XEvent *);
int
main(int argc, char *argv[])
{
    char buf[1024];
    const char *confpath = NULL;
    int r, xfd, e_dummy, csfd, shape_event, randr_event_base, i, screen, opt;
    ssize_t cmdlen = 1;
    fd_set rd;
    XEvent ev;
    Display * dpy;
    event_handler **handler;
    struct sockaddr_un *addr;
    Client *c;
    static struct option long_options[] =
    {
        {"help",    0, NULL, 'h'},
        {"version", 0, NULL, 'v'},
        {"check",   0, NULL, 'k'},
        {"config",  0, NULL, 'c'},
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
          case 'c':
            if(a_strlen(optarg))
                confpath = optarg;
            else
                eprint("-c option requires a file name\n");
            break;
          case 'k':
            return config_check(confpath);
            break;
        }

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* X stuff */
    if(!(dpy = XOpenDisplay(NULL)))
        eprint("cannot open display\n");

    xfd = ConnectionNumber(dpy);

    XSetErrorHandler(xerrorstart);
    for(screen = 0; screen < ScreenCount(dpy); screen++)
        /* this causes an error if some other window manager is running */
        XSelectInput(dpy, RootWindow(dpy, screen), SubstructureRedirectMask);

    /* need to XSync to validate errorhandler */
    XSync(dpy, False);
    XSetErrorHandler(NULL);
    xerrorxlib = XSetErrorHandler(xerror);
    XSync(dpy, False);

    /* store display */
    globalconf.display = dpy;

    /* init EWMH atoms */
    ewmh_init_atoms();

    /* init screens struct */
    screen_build_screens();
    focus_add_client(NULL);

    /* parse config */
    config_parse(confpath);

    scan();

    /* for each virtual screen */
    for(screen = 0; screen < globalconf.nscreen; screen++)
        setup(screen);

    /* do this only for real screen */
    for(screen = 0; screen < ScreenCount(dpy); screen++)
    {
        ewmh_set_supported_hints(screen);
        /* call this to at least grab root window clicks */
        window_root_grabbuttons(screen);
    }

    handler = p_new(event_handler *, LASTEvent);
    handler[ButtonPress] = handle_event_buttonpress;
    handler[ConfigureRequest] = handle_event_configurerequest;
    handler[ConfigureNotify] = handle_event_configurenotify;
    handler[DestroyNotify] = handle_event_destroynotify;
    handler[EnterNotify] = handle_event_enternotify;
    handler[LeaveNotify] = handle_event_leavenotify;
    handler[Expose] = handle_event_expose;
    handler[KeyPress] = handle_event_keypress;
    handler[MappingNotify] = handle_event_mappingnotify;
    handler[MapRequest] = handle_event_maprequest;
    handler[PropertyNotify] = handle_event_propertynotify;
    handler[UnmapNotify] = handle_event_unmapnotify;
    handler[ClientMessage] = handle_event_clientmessage;

    /* check for shape extension */
    if((globalconf.have_shape = XShapeQueryExtension(dpy, &shape_event, &e_dummy)))
    {
        p_realloc(&handler, shape_event + 1);
        handler[shape_event] = handle_event_shape;
    }

    /* check for randr extension */
    if((globalconf.have_randr = XRRQueryExtension(dpy, &randr_event_base, &e_dummy)))
    {
        p_realloc(&handler, randr_event_base + RRScreenChangeNotify + 1);
        handler[randr_event_base + RRScreenChangeNotify] = handle_event_randr_screen_change_notify;
    }

    XSync(dpy, False);

    /* get socket fd */
    csfd = get_client_socket();
    addr = get_client_addr(getenv("DISPLAY"));

    if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
    {
        if(errno == EADDRINUSE)
        {
            if(unlink(addr->sun_path))
                perror("error unlinking existing file");
            if(bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                perror("error binding UNIX domain socket");
        }
        else
            perror("error binding UNIX domain socket");
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
                perror("awesome: error reading UNIX domain socket");
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
        while(XPending(dpy))
        {
            while(XPending(dpy))
            {
                XNextEvent(dpy, &ev);
                if(handler[ev.type])
                    handler[ev.type](&ev);

                /* drop events requested to */
                if(globalconf.drop_events)
                {
                    /* need to resync */
                    XSync(dpy, False);
                    while(XCheckMaskEvent(dpy, globalconf.drop_events, &ev));
                    globalconf.drop_events = NoEventMask;
                }

                /* need to resync */
                XSync(dpy, False);
            }

            statusbar_refresh();
            layout_refresh();

            /* need to resync */
            XSync(dpy, False);
        }
    }


    if(csfd > 0 && close(csfd))
        perror("error closing UNIX domain socket");
    if(unlink(addr->sun_path))
        perror("error unlinking UNIX domain socket");
    p_delete(&addr);

    /* remap all clients since some WM won't handle them otherwise */
    for(c = globalconf.clients; c; c = c->next)
        client_unban(c);

    XSync(globalconf.display, False);

    XCloseDisplay(dpy);

    return EXIT_SUCCESS;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
