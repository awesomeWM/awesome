/*
 * client.h - client management header
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

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include "common.h"

/** Mask shorthands, used in event.c and client.c */
#define BUTTONMASK              (ButtonPressMask | ButtonReleaseMask)


void grabbuttons(Client *, Bool, Bool, KeySym, unsigned int);
inline void attach(Client **, Client *);
inline void detach(Client **, Client *);
void ban(Client *);             /* bans c */
void configure(Client *);       /* send synthetic configure event */
void focus(Display *, Client *, Bool, awesome_config *);           /* focus c if visible && !NULL, or focus top visible */
void manage(Display *, Window, XWindowAttributes *, awesome_config *);
void resize(Client *, int, int, int, int, awesome_config *, Bool);        /* resize with given coordinates c */
void unban(Client *);           /* unbans c */
void unmanage(Client *, long, awesome_config *);  /* unmanage c */
inline void updatesizehints(Client *); /* update the size hint variables of c */
void updatetitle(Client *);     /* update the name of c */
void saveprops(Client * c, int);     /* saves client properties */
void set_shape(Client *);
UICB_PROTO(uicb_killclient);
UICB_PROTO(uicb_moveresize);
UICB_PROTO(uicb_settrans);
UICB_PROTO(uicb_setborder);
UICB_PROTO(uicb_swapnext);
UICB_PROTO(uicb_swapprev);

#endif
// vim: filetype=c:expandtab:shiftwidth=6:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
