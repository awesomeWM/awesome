/*
 * selection_acquire.c - objects for selection ownership
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

#include "objects/selection_acquire.h"
#include "common/luaobject.h"
#include "globalconf.h"

#define REGISTRY_ACQUIRE_TABLE_INDEX "awesome_selection_acquires"

typedef struct selection_acquire_t
{
    LUA_OBJECT_HEADER
    /** Window used for owning the selection. */
    xcb_window_t window;
    /** Timestamp used for acquiring the selection. */
    xcb_timestamp_t timestamp;
    /** Reference in the special table to this object. */
    int ref;
} selection_acquire_t;

static lua_class_t selection_acquire_class;
LUA_OBJECT_FUNCS(selection_acquire_class, selection_acquire_t, selection_acquire)

static void
selection_acquire_wipe(selection_acquire_t *selection)
{
    if (selection->window != XCB_NONE)
        xcb_destroy_window(globalconf.connection, selection->window);
}

static int
luaA_selection_acquire_new(lua_State *L)
{
    size_t name_length;
    const char *name;
    xcb_intern_atom_reply_t *reply;
    xcb_get_selection_owner_reply_t *selection_reply;
    xcb_atom_t name_atom;
    selection_acquire_t *selection;

    name = luaL_checklstring(L, 2, &name_length);

    /* Create a selection object */
    selection = (void *) selection_acquire_class.allocator(L);
    selection->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.screen->root_depth,
            selection->window, globalconf.screen->root, -1, -1, 1, 1, 0,
            XCB_COPY_FROM_PARENT, globalconf.screen->root_visual, 0, NULL);

    /* Get the atom identifying the selection */
    reply = xcb_intern_atom_reply(globalconf.connection,
            xcb_intern_atom_unchecked(globalconf.connection, false, name_length, name),
            NULL);
    name_atom = reply ? reply->atom : XCB_NONE;
    p_delete(&reply);

    /* Try to acquire the selection */
    selection->timestamp = globalconf.timestamp;
    xcb_set_selection_owner(globalconf.connection, selection->window, name_atom, selection->timestamp);
    selection_reply = xcb_get_selection_owner_reply(globalconf.connection,
            xcb_get_selection_owner(globalconf.connection, name_atom),
            NULL);
    if (selection_reply == NULL || selection_reply->owner != selection->window) {
        /* Acquiring the selection failed, return nothing */
        p_delete(&selection_reply);

        xcb_destroy_window(globalconf.connection, selection->window);
        selection->window = XCB_NONE;
        return 0;
    }

    /* Everything worked, register the object in table */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, -2);
    selection->ref = luaL_ref(L, -2);
    lua_pop(L, 1);

    return 1;
}

void
selection_acquire_class_setup(lua_State *L)
{
    static const struct luaL_Reg selection_acquire_methods[] =
    {
        { "__call", luaA_selection_acquire_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg selection_acquire_meta[] =
    {
        LUA_OBJECT_META(selection_acquire)
        LUA_CLASS_META
        { NULL, NULL }
    };

    /* Store a table in the registry that tracks active selection_acquire_t. */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    luaA_class_setup(L, &selection_acquire_class, "selection_acquire", NULL,
            (lua_class_allocator_t) selection_acquire_new,
            (lua_class_collector_t) selection_acquire_wipe, NULL,
            luaA_class_index_miss_property, luaA_class_newindex_miss_property,
            selection_acquire_methods, selection_acquire_meta);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
