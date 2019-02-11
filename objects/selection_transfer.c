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
#include "common/atoms.h"
#include "globalconf.h"

#define REGISTRY_TRANSFER_TABLE_INDEX "awesome_selection_transfers"
#define TRANSFER_DATA_INDEX "data_for_next_chunk"

enum transfer_state {
    TRANSFER_WAIT_FOR_DATA,
    TRANSFER_INCREMENTAL,
    TRANSFER_DONE
};

typedef struct selection_transfer_t
{
    LUA_OBJECT_HEADER
    /** Reference in the special table to this object */
    int ref;
    /* Information from the xcb_selection_request_event_t */
    xcb_window_t requestor;
    xcb_atom_t selection;
    xcb_atom_t target;
    xcb_atom_t property;
    xcb_timestamp_t time;
    /* Current state of the transfer */
    enum transfer_state state;
    /* Offset into TRANSFER_DATA_INDEX for the next chunk of data */
    size_t offset;
} selection_transfer_t;

static lua_class_t selection_transfer_class;
LUA_OBJECT_FUNCS(selection_transfer_class, selection_transfer_t, selection_transfer)

static size_t max_property_length(void)
{
    uint32_t max_request_length = xcb_get_maximum_request_length(globalconf.connection);
    max_request_length = MIN(max_request_length, (1<<16) - 1);
    return max_request_length * 4 - sizeof(xcb_change_property_request_t);
}

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

static void
transfer_done(lua_State *L, selection_transfer_t *transfer)
{
    transfer->state = TRANSFER_DONE;

    lua_pushliteral(L, REGISTRY_TRANSFER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    luaL_unref(L, -1, transfer->ref);
    transfer->ref = LUA_NOREF;
    lua_pop(L, 1);
}

void
selection_transfer_begin(lua_State *L, int ud, xcb_window_t requestor,
        xcb_atom_t selection, xcb_atom_t target, xcb_atom_t property,
        xcb_timestamp_t time)
{
    ud = luaA_absindex(L, ud);

    /* Allocate a transfer object */
    selection_transfer_t *transfer = (void *) selection_transfer_class.allocator(L);
    transfer->requestor = requestor;
    transfer->selection = selection;
    transfer->target = target;
    transfer->property = property;
    transfer->time = time;
    transfer->state = TRANSFER_WAIT_FOR_DATA;

    /* Save the object in the registry */
    lua_pushliteral(L, REGISTRY_TRANSFER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, -2);
    transfer->ref = luaL_ref(L, -2);
    lua_pop(L, 1);

    /* Get the atom name */
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(globalconf.connection,
            xcb_get_atom_name_unchecked(globalconf.connection, target), NULL);
    if (reply) {
        lua_pushlstring(L, xcb_get_atom_name_name(reply),
                xcb_get_atom_name_name_length(reply));
        p_delete(&reply);
    } else
        lua_pushnil(L);

    /* Emit the request signal with target and transfer object */
    lua_pushvalue(L, -2);
    luaA_object_emit_signal(L, ud, "request", 2);

    /* Reject the transfer if Lua did not do anything */
    if (transfer->state == TRANSFER_WAIT_FOR_DATA) {
        selection_transfer_reject(requestor, selection, target, time);
        transfer_done(L, transfer);
    }

    /* Remove the transfer object from the stack */
    lua_pop(L, 1);
}

static int
luaA_selection_transfer_send(lua_State *L)
{
    size_t data_length;
    const char *data;
    bool incr = false;
    size_t incr_size = 0;

    selection_transfer_t *transfer = luaA_checkudata(L, 1, &selection_transfer_class);
    if (transfer->state != TRANSFER_WAIT_FOR_DATA)
        luaL_error(L, "Transfer object is not ready for more data to be sent");

    /* Get format and data from the table */
    luaA_checktable(L, 2);
    lua_pushliteral(L, "format");
    lua_rawget(L, 2);
    lua_pushliteral(L, "data");
    lua_rawget(L, 2);

    lua_pushliteral(L, "continue");
    lua_rawget(L, 2);
    incr = lua_toboolean(L, -1);
    if (incr && lua_isnumber(L, -1))
        incr_size = lua_tonumber(L, -1);
    lua_pop(L, 1);

    if (lua_isstring(L, -2)) {
        const char *format_string = luaL_checkstring(L, -2);
        if (A_STRNEQ(format_string, "atom"))
            luaL_error(L, "Unknown format '%s'", format_string);
        if (incr)
            luaL_error(L, "Cannot transfer atoms in pieces");

        /* 'data' is a table with strings */
        size_t len = luaA_rawlen(L, -1);

        /* Get an array with atoms */
        size_t atom_lengths[len];
        const char *atom_strings[len];
        for (size_t i = 0; i < len; i++) {
            lua_rawgeti(L, -1, i+1);
            atom_strings[i] = luaL_checklstring(L, -1, &atom_lengths[i]);
            lua_pop(L, 1);
        }

        xcb_intern_atom_cookie_t cookies[len];
        xcb_atom_t atoms[len];
        for (size_t i = 0; i < len; i++) {
            cookies[i] = xcb_intern_atom_unchecked(globalconf.connection, false,
                    atom_lengths[i], atom_strings[i]);
        }
        for (size_t i = 0; i < len; i++) {
            xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(globalconf.connection,
                    cookies[i], NULL);
            atoms[i] = reply ? reply->atom : XCB_NONE;
            p_delete(&reply);
        }

        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                transfer->requestor, transfer->property, XCB_ATOM_ATOM, 32,
                len, &atoms[0]);
    } else {
        /* 'data' is a string with the data to transfer */
        data = luaL_checklstring(L, -1, &data_length);

        if (!incr)
            incr_size = data_length;

        if (data_length >= max_property_length())
            incr = true;

        if (incr) {
            xcb_change_window_attributes(globalconf.connection,
                    transfer->requestor, XCB_CW_EVENT_MASK,
                    (uint32_t[]) { XCB_EVENT_MASK_PROPERTY_CHANGE });

            xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                    transfer->requestor, transfer->property, INCR, 32, 1,
                    (const uint32_t[]) { incr_size });

            /* Save the data on the transfer object */
            luaA_getuservalue(L, 1);
            lua_pushliteral(L, TRANSFER_DATA_INDEX);
            lua_pushvalue(L, -3);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            transfer->state = TRANSFER_INCREMENTAL;
            transfer->offset = 0;
        } else {
            xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                    transfer->requestor, transfer->property, UTF8_STRING, 8,
                    data_length, data);
        }
    }

    selection_transfer_notify(transfer->requestor, transfer->selection,
            transfer->target, transfer->property, transfer->time);
    if (!incr)
        transfer_done(L, transfer);

    return 0;
}

static void
transfer_continue_incremental(lua_State *L, int ud)
{
    const char *data;
    size_t data_length;
    selection_transfer_t *transfer = luaA_checkudata(L, ud, &selection_transfer_class);

    ud = luaA_absindex(L, ud);

    /* Get the data that is to be sent next */
    luaA_getuservalue(L, ud);
    lua_pushliteral(L, TRANSFER_DATA_INDEX);
    lua_rawget(L, -2);
    lua_remove(L, -2);

    data = luaL_checklstring(L, -1, &data_length);
    if (transfer->offset == data_length) {
        /* End of transfer */
        xcb_change_window_attributes(globalconf.connection,
                transfer->requestor, XCB_CW_EVENT_MASK,
                (uint32_t[]) { 0 });
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                transfer->requestor, transfer->property, UTF8_STRING, 8,
                0, NULL);
        transfer_done(L, transfer);
    } else {
        /* Send next piece of data */
        assert(transfer->offset < data_length);
        size_t next_length = MIN(data_length - transfer->offset, max_property_length());
        xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                transfer->requestor, transfer->property, UTF8_STRING, 8,
                next_length, &data[transfer->offset]);
        transfer->offset += next_length;
    }
    lua_pop(L, 1);
}

void
selection_transfer_handle_propertynotify(xcb_property_notify_event_t *ev)
{
    lua_State *L = globalconf_get_lua_State();

    if (ev->state != XCB_PROPERTY_DELETE)
        return;

    /* Iterate over all active selection acquire objects */
    lua_pushliteral(L, REGISTRY_TRANSFER_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -1) == LUA_TUSERDATA) {
            selection_transfer_t *transfer = lua_touserdata(L, -1);
            if (transfer->state == TRANSFER_INCREMENTAL
                    && transfer->requestor == ev->window
                    && transfer->property == ev->atom) {
                transfer_continue_incremental(L, -1);
                /* Remove table, key and transfer object */
                lua_pop(L, 3);
                return;
            }
        }
        /* Remove the value, leaving only the key */
        lua_pop(L, 1);
    }
    /* Remove the table */
    lua_pop(L, 1);
}

static bool
selection_transfer_checker(selection_transfer_t *transfer)
{
    return transfer->state != TRANSFER_DONE;
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
        { "send", luaA_selection_transfer_send },
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
