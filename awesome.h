/* See LICENSE file for copyright and license details.
 *
 * Julien's dynamic window manager is designed like any other X client as well.
 * It is driven through handling X events. In contrast to other X clients, a
 * window manager selects for SubstructureRedirectMask on the root window, to
 * receive events about window (dis-)appearance.  Only one X connection at a
 * time is allowed to select for this event mask.
 *
 * Calls to fetch an X event from the event queue are blocking.  Due reading
 * status text from standard input, a select()-driven main loop has been
 * implemented which selects for reads on the X connection and STDIN_FILENO to
 * handle all data smoothly. The event handlers of awesome are organized in an
 * array which is accessed whenever a new event has been fetched. This allows
 * event dispatching in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.  Clients are organized in a global
 * doubly-linked client list, the focus history is remembered through a global
 * stack list. Each client contains an array of Bools of the same size as the
 * global tags array to indicate the tags of a client.  For each client awesome
 * creates a small title window, which is resized whenever the (_NET_)WM_NAME
 * properties are updated or the client is moved/resized.
 *
 * Keys and tagging rules are organized as arrays and defined in the config.h
 * file. These arrays are kept static in event.o and tag.o respectively,
 * because no other part of awesome needs access to them.  The current layout is
 * represented by the lt pointer.
 *
 * To understand everything else, start reading main.c:main().
 */

#ifndef AWESOME_AWESOME_H
#define AWESOME_AWESOME_H

#include "config.h"

enum
{ CurNormal, CurResize, CurMove, CurLast };     /* cursor */
enum
{ NetSupported, NetWMName, NetLast };   /* EWMH atoms */
enum
{ WMName, WMState, WMLast };     /* default atoms */

Bool gettextprop(Display *, Window, Atom, char *, unsigned int);   /* return text property, UTF-8 compliant */
void updatebarpos(Display *, Statusbar);        /* updates the bar position */
void uicb_quit(Display *, awesome_config *, const char *);        /* quit awesome nicely */
int xerror(Display *, XErrorEvent *);   /* awesome's X error handler */

#endif
