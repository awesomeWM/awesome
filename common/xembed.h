/*
 * common/xembed.h - XEMBED functions header
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2004 Matthew Reppert
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

#ifndef AWESOME_COMMON_XEMBED_H
#define AWESOME_COMMON_XEMBED_H

#include <xcb/xcb.h>

#include <stdbool.h>

#include "common/array.h"
#include "common/util.h"

/** XEMBED information for a window.
 */
typedef struct
{
    unsigned long version;
    unsigned long flags;
} xembed_info_t;

typedef struct xembed_window xembed_window_t;
struct xembed_window
{
    xcb_window_t win;
    xembed_info_t info;
};

DO_ARRAY(xembed_window_t, xembed_window, DO_NOTHING)

/** The version of the XEMBED protocol that this library supports.  */
#define XEMBED_VERSION  0

/** Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                  (1 << 0)
#define XEMBED_INFO_FLAGS_ALL          1

/** XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

/** Details for XEMBED_FOCUS_IN */
#define XEMBED_FOCUS_CURRENT            0
#define XEMBED_FOCUS_FIRST              1
#define XEMBED_FOCUS_LAST               2


/** Modifiers field for XEMBED_REGISTER_ACCELERATOR */
#define XEMBED_MODIFIER_SHIFT   (1 << 0)
#define XEMBED_MODIFIER_CONTROL (1 << 1)
#define XEMBED_MODIFIER_ALT     (1 << 2)
#define XEMBED_MODIFIER_SUPER   (1 << 3)
#define XEMBED_MODIFIER_HYPER   (1 << 4)


/** Flags for XEMBED_ACTIVATE_ACCELERATOR */
#define XEMBED_ACCELERATOR_OVERLOADED   (1 << 0)


void xembed_message_send(xcb_connection_t *, xcb_window_t, long, long, long, long);
xembed_window_t * xembed_getbywin(xembed_window_array_t *, xcb_window_t);
void xembed_property_update(xcb_connection_t *, xembed_window_t *, xcb_get_property_reply_t *);
xcb_get_property_cookie_t xembed_info_get_unchecked(xcb_connection_t *,
                                                    xcb_window_t);
bool xembed_info_get_reply(xcb_connection_t *connection,
                           xcb_get_property_cookie_t cookie,
                           xembed_info_t *info);


/** Indicate to an embedded window that it has focus.
 * \param c The X connection.
 * \param client The client.
 * \param focus_type The type of focus.
 */
static inline void
xembed_focus_in(xcb_connection_t *c, xcb_window_t client, long focus_type)
{
    xembed_message_send(c, client, XEMBED_FOCUS_IN, focus_type, 0, 0);
}

/** Notify a window that it has become active.
 * \param c The X connection.
 * \param client The window to notify.
 */
static inline void
xembed_window_activate(xcb_connection_t *c, xcb_window_t client)
{
    xembed_message_send(c, client, XEMBED_WINDOW_ACTIVATE, 0, 0, 0);
}

/** Notify a window that it has become inactive.
 * \param c The X connection.
 * \param client The window to notify.
 */
static inline
void xembed_window_deactivate(xcb_connection_t *c, xcb_window_t client)
{
    xembed_message_send(c, client, XEMBED_WINDOW_DEACTIVATE, 0, 0, 0);
}

/** Notify a window that its embed request has been received and accepted.
 * \param c The X connection.
 * \param client The client to send message to.
 * \param embedder The embedder window.
 * \param version The version.
 */
static inline void
xembed_embedded_notify(xcb_connection_t *c,
                       xcb_window_t client, xcb_window_t embedder,
                       long version)
{
    xembed_message_send(c, client, XEMBED_EMBEDDED_NOTIFY, 0, embedder, version);
}

/** Have the embedder end XEMBED protocol communication with a child.
 * \param connection The X connection.
 * \param child The window to unembed.
 * \param root The root window to reparent to.
 */
static inline void
xembed_window_unembed(xcb_connection_t *connection, xcb_window_t child, xcb_window_t root)
{
    xcb_reparent_window(connection, child, root, 0, 0);
}

/** Indicate to an embedded window that it has lost focus.
 * \param c The X connection.
 * \param client The client to send message to.
 */
static inline void
xembed_focus_out(xcb_connection_t *c, xcb_window_t client)
{
    xembed_message_send(c, client, XEMBED_FOCUS_OUT, 0, 0, 0);
}


#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
