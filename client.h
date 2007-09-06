/* See LICENSE file for copyright and license details. */

#ifndef JDWM_CLIENT_H
#define JDWM_CLIENT_H

/* mask shorthands, used in event.c and client.c */
#define BUTTONMASK              (ButtonPressMask | ButtonReleaseMask)

#include "draw.h"

typedef struct Client Client;
struct Client
{
    char name[256];
    int x, y, w, h;
    int rx, ry, rw, rh;         /* revert geometry */
    int basew, baseh, incw, inch, maxw, maxh, minw, minh;
    int minax, maxax, minay, maxay;
    int unmapped;
    long flags;
    int border, oldborder;
    Bool isbanned, isfixed, ismax, isfloating, wasfloating;
    Bool *tags;
    Client *next;
    Client *prev;
    Client *snext;
    Window win;
    Display * display;
    Bool ftview; /* first time viewed on new layout */
};

void attach(Client *);          /* attaches c to global client list */
void ban(Client *);             /* bans c */
void configure(Client *);       /* send synthetic configure event */
void detach(Client *);          /* detaches c from global client list */
void focus(Display *, DC *, Client *, jdwm_config *);           /* focus c if visible && !NULL, or focus top visible */
void manage(Display *, DC *, Window, XWindowAttributes *, jdwm_config *);       /* manage new client */
void resize(Client *, int, int, int, int, Bool);        /* resize with given coordinates c */
void unban(Client *);           /* unbans c */
void unmanage(Client *, DC *, long, jdwm_config *);  /* unmanage c */
void updatesizehints(Client *); /* update the size hint variables of c */
void updatetitle(Client *);     /* update the name of c */
void saveprops(Client * c, int);     /* saves client properties */
void uicb_killclient(Display *, jdwm_config *, const char *); /* kill client */
void uicb_moveresize(Display *, jdwm_config *, const char *);  /* move and resize window */
void uicb_settrans(Display *, jdwm_config *, const char *);

#endif
