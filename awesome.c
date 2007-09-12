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

#include "awesome.h"
#include "event.h"
#include "layout.h"
#include "tag.h"

int wax, way, waw, wah;
Client *clients = NULL;
Client *sel = NULL;
Client *stack = NULL;
Cursor cursor[CurLast];
DC dc;

/* static */

static int (*xerrorxlib) (Display *, XErrorEvent *);
static Bool readin = True;
static Bool running = True;


Bool
gettextprop(Display *disp, Window w, Atom atom, char *text, unsigned int size)
{
    char **list = NULL;
    int n;

    XTextProperty name;

    if(!text || size == 0)
        return False;

    text[0] = '\0';
    XGetTextProperty(disp, w, &name, atom);

    if(!name.nitems)
        return False;

    if(name.encoding == XA_STRING)
        strncpy(text, (char *) name.value, size - 1);
    else if(XmbTextPropertyToTextList(disp, &name, &list, &n) >= Success && n > 0 && *list)
    {
        strncpy(text, *list, size - 1);
        XFreeStringList(list);
    }

    text[size - 1] = '\0';
    XFree(name.value);

    return True;
}

static void
cleanup(Display *disp, awesome_config *awesomeconf)
{
    close(STDIN_FILENO);
    while(stack)
    {
        unban(stack);
        unmanage(stack, &dc, NormalState, awesomeconf);
    }
    if(dc.font.set)
        XFreeFontSet(disp, dc.font.set);
    else
        XFreeFont(disp, dc.font.xfont);
    XUngrabKey(disp, AnyKey, AnyModifier, DefaultRootWindow(disp));
    XFreePixmap(disp, dc.drawable);
    XFreeGC(disp, dc.gc);
    XDestroyWindow(disp, awesomeconf->statusbar.window);
    XFreeCursor(disp, cursor[CurNormal]);
    XFreeCursor(disp, cursor[CurResize]);
    XFreeCursor(disp, cursor[CurMove]);
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
scan(Display *disp, awesome_config *awesomeconf)
{
    unsigned int i, num;
    Window *wins, d1, d2;
    XWindowAttributes wa;

    wins = NULL;
    if(XQueryTree(disp, DefaultRootWindow(disp), &d1, &d2, &wins, &num))
        for(i = 0; i < num; i++)
        {
            if(!XGetWindowAttributes(disp, wins[i], &wa)
               || wa.override_redirect || XGetTransientForHint(disp, wins[i], &d1))
                continue;
            if(wa.map_state == IsViewable || getstate(disp, wins[i]) == IconicState)
                manage(disp, &dc, wins[i], &wa, awesomeconf);
        }
    /* now the transients */
    for(i = 0; i < num; i++)
    {
        if(!XGetWindowAttributes(disp, wins[i], &wa))
            continue;
        if(XGetTransientForHint(disp, wins[i], &d1)
           && (wa.map_state == IsViewable || getstate(disp, wins[i]) == IconicState))
            manage(disp, &dc, wins[i], &wa, awesomeconf);
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
setup(Display *disp, awesome_config *awesomeconf)
{
    XSetWindowAttributes wa;

    netatom[NetSupported] = XInternAtom(disp, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(disp, "_NET_WM_NAME", False);
    XChangeProperty(disp, DefaultRootWindow(disp), netatom[NetSupported],
                    XA_ATOM, 32, PropModeReplace, (unsigned char *) netatom, NetLast);
    /* init cursors */
    cursor[CurNormal] = XCreateFontCursor(disp, XC_left_ptr);
    cursor[CurResize] = XCreateFontCursor(disp, XC_sizing);
    cursor[CurMove] = XCreateFontCursor(disp, XC_fleur);
    /* select for events */
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = cursor[CurNormal];
    XChangeWindowAttributes(disp, DefaultRootWindow(disp), CWEventMask | CWCursor, &wa);
    XSelectInput(disp, DefaultRootWindow(disp), wa.event_mask);
    grabkeys(disp, awesomeconf);
    compileregs(awesomeconf->rules, awesomeconf->nrules);
    /* bar */
    dc.h = awesomeconf->statusbar.height = dc.font.height + 2;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    awesomeconf->statusbar.window = XCreateWindow(disp, DefaultRootWindow(disp), 0, 0, DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, 0,
                                               DefaultDepth(disp, DefaultScreen(disp)), CopyFromParent,
                                               DefaultVisual(disp, DefaultScreen(disp)),
                                               CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, awesomeconf->statusbar.window, cursor[CurNormal]);
    updatebarpos(disp, awesomeconf->statusbar);
    XMapRaised(disp, awesomeconf->statusbar.window);
    /* pixmap for everything */
    dc.drawable = XCreatePixmap(disp, DefaultRootWindow(disp), DisplayWidth(disp, DefaultScreen(disp)), awesomeconf->statusbar.height, DefaultDepth(disp, DefaultScreen(disp)));
    dc.gc = XCreateGC(disp, DefaultRootWindow(disp), 0, 0);
    XSetLineAttributes(disp, dc.gc, 1, LineSolid, CapButt, JoinMiter);
    if(!dc.font.set)
        XSetFont(disp, dc.gc, dc.font.xfont->fid);
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
          awesome_config *awesomeconf __attribute__((unused)),
          const char *arg __attribute__ ((unused)))
{
    readin = running = False;
}

void
updatebarpos(Display *disp, Statusbar statusbar)
{
    XEvent ev;

    wax = 0;
    way = 0;
    wah = DisplayHeight(disp, DefaultScreen(disp));
    waw = DisplayWidth(disp, DefaultScreen(disp));
    switch (statusbar.position)
    {
    default:
        wah -= statusbar.height;
        way += statusbar.height;
        XMoveWindow(disp, statusbar.window, 0, 0);
        break;
    case BarBot:
        wah -= statusbar.height;
        XMoveWindow(disp, statusbar.window, 0, wah);
        break;
    case BarOff:
        XMoveWindow(disp, statusbar.window, 0, 0 - statusbar.height);
        break;
    }
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
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
    int r, xfd;
    fd_set rd;
    XEvent ev;
    Display * dpy;
    Window root;
    awesome_config awesomeconf;

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
    setup(dpy, &awesomeconf);
    drawstatus(dpy, &awesomeconf);
    scan(dpy, &awesomeconf);
    XSync(dpy, False);

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
        [UnmapNotify] = handle_event_unmapnotify
    };


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
            drawstatus(dpy, &awesomeconf);
        }

        while(XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            if(handler[ev.type])
                (handler[ev.type]) (&ev, &awesomeconf);       /* call handler */
        }
    }
    cleanup(dpy, &awesomeconf);
    XCloseDisplay(dpy);

    return 0;
}
