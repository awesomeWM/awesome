/*
 * common/xutil.h - X-related useful functions header
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

#ifndef AWESOME_COMMON_XUTIL_H
#define AWESOME_COMMON_XUTIL_H

#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_event.h>

/* XCB doesn't provide keysyms definition */
#include <X11/keysym.h>

bool xgettextprop(xcb_connection_t *, xcb_window_t, xcb_atom_t, char *, ssize_t);
void xutil_get_lock_mask(xcb_connection_t *, xcb_key_symbols_t *,
                         unsigned int *, unsigned int *, unsigned int *);

/* See http://tronche.com/gui/x/xlib/appendix/b/ for values */
#define CURSOR_FLEUR    52
#define CURSOR_LEFT_PTR 68
#define CURSOR_SIZING   120

#define ANY_KEY      0L      /* special Key Code, passed to GrabKey */
#define ANY_MODIFIER (1<<15) /* used in GrabButton, GrabKey */

/*****************************************************************
 * ERROR CODES
 *****************************************************************/

#define Success            0    /* everything's okay */
#define BadRequest         1    /* bad request code */
#define BadValue           2    /* int parameter out of range */
#define BadWindow          3    /* parameter not a Window */
#define BadPixmap          4    /* parameter not a Pixmap */
#define BadAtom            5    /* parameter not an Atom */
#define BadCursor          6    /* parameter not a Cursor */
#define BadFont            7    /* parameter not a Font */
#define BadMatch           8    /* parameter mismatch */
#define BadDrawable        9    /* parameter not a Pixmap or Window */
#define BadAccess         10    /* depending on context:
                                 - key/button already grabbed
                                 - attempt to free an illegal
                                   cmap entry
                                - attempt to store into a read-only
                                   color map entry.
                                - attempt to modify the access control
                                   list from other than the local host.
                                */
#define BadAlloc          11    /* insufficient resources */
#define BadColor          12    /* no such colormap */
#define BadGC             13    /* parameter not a GC */
#define BadIDChoice       14    /* choice not in range or already used */
#define BadName           15    /* font or color name doesn't exist */
#define BadLength         16    /* Request length incorrect */
#define BadImplementation 17    /* server is defective */

#define FirstExtensionError     128
#define LastExtensionError      255
/* End of macros not defined in XCB */

/* Common function defined in Xlib but not in XCB */
bool xutil_get_transient_for_hint(xcb_connection_t *, xcb_window_t, xcb_window_t *);

typedef struct _class_hint_t
{
    char *res_name;
    char *res_class;
} class_hint_t;

typedef struct
{
    uint32_t pixel;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} xcolor_t;

class_hint_t *xutil_get_class_hint(xcb_connection_t *, xcb_window_t);

/* Equivalent call to XInternAtom
 *
 * WARNING: should not be used in loop, in this case, it should send
 * the queries first and then treat the answer as late as possible)
 */
xcb_atom_t xutil_intern_atom(xcb_connection_t *, const char *);

/* Equivalent XCB call to XMapRaised, which actually raises the
   specified window to the top of the stack and maps it */
void xutil_map_raised(xcb_connection_t *, xcb_window_t);

/** Set the same handler for all errors */
void xutil_set_error_handler_catch_all(xcb_event_handlers_t *,
                                       xcb_generic_error_handler_t, void *);

typedef struct xutil_error_t
{
    uint8_t request_code;
    char *request_label;
    char *error_label;
} xutil_error_t;

xutil_error_t *xutil_get_error(const xcb_generic_error_t *);
void xutil_delete_error(xutil_error_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
