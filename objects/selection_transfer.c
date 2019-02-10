/*
 * selection_transfer.c - objects for selection transfer header
 *
 * Copyright © 2019 Uli Schlachter <psychon@znc.in>
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

#include "objects/selection_transfer.h"
#include "common/luaobject.h"
#include "globalconf.h"

#define REGISTRY_TRANSFER_TABLE_INDEX "awesome_selection_transfers"

typedef struct selection_transfer_t
{
    LUA_OBJECT_HEADER
} selection_transfer_t;

static lua_class_t selection_transfer_class;
LUA_OBJECT_FUNCS(selection_transfer_class, selection_transfer_t, selection_transfer)

static void
selection_transfer_notify(xcb_window_t requestor, xcb_atom_t selection,
        xcb_atom_t target, xcb_atom_t property, xcb_timestamp_t time)
{
    xcb_selection_notify_event_t ev;

    p_clear(&ev, 1);
    ev.response_type = XCB_SELECTION_NOTIFY;
    ev.requestor = requestor;
    ev.selection = selection;
    ev.target = target;
    ev.property = property;
    ev.time = time;

    xcb_send_event(globalconf.connection, false, requestor,
            XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

void
selection_transfer_reject(xcb_window_t requestor, xcb_atom_t selection,
        xcb_atom_t target, xcb_timestamp_t time)
{
    selection_transfer_notify(requestor, selection, target, XCB_NONE, time);
}

#include "common/atoms.h" /* TODO remove */
void
selection_transfer_begin(lua_State *L, int ud, xcb_window_t requestor,
        xcb_atom_t selection, xcb_atom_t target, xcb_atom_t property,
        xcb_timestamp_t time)
{
    /* TODO implement this */
    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
            requestor, property, UTF8_STRING, 8, 5, "Test\n");
    selection_transfer_notify(requestor, selection, target, property, time);
}

static bool
selection_transfer_checker(selection_transfer_t *selection)
{
    return true;
}

void
selection_transfer_class_setup(lua_State *L)
{
    static const struct luaL_Reg selection_transfer_methods[] =
    {
        { NULL, NULL }
    };

    static const struct luaL_Reg selection_transfer_meta[] =
    {
        LUA_OBJECT_META(selection_transfer)
        LUA_CLASS_META
        { NULL, NULL }
    };

    /* Store a table in the registry that tracks active selection_transfer_t. */
    lua_pushliteral(L, REGISTRY_TRANSFER_TABLE_INDEX);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    luaA_class_setup(L, &selection_transfer_class, "selection_transfer", NULL,
            (lua_class_allocator_t) selection_transfer_new, NULL,
            (lua_class_checker_t) selection_transfer_checker,
            luaA_class_index_miss_property, luaA_class_newindex_miss_property,
            selection_transfer_methods, selection_transfer_meta);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
