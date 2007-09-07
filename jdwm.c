/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include "jdwm.h"
#include "util.h"
#include "event.h"
#include "layout.h"
#include "tag.h"

int screen, sx, sy, sw, sh, wax, way, waw, wah;
int bh;
Atom jdwmprops, wmatom[WMLast], netatom[NetLast];
Client *clients = NULL;
Client *sel = NULL;
Client *stack = NULL;
Cursor cursor[CurLast];
DC dc;
Window barwin;

/* static */

static int (*xerrorxlib) (Display *, XErrorEvent *);
static Bool otherwm = False, readin = True;
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
cleanup(Display *disp, jdwm_config *jdwmconf)
{
    close(STDIN_FILENO);
    while(stack)
    {
        unban(stack);
        unmanage(stack, &dc, NormalState, jdwmconf);
    }
    if(dc.font.set)
        XFreeFontSet(disp, dc.font.set);
    else
        XFreeFont(disp, dc.font.xfont);
    XUngrabKey(disp, AnyKey, AnyModifier, DefaultRootWindow(disp));
    XFreePixmap(disp, dc.drawable);
    XFreeGC(disp, dc.gc);
    XDestroyWindow(disp, barwin);
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
    status = XGetWindowProperty(disp, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
                                &real, &format, &n, &extra, (unsigned char **) &p);
    if(status != Success)
        return -1;
    if(n != 0)
        result = *p;
    XFree(p);
    return result;
}

static void
scan(Display *disp, jdwm_config *jdwmconf)
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
                manage(disp, &dc, wins[i], &wa, jdwmconf);
        }
    /* now the transients */
    for(i = 0; i < num; i++)
    {
        if(!XGetWindowAttributes(disp, wins[i], &wa))
            continue;
        if(XGetTransientForHint(disp, wins[i], &d1)
           && (wa.map_state == IsViewable || getstate(disp, wins[i]) == IconicState))
            manage(disp, &dc, wins[i], &wa, jdwmconf);
    }
    if(wins)
        XFree(wins);
}

/** Setup everything before running
 * \param disp Display ref
 * \param jdwmconf jdwm config ref
 * \todo clean things...
 */
static void
setup(Display *disp, jdwm_config *jdwmconf)
{
    XSetWindowAttributes wa;

    /* init atoms */
    jdwmprops = XInternAtom(disp, "_JDWM_PROPERTIES", False);
    wmatom[WMProtocols] = XInternAtom(disp, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(disp, "WM_DELETE_WINDOW", False);
    wmatom[WMName] = XInternAtom(disp, "WM_NAME", False);
    wmatom[WMState] = XInternAtom(disp, "WM_STATE", False);
    netatom[NetSupported] = XInternAtom(disp, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(disp, "_NET_WM_NAME", False);
    XChangeProperty(disp, DefaultRootWindow(disp), netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) netatom, NetLast);
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
    grabkeys(disp, jdwmconf);
    compileregs(jdwmconf);
    /* geometry */
    sx = sy = 0;
    sw = DisplayWidth(disp, screen);
    sh = DisplayHeight(disp, screen);
    initlayouts(jdwmconf);
    /* bar */
    dc.h = bh = dc.font.height + 2;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    barwin = XCreateWindow(disp, DefaultRootWindow(disp), sx, sy, sw, bh, 0,
                           DefaultDepth(disp, screen), CopyFromParent,
                           DefaultVisual(disp, screen),
                           CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, barwin, cursor[CurNormal]);
    updatebarpos(disp, jdwmconf->current_bpos);
    XMapRaised(disp, barwin);
    /* pixmap for everything */
    dc.drawable = XCreatePixmap(disp, DefaultRootWindow(disp), sw, bh, DefaultDepth(disp, screen));
    dc.gc = XCreateGC(disp, DefaultRootWindow(disp), 0, 0);
    XSetLineAttributes(disp, dc.gc, 1, LineSolid, CapButt, JoinMiter);
    if(!dc.font.set)
        XSetFont(disp, dc.gc, dc.font.xfont->fid);
    loadjdwmprops(disp, jdwmconf);
}

/*
 * Startup Error handler to check if another window manager
 * is already running.
 */
static int
xerrorstart(Display * dsply __attribute__ ((unused)), XErrorEvent * ee __attribute__ ((unused)))
{
    otherwm = True;
    return -1;
}

/* extern */

void
uicb_quit(Display *disp __attribute__ ((unused)),
          jdwm_config *jdwmconf __attribute__((unused)),
          const char *arg __attribute__ ((unused)))
{
    readin = running = False;
}

void
updatebarpos(Display *disp, int bpos)
{
    XEvent ev;

    wax = sx;
    way = sy;
    wah = sh;
    waw = sw;
    switch (bpos)
    {
    default:
        wah -= bh;
        way += bh;
        XMoveWindow(disp, barwin, sx, sy);
        break;
    case BarBot:
        wah -= bh;
        XMoveWindow(disp, barwin, sx, sy + wah);
        break;
    case BarOff:
        XMoveWindow(disp, barwin, sx, sy - bh);
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
    fprintf(stderr, "jdwm: fatal error: request code=%d, error code=%d\n",
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
    jdwm_config jdwmconf;

    if(argc == 2 && !strcmp("-v", argv[1]))
        eprint("jdwm-" VERSION " © 2007 Julien Danjou\n");
    else if(argc != 1)
        eprint("usage: jdwm [-v]\n");
    setlocale(LC_CTYPE, "");


    if(!(dpy = XOpenDisplay(NULL)))
        eprint("jdwm: cannot open display\n");
    xfd = ConnectionNumber(dpy);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    XSetErrorHandler(xerrorstart);

    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, root, SubstructureRedirectMask);
    XSync(dpy, False);

    if(otherwm)
        eprint("jdwm: another window manager is already running\n");

    XSync(dpy, False);
    XSetErrorHandler(NULL);
    xerrorxlib = XSetErrorHandler(xerror);
    XSync(dpy, False);
    parse_config(dpy, screen, &dc, &jdwmconf);
    setup(dpy, &jdwmconf);
    drawstatus(dpy, &jdwmconf);
    scan(dpy, &jdwmconf);
    XSync(dpy, False);

    void (*handler[LASTEvent]) (XEvent *, jdwm_config *) = 
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
            switch (r = read(STDIN_FILENO, jdwmconf.statustext, sizeof(jdwmconf.statustext) - 1))
            {
            case -1:
                strncpy(jdwmconf.statustext, strerror(errno), sizeof(jdwmconf.statustext) - 1);
                jdwmconf.statustext[sizeof(jdwmconf.statustext) - 1] = '\0';
                readin = False;
                break;
            case 0:
                strncpy(jdwmconf.statustext, "EOF", 4);
                readin = False;
                break;
            default:
                for(jdwmconf.statustext[r] = '\0', p = jdwmconf.statustext + strlen(jdwmconf.statustext) - 1;
                    p >= jdwmconf.statustext && *p == '\n'; *p-- = '\0');
                for(; p >= jdwmconf.statustext && *p != '\n'; --p);
                if(p > jdwmconf.statustext)
                    strncpy(jdwmconf.statustext, p + 1, sizeof(jdwmconf.statustext));
            }
            drawstatus(dpy, &jdwmconf);
        }

        while(XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            if(handler[ev.type])
                (handler[ev.type]) (&ev, &jdwmconf);       /* call handler */
        }
    }
    cleanup(dpy, &jdwmconf);
    XCloseDisplay(dpy);

    return 0;
}
