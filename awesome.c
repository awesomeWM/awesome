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
#include "tag.h"
#include "screen.h"
#include "util.h"
#include "statusbar.h"

Client *clients = NULL;
Client *sel = NULL;
Client *stack = NULL;
DC dc;

/* static */

static int (*xerrorxlib) (Display *, XErrorEvent *);
static Bool readin = True, running = True;

static void
cleanup(Display *disp, DC *drawcontext, awesome_config *awesomeconf)
{
    close(STDIN_FILENO);
    while(stack)
    {
        unban(stack);
        unmanage(stack, drawcontext, NormalState, awesomeconf);
    }
    if(drawcontext->font.set)
        XFreeFontSet(disp, drawcontext->font.set);
    else
        XFreeFont(disp, drawcontext->font.xfont);
    XUngrabKey(disp, AnyKey, AnyModifier, DefaultRootWindow(disp));
    XFreePixmap(disp, awesomeconf->statusbar.drawable);
    XFreeGC(disp, drawcontext->gc);
    XDestroyWindow(disp, awesomeconf->statusbar.window);
    XFreeCursor(disp, drawcontext->cursor[CurNormal]);
    XFreeCursor(disp, drawcontext->cursor[CurResize]);
    XFreeCursor(disp, drawcontext->cursor[CurMove]);
    XSetInputFocus(disp, PointerRoot, RevertToPointerRoot, CurrentTime);
    XSync(disp, False);
}

static long
getstate(Display *disp, Window w)
{
    int format, status;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;
    status = XGetWindowProperty(disp, w, XInternAtom(disp, "WM_STATE", False),
                                0L, 2L, False, XInternAtom(disp, "WM_STATE", False),
                                &real, &format, &n, &extra, (unsigned char **) &p);
    if(status != Success)
        return -1;
    if(n != 0)
        result = *p;
    XFree(p);
    return result;
}

static void
scan(Display *disp, DC *drawcontext, awesome_config *awesomeconf)
{
    unsigned int i, num;
    Window *wins, d1, d2;
    XWindowAttributes wa;

    wins = NULL;
    if(XQueryTree(disp, DefaultRootWindow(disp), &d1, &d2, &wins, &num))
        for(i = 0; i < num; i++)
        {
            if(!XGetWindowAttributes(disp, wins[i], &wa)
               || wa.override_redirect
               || XGetTransientForHint(disp, wins[i], &d1))
                continue;
            if(wa.map_state == IsViewable || getstate(disp, wins[i]) == IconicState)
                manage(disp, drawcontext, wins[i], &wa, awesomeconf);
        }
    /* now the transients */
    for(i = 0; i < num; i++)
    {
        if(!XGetWindowAttributes(disp, wins[i], &wa))
            continue;
        if(XGetTransientForHint(disp, wins[i], &d1)
           && (wa.map_state == IsViewable || getstate(disp, wins[i]) == IconicState))
            manage(disp, drawcontext, wins[i], &wa, awesomeconf);
    }
    if(wins)
        XFree(wins);
}



enum { NetSupported, NetWMName, NetLast };   /* EWMH atoms */ 
Atom netatom[NetWMName];
/** Setup everything before running
 * \param disp Display ref
 * \param awesomeconf awesome config ref
 * \todo clean things...
 */
static void
setup(Display *disp, DC *drawcontext, awesome_config *awesomeconf)
{
    XSetWindowAttributes wa;

    netatom[NetSupported] = XInternAtom(disp, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(disp, "_NET_WM_NAME", False);
    XChangeProperty(disp, DefaultRootWindow(disp), netatom[NetSupported],
                    XA_ATOM, 32, PropModeReplace, (unsigned char *) netatom, NetLast);
    /* init cursors */
    drawcontext->cursor[CurNormal] = XCreateFontCursor(disp, XC_left_ptr);
    drawcontext->cursor[CurResize] = XCreateFontCursor(disp, XC_sizing);
    drawcontext->cursor[CurMove] = XCreateFontCursor(disp, XC_fleur);
    /* select for events */
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = drawcontext->cursor[CurNormal];
    XChangeWindowAttributes(disp, DefaultRootWindow(disp), CWEventMask | CWCursor, &wa);
    XSelectInput(disp, DefaultRootWindow(disp), wa.event_mask);
    grabkeys(disp, awesomeconf);
    compileregs(awesomeconf->rules, awesomeconf->nrules);
    /* bar */
    drawcontext->h = awesomeconf->statusbar.height = drawcontext->font.height + 2;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    awesomeconf->statusbar.window = XCreateWindow(disp, DefaultRootWindow(disp), 0, 0, DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, 0,
                                               DefaultDepth(disp, DefaultScreen(disp)), CopyFromParent,
                                               DefaultVisual(disp, DefaultScreen(disp)),
                                               CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, awesomeconf->statusbar.window, drawcontext->cursor[CurNormal]);
    updatebarpos(disp, awesomeconf->statusbar);
    XMapRaised(disp, awesomeconf->statusbar.window);
    /* pixmap for everything */
    awesomeconf->statusbar.drawable = XCreatePixmap(disp, DefaultRootWindow(disp), DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, DefaultDepth(disp, DefaultScreen(disp)));
    drawcontext->gc = XCreateGC(disp, DefaultRootWindow(disp), 0, 0);
    XSetLineAttributes(disp, drawcontext->gc, 1, LineSolid, CapButt, JoinMiter);
    if(!drawcontext->font.set)
        XSetFont(disp, drawcontext->gc, drawcontext->font.xfont->fid);
    loadawesomeprops(disp, awesomeconf);
}

/*
 * Startup Error handler to check if another window manager
 * is already running.
 */
static int __attribute__ ((noreturn))
xerrorstart(Display * dsply __attribute__ ((unused)), XErrorEvent * ee __attribute__ ((unused)))
{
    eprint("awesome: another window manager is already running\n");
}

/* extern */

void
uicb_quit(Display *disp __attribute__ ((unused)),
          DC *drawcontext __attribute__ ((unused)),
          awesome_config *awesomeconf __attribute__((unused)),
          const char *arg __attribute__ ((unused)))
{
    readin = running = False;
}

int
get_windows_area_x(Statusbar statusbar __attribute__ ((unused)))
{
    return 0;
}

int
get_windows_area_y(Statusbar statusbar)
{
    if(statusbar.position == BarTop)
        return statusbar.height;

    return 0;
}

int
get_windows_area_height(Display *disp, Statusbar statusbar)
{
    if(statusbar.position == BarTop || statusbar.position == BarBot)
        return DisplayHeight(disp, DefaultScreen(disp)) - statusbar.height;

    return DisplayHeight(disp, DefaultScreen(disp));
}

int
get_windows_area_width(Display *disp,
                       Statusbar statusbar __attribute__ ((unused)))
{
    return DisplayWidth(disp, DefaultScreen(disp));
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
    fprintf(stderr, "awesome: fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);

    return xerrorxlib(edpy, ee);        /* may call exit */
}

int
main(int argc, char *argv[])
{
    char *p;
    int r, xfd, e_dummy;
    fd_set rd;
    XEvent ev;
    Display * dpy;
    Window root;
    awesome_config awesomeconf;
    int shape_event, randr_event_base;

    if(argc == 2 && !strcmp("-v", argv[1]))
        eprint("awesome-" VERSION " © 2007 Julien Danjou\n");
    else if(argc != 1)
        eprint("usage: awesome [-v]\n");
    setlocale(LC_CTYPE, "");


    if(!(dpy = XOpenDisplay(NULL)))
        eprint("awesome: cannot open display\n");
    xfd = ConnectionNumber(dpy);
    root = RootWindow(dpy, DefaultScreen(dpy));
    XSetErrorHandler(xerrorstart);

    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, root, SubstructureRedirectMask);
    XSync(dpy, False);

    XSync(dpy, False);
    XSetErrorHandler(NULL);
    xerrorxlib = XSetErrorHandler(xerror);
    XSync(dpy, False);
    parse_config(dpy, DefaultScreen(dpy), &dc, &awesomeconf);
    setup(dpy, &dc, &awesomeconf);
    drawstatusbar(dpy, &dc, &awesomeconf);

    void (*handler[LASTEvent]) (XEvent *, awesome_config *) = 
    {
        [ButtonPress] = handle_event_buttonpress,
        [ConfigureRequest] = handle_event_configurerequest,
        [ConfigureNotify] = handle_event_configurenotify,
        [DestroyNotify] = handle_event_destroynotify,
        [EnterNotify] = handle_event_enternotify,
        [LeaveNotify] = handle_event_leavenotify,
        [Expose] = handle_event_expose,
        [KeyPress] = handle_event_keypress,
        [MappingNotify] = handle_event_mappingnotify,
        [MapRequest] = handle_event_maprequest,
        [PropertyNotify] = handle_event_propertynotify,
        [UnmapNotify] = handle_event_unmapnotify,
    };

    /* check for shape extension */
    if((awesomeconf.have_shape = XShapeQueryExtension(dpy, &shape_event, &e_dummy)))
        handler[shape_event] = handle_event_shape;

    /* check for randr extension */
    if((awesomeconf.have_randr = XRRQueryExtension(dpy, &randr_event_base, &e_dummy)))
       handler[randr_event_base + RRScreenChangeNotify] = handle_event_randr_screen_change_notify;

    scan(dpy, &dc, &awesomeconf);
    XSync(dpy, False);

    /* main event loop, also reads status text from stdin */
    while(running)
    {
        FD_ZERO(&rd);
        if(readin)
            FD_SET(STDIN_FILENO, &rd);
        FD_SET(xfd, &rd);
        if(select(xfd + 1, &rd, NULL, NULL, NULL) == -1)
        {
            if(errno == EINTR)
                continue;
            eprint("select failed\n");
        }
        if(FD_ISSET(STDIN_FILENO, &rd))
        {
            switch (r = read(STDIN_FILENO, awesomeconf.statustext, sizeof(awesomeconf.statustext) - 1))
            {
            case -1:
                strncpy(awesomeconf.statustext, strerror(errno), sizeof(awesomeconf.statustext) - 1);
                awesomeconf.statustext[sizeof(awesomeconf.statustext) - 1] = '\0';
                readin = False;
                break;
            case 0:
                strncpy(awesomeconf.statustext, "EOF", 4);
                readin = False;
                break;
            default:
                for(awesomeconf.statustext[r] = '\0', p = awesomeconf.statustext + a_strlen(awesomeconf.statustext) - 1;
                    p >= awesomeconf.statustext && *p == '\n'; *p-- = '\0');
                for(; p >= awesomeconf.statustext && *p != '\n'; --p);
                if(p > awesomeconf.statustext)
                    strncpy(awesomeconf.statustext, p + 1, sizeof(awesomeconf.statustext));
            }
            drawstatusbar(dpy, &dc, &awesomeconf);
        }

        while(XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            if(handler[ev.type])
                (handler[ev.type]) (&ev, &awesomeconf);       /* call handler */
        }
    }
    cleanup(dpy, &dc, &awesomeconf);
    XCloseDisplay(dpy);

    return 0;
}
