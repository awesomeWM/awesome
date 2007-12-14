/*
 * awesome.c - awesome main functions
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include "awesome.h"
#include "event.h"
#include "layout.h"
#include "screen.h"
#include "util.h"
#include "statusbar.h"
#include "uicb.h"
#include "window.h"
#include "client.h"
#include "awesome-client.h"

static int (*xerrorxlib) (Display *, XErrorEvent *);
static Bool running = True;

static inline void
cleanup_buttons(Button *buttons)
{
    Button *b, *bn;

    for(b = buttons; b; b = bn)
    {
        bn = b->next;
        p_delete(&b->arg);
        p_delete(&b);
    }
}

void
cleanup_screen(awesome_config *awesomeconf, int screen)
{
    int i;

    XftFontClose(awesomeconf->display, awesomeconf->screens[screen].font);
    XUngrabKey(awesomeconf->display, AnyKey, AnyModifier, RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)));
    XDestroyWindow(awesomeconf->display, awesomeconf->screens[screen].statusbar.window);

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
        p_delete(&awesomeconf->screens[screen].tags[i].name);

    for(i = 0; i < awesomeconf->screens[screen].nlayouts; i++)
        p_delete(&awesomeconf->screens[screen].layouts[i].symbol);

    p_delete(&awesomeconf->screens[screen].tags);
    p_delete(&awesomeconf->screens[screen].layouts);
}

/** Cleanup everything on quit
 * \param awesomeconf awesome config
 */
static void
cleanup(awesome_config *awesomeconf)
{
    int screen;
    Rule *r, *rn;
    Key *k, *kn;

    while(awesomeconf->clients)
    {
        client_unban(awesomeconf->clients);
        client_unmanage(awesomeconf->clients, NormalState, awesomeconf);
    }

    XFreeCursor(awesomeconf->display, awesomeconf->cursor[CurNormal]);
    XFreeCursor(awesomeconf->display, awesomeconf->cursor[CurResize]);
    XFreeCursor(awesomeconf->display, awesomeconf->cursor[CurMove]);

    for(r = awesomeconf->rules; r; r = rn)
    {
        rn = r->next;
        p_delete(&r->prop);
        p_delete(&r->tags);
        p_delete(&r);
    }

    for(k = awesomeconf->keys; k; k = kn)
    {
        kn = k->next;
        p_delete(&k->arg);
        p_delete(&k);
    }

    cleanup_buttons(awesomeconf->buttons.tag);
    cleanup_buttons(awesomeconf->buttons.title);
    cleanup_buttons(awesomeconf->buttons.layout);
    cleanup_buttons(awesomeconf->buttons.root);
    cleanup_buttons(awesomeconf->buttons.client);

    p_delete(&awesomeconf->configpath);

    for(screen = 0; screen < get_screen_count(awesomeconf->display); screen++)
        cleanup_screen(awesomeconf, screen);

    XSetInputFocus(awesomeconf->display, PointerRoot, RevertToPointerRoot, CurrentTime);
    XSync(awesomeconf->display, False);

    p_delete(&awesomeconf->clients);
    p_delete(&awesomeconf);
}

/** Scan X to find windows to manage
 * \param screen Screen number
 * \param awesomeconf awesome config
 */
static void
scan(awesome_config *awesomeconf)
{
    unsigned int i, num;
    int screen, real_screen;
    Window *wins = NULL, d1, d2;
    XWindowAttributes wa;

    for(screen = 0; screen < ScreenCount(awesomeconf->display); screen++)
    {
        if(XQueryTree(awesomeconf->display, RootWindow(awesomeconf->display, screen), &d1, &d2, &wins, &num))
        {
            real_screen = screen;
            for(i = 0; i < num; i++)
            {
                if(!XGetWindowAttributes(awesomeconf->display, wins[i], &wa)
                   || wa.override_redirect
                   || XGetTransientForHint(awesomeconf->display, wins[i], &d1))
                    continue;
                if(wa.map_state == IsViewable || window_getstate(awesomeconf->display, wins[i]) == IconicState)
                {
                    if(screen == 0)
                        real_screen = get_screen_bycoord(awesomeconf->display, wa.x, wa.y);
                    client_manage(wins[i], &wa, awesomeconf, real_screen);
                }
            }
            /* now the transients */
            for(i = 0; i < num; i++)
            {
                if(!XGetWindowAttributes(awesomeconf->display, wins[i], &wa))
                    continue;
                if(XGetTransientForHint(awesomeconf->display, wins[i], &d1)
                   && (wa.map_state == IsViewable || window_getstate(awesomeconf->display, wins[i]) == IconicState))
                {
                    if(screen == 0)
                        real_screen = get_screen_bycoord(awesomeconf->display, wa.x, wa.y);
                    client_manage(wins[i], &wa, awesomeconf, real_screen);
                }
            }
        }
        if(wins)
            XFree(wins);
    }
}

/** Setup everything before running
 * \param awesomeconf awesome config ref
 * \todo clean things...
 */
static void
setup(awesome_config *awesomeconf, int screen)
{
    XSetWindowAttributes wa;

    /* init cursors */
    awesomeconf->cursor[CurNormal] = XCreateFontCursor(awesomeconf->display, XC_left_ptr);
    awesomeconf->cursor[CurResize] = XCreateFontCursor(awesomeconf->display, XC_sizing);
    awesomeconf->cursor[CurMove] = XCreateFontCursor(awesomeconf->display, XC_fleur);

    /* select for events */
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = awesomeconf->cursor[CurNormal];

    XChangeWindowAttributes(awesomeconf->display,
                            RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)),
                            CWEventMask | CWCursor, &wa);

    XSelectInput(awesomeconf->display, RootWindow(awesomeconf->display, get_phys_screen(awesomeconf->display, screen)), wa.event_mask);

    grabkeys(awesomeconf, screen);
}

void
setup_screen(awesome_config *awesomeconf, int screen)
{
    setup(awesomeconf, screen);
    initstatusbar(awesomeconf->display, screen, &awesomeconf->screens[screen].statusbar,
                  awesomeconf->cursor[CurNormal], awesomeconf->screens[screen].font,
                  awesomeconf->screens[screen].layouts, awesomeconf->screens[screen].nlayouts, &awesomeconf->screens[screen].padding);
}

/** Startup Error handler to check if another window manager
 * is already running.
 * \param disp Display ref
 * \param ee Error event
 */
static int __attribute__ ((noreturn))
xerrorstart(Display * disp __attribute__ ((unused)), XErrorEvent * ee __attribute__ ((unused)))
{
    eprint("another window manager is already running\n");
}

/** Quit awesome
 * \param awesomeconf awesome config
 * \param arg nothing
 * \ingroup ui_callback
 */
void
uicb_quit(awesome_config *awesomeconf __attribute__((unused)),
          int screen __attribute__ ((unused)),
          const char *arg __attribute__ ((unused)))
{
    running = False;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 */
int
xerror(Display * edpy, XErrorEvent * ee)
{
    if(ee->error_code == BadWindow
       || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
       || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
       || (ee->request_code == X_PolyFillRectangle
           && ee->error_code == BadDrawable)
       || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
       || (ee->request_code == X_ConfigureWindow
           && ee->error_code == BadMatch) || (ee->request_code == X_GrabKey
                                              && ee->error_code == BadAccess)
       || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    warn("fatal error: request code=%d, error code=%d\n", ee->request_code, ee->error_code);
    return xerrorxlib(edpy, ee);        /* may call exit */
}

/** Hello, this is main
 * \param argc who knows
 * \param argv who knows
 * \return EXIT_SUCCESS I hope
 */
typedef void event_handler (XEvent *, awesome_config *);
int
main(int argc, char *argv[])
{
    char buf[1024];
    const char *confpath = NULL;
    int r, xfd, e_dummy, csfd;
    fd_set rd;
    XEvent ev;
    Display * dpy;
    awesome_config *awesomeconf;
    int shape_event, randr_event_base;
    int screen;
    enum { NetSupported, NetWMName, NetLast };   /* EWMH atoms */
    Atom netatom[NetLast];
    event_handler **handler;
    struct sockaddr_un *addr;

    if(argc >= 2)
    {
        if(!a_strcmp("-v", argv[1]) || !a_strcmp("--version", argv[1]))
        {
            printf("awesome-" VERSION " (" RELEASE ")\n");
            return EXIT_SUCCESS;
        }
        else if(!a_strcmp("-c", argv[1]))
        {
            if(a_strlen(argv[2]))
                confpath = argv[2];
            else
                eprint("-c require a file\n");
        }
        else
            eprint("options: [-v | -c configfile]\n");
    }

    /* Tag won't be printed otherwised */
    setlocale(LC_CTYPE, "");

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

    /* allocate stuff */
    awesomeconf = p_new(awesome_config, 1);
    awesomeconf->screens = p_new(VirtScreen, get_screen_count(dpy));
    /* store display */
    awesomeconf->display = dpy;
    parse_config(confpath, awesomeconf);

    for(screen = 0; screen < get_screen_count(dpy); screen++)
    {
        /* set screen */
        setup_screen(awesomeconf, screen);
        drawstatusbar(awesomeconf, screen);
    }

    netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);

    /* do this only for real screen */
    for(screen = 0; screen < ScreenCount(dpy); screen++)
    {
        loadawesomeprops(awesomeconf, screen);
        XChangeProperty(dpy, RootWindow(dpy, screen), netatom[NetSupported],
                        XA_ATOM, 32, PropModeReplace, (unsigned char *) netatom, NetLast);
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

    /* check for shape extension */
    if((awesomeconf->have_shape = XShapeQueryExtension(dpy, &shape_event, &e_dummy)))
    {
        p_realloc(&handler, shape_event + 1);
        handler[shape_event] = handle_event_shape;
    }

    /* check for randr extension */
    if((awesomeconf->have_randr = XRRQueryExtension(dpy, &randr_event_base, &e_dummy)))
    {
        p_realloc(&handler, randr_event_base + RRScreenChangeNotify + 1);
        handler[randr_event_base + RRScreenChangeNotify] = handle_event_randr_screen_change_notify;
    }

    scan(awesomeconf);

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
                parse_control(buf, awesomeconf);
            }

        while(XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            if(handler[ev.type])
                handler[ev.type](&ev, awesomeconf);       /* call handler */
        }
    }

    if(csfd > 0 && close(csfd))
        perror("error closing UNIX domain socket");
    if(unlink(addr->sun_path))
        perror("error unlinking UNIX domain socket");
    p_delete(&addr);

    cleanup(awesomeconf);
    XCloseDisplay(dpy);

    return EXIT_SUCCESS;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
