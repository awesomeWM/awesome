/*
 * selection_getter.c - selection content getter
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

#include "objects/selection_getter.h"
#include "common/luaobject.h"
#include "common/atoms.h"
#include "globalconf.h"

#define REGISTRY_GETTER_TABLE_INDEX "awesome_selection_getters"

typedef struct selection_getter_t
{
    LUA_OBJECT_HEADER
    /** Reference in the special table to this object */
    int ref;
    /** Window used for the transfer */
    xcb_window_t window;
} selection_getter_t;

static lua_class_t selection_getter_class;
LUA_OBJECT_FUNCS(selection_getter_class, selection_getter_t, selection_getter)

static void
selection_getter_wipe(selection_getter_t *selection)
{
    xcb_destroy_window(globalconf.connection, selection->window);
}

static int
luaA_selection_getter_new(lua_State *L)
{
    size_t name_length, target_length;
    const char *name, *target;
    xcb_intern_atom_cookie_t cookies[2];
    xcb_intern_atom_reply_t *reply;
    selection_getter_t *selection;
    xcb_atom_t name_atom, target_atom;

    luaA_checktable(L, 2);
    lua_pushliteral(L, "selection");
    lua_gettable(L, 2);
    lua_pushliteral(L, "target");
    lua_gettable(L, 2);

    name = luaL_checklstring(L, -2, &name_length);
    target = luaL_checklstring(L, -1, &target_length);

    /* Create a selection object */
    selection = (void *) selection_getter_class.allocator(L);
    selection->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.screen->root_depth,
            selection->window, globalconf.screen->root, -1, -1, 1, 1, 0,
            XCB_COPY_FROM_PARENT, globalconf.screen->root_visual, 0, NULL);

    /* Save it in the registry */
    lua_pushliteral(L, REGISTRY_GETTER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, -2);
    selection->ref = luaL_ref(L, -2);
    lua_pop(L, 1);

    /* Get the atoms identifying the request */
    cookies[0] = xcb_intern_atom_unchecked(globalconf.connection, false, name_length, name);
    cookies[1] = xcb_intern_atom_unchecked(globalconf.connection, false, target_length, target);

    reply = xcb_intern_atom_reply(globalconf.connection, cookies[0], NULL);
    name_atom = reply ? reply->atom : XCB_NONE;
    p_delete(&reply);

    reply = xcb_intern_atom_reply(globalconf.connection, cookies[1], NULL);
    target_atom = reply ? reply->atom : XCB_NONE;
    p_delete(&reply);

    xcb_convert_selection(globalconf.connection, selection->window, name_atom,
            target_atom, AWESOME_SELECTION_ATOM, globalconf.timestamp);

    return 1;
}

static void
selection_transfer_finished(lua_State *L, int ud)
{
    selection_getter_t *selection = lua_touserdata(L, ud);

    /* Unreference the selection object; it's dead */
    lua_pushliteral(L, REGISTRY_GETTER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    luaL_unref(L, -1, selection->ref);
    lua_pop(L, 1);

    selection->ref = LUA_NOREF;

    luaA_object_emit_signal(L, ud, "data_end", 0);
}

static void
selection_push_data(lua_State *L, xcb_get_property_reply_t *property)
{
    if (property->type == XCB_ATOM_ATOM && property->format == 32) {
        size_t num_atoms = xcb_get_property_value_length(property) / 4;
        xcb_atom_t *atoms = xcb_get_property_value(property);
        xcb_get_atom_name_cookie_t cookies[num_atoms];

        for (size_t i = 0; i < num_atoms; i++)
                cookies[i] = xcb_get_atom_name_unchecked(globalconf.connection, atoms[i]);

        lua_newtable(L);
        for (size_t i = 0; i < num_atoms; i++) {
            xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(
                    globalconf.connection, cookies[i], NULL);
            if (reply)
            {
                lua_pushlstring(L, xcb_get_atom_name_name(reply), xcb_get_atom_name_name_length(reply));
                lua_rawseti(L, -2, i+1);
            }
            p_delete(&reply);
        }
    } else {
        lua_pushlstring(L, xcb_get_property_value(property), xcb_get_property_value_length(property));
    }
}

static void
selection_handle_selectionnotify(lua_State *L, int ud, xcb_atom_t property)
{
    selection_getter_t *selection;

    ud = luaA_absindex(L, ud);
    selection = lua_touserdata(L, ud);

    if (property != XCB_NONE)
    {
        xcb_change_window_attributes(globalconf.connection, selection->window,
            XCB_CW_EVENT_MASK, (uint32_t[]) { XCB_EVENT_MASK_PROPERTY_CHANGE });

        xcb_get_property_reply_t *property_r = xcb_get_property_reply(globalconf.connection,
                xcb_get_property(globalconf.connection, true, selection->window, AWESOME_SELECTION_ATOM,
                    XCB_GET_PROPERTY_TYPE_ANY, 0, 0xffffffff), NULL);

        if (property_r)
        {
            if (property_r->type == INCR)
            {
                /* This is an incremental transfer. The above GetProperty had
                 * delete=true. This indicates to the other end that the
                 * transfer should start now. Right now we only get an estimate
                 * of the size of the data to be transferred, which we ignore.
                 */
                p_delete(&property_r);
                return;
            }
            selection_push_data(L, property_r);
            luaA_object_emit_signal(L, ud, "data", 1);
            p_delete(&property_r);
        }
    }

    selection_transfer_finished(L, ud);
}

static int
selection_getter_find_by_window(lua_State *L, xcb_window_t window)
{
    /* Iterate over all active selection getters */
    lua_pushliteral(L, REGISTRY_GETTER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -1) == LUA_TUSERDATA) {
            selection_getter_t *selection = lua_touserdata(L, -1);
            if (selection->window == window) {
                /* Found the right selection, remove table and key */
                lua_remove(L, -2);
                lua_remove(L, -2);
                return 1;
            }
        }
        /* Remove the value, leaving only the key */
        lua_pop(L, 1);
    }
    /* Remove the getter table */
    lua_pop(L, 1);

    return 0;
}

int
property_handle_awesome_selection_atom(uint8_t state, xcb_window_t window)
{
    lua_State *L = globalconf_get_lua_State();

    if (state != XCB_PROPERTY_NEW_VALUE)
        return 0;

    if (selection_getter_find_by_window(L, window) == 0)
        return 0;

    selection_getter_t *selection = lua_touserdata(L, -1);

    xcb_get_property_reply_t *property_r = xcb_get_property_reply(globalconf.connection,
            xcb_get_property(globalconf.connection, true, selection->window, AWESOME_SELECTION_ATOM,
                XCB_GET_PROPERTY_TYPE_ANY, 0, 0xffffffff), NULL);

    if (property_r)
    {
        if (property_r->value_len > 0)
        {
            selection_push_data(L, property_r);
            luaA_object_emit_signal(L, -2, "data", 1);
        }
        else
        {
            /* Transfer finished */
            selection_transfer_finished(L, -1);
        }

        p_delete(&property_r);
    }

    lua_pop(L, 1);
    return 0;
}

void
event_handle_selectionnotify(xcb_selection_notify_event_t *ev)
{
    lua_State *L = globalconf_get_lua_State();

    if (selection_getter_find_by_window(L, ev->requestor) == 0)
        return;

    selection_handle_selectionnotify(L, -1, ev->property);
    lua_pop(L, 1);
}

void
selection_getter_class_setup(lua_State *L)
{
    static const struct luaL_Reg selection_getter_methods[] =
    {
        { "__call", luaA_selection_getter_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg selection_getter_metha[] = {
        LUA_OBJECT_META(selection_getter)
        LUA_CLASS_META
        { NULL, NULL }
    };

    /* Store a table in the registry that tracks active getters. This code does
     * debug.getregistry(){REGISTRY_GETTER_TABLE_INDEX] = {}
     */
    lua_pushliteral(L, REGISTRY_GETTER_TABLE_INDEX);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    luaA_class_setup(L, &selection_getter_class, "selection_getter", NULL,
            (lua_class_allocator_t) selection_getter_new,
            (lua_class_collector_t) selection_getter_wipe, NULL,
            luaA_class_index_miss_property, luaA_class_newindex_miss_property,
            selection_getter_methods, selection_getter_metha);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
