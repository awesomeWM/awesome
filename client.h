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

typedef struct Client Client;
struct Client
{
    /** Client name */
    char name[256];
    /** Window geometry */
    int x, y, w, h;
    /** Real window geometry for floating */
    int rx, ry, rw, rh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    /** True if client is unmapped */
    Bool unmapped;
    long flags;
    int border, oldborder;
    Bool isbanned, isfixed, ismax, isfloating, wasfloating;
    /** Tags for the client */
    Bool *tags;
    /** Next client */
    Client *next;
    /** Previous client */
    Client *prev;
    Client *snext;
    /** Window of the client */
    Window win;
    /** Client display */
    Display *display;
    /** Client screen (Zaphod) */
    int screen;
    /** First time viewed on new layout */
    Bool ftview;
};

void attach(Client *);          /* attaches c to global client list */
void ban(Client *);             /* bans c */
void configure(Client *);       /* send synthetic configure event */
void detach(Client *);          /* detaches c from global client list */
void focus(Display *, DC *, Client *, Bool, awesome_config *);           /* focus c if visible && !NULL, or focus top visible */
void manage(Display *, DC *, Window, XWindowAttributes *, awesome_config *);
void resize(Client *, int, int, int, int, awesome_config *, Bool);        /* resize with given coordinates c */
void unban(Client *);           /* unbans c */
void unmanage(Client *, DC *, long, awesome_config *);  /* unmanage c */
void updatesizehints(Client *); /* update the size hint variables of c */
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
