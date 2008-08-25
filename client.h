/*
 * client.h - client management header
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

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include <xcb/xcb_icccm.h>

#include "structs.h"
#include "stack.h"

#define client_need_arrange(c) \
    do { \
        if(!globalconf.screens[(c)->screen].need_arrange \
           && client_isvisible(c, (c)->screen)) \
            globalconf.screens[(c)->screen].need_arrange = true; \
    } while(0)

bool client_maybevisible(client_t *, int);
bool client_isvisible(client_t *, int);
client_t * client_getbywin(xcb_window_t);
void client_setlayer(client_t *, layer_t);
void client_stack(void);
void client_ban(client_t *);
void client_unban(client_t *);
void client_manage(xcb_window_t, xcb_get_geometry_reply_t *, int);
area_t client_geometry_hints(client_t *, area_t);
bool client_resize(client_t *, area_t, bool);
void client_unmanage(client_t *);
void client_updatewmhints(client_t *);
bool client_updatesizehints(client_t *, xcb_size_hints_t *);
bool client_updatetitle(client_t *);
void client_saveprops_tags(client_t *);
void client_kill(client_t *);
void client_setfloating(client_t *, bool);
void client_setsticky(client_t *, bool);
void client_setabove(client_t *, bool);
void client_setbelow(client_t *, bool);
void client_setmodal(client_t *, bool);
void client_setontop(client_t *, bool);
void client_setfullscreen(client_t *, bool);
void client_setborder(client_t *, int);

int luaA_client_newindex(lua_State *);

int luaA_client_userdata_new(lua_State *, client_t *);

DO_SLIST(client_t, client, client_unref)

/** Put client on top of the stack
 * \param c The client to raise.
 */
static inline void
client_raise(client_t *c)
{
    /* Push c on top of the stack. */
    stack_client_push(c);
    client_stack();
}


#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
