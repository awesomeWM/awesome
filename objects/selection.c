/*
 * selection.c - X11 selection interface
 *
 * Copyright © 2017 Uli Schlachter <psychon@znc.in>
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

/** awesome selection API
 *
 * TODO: Write some docs
 *
 * Some signal names are starting with a dot. These dots are artefacts from
 * the documentation generation, you get the real signal name by
 * removing the starting dot.
 *
 * @author Uli Schlachter &lt;psychon@znc.in&gt;
 * @copyright 2017 Uli Schlachter
 * @classmod selection
 */

#include "objects/selection.h"
#include "globalconf.h"
#include "common/atoms.h"

/** Selection object.
 *
 * @tfield string foo Some foo so that I remember the doc syntax.
 * @table selection
 */

/**
 * An undocumented signal
 * @signal .bar
 */

/** Get the number of instances.
 *
 * @return The number of selection objects alive.
 * @function instances
 */

/** Set a __index metamethod for all selection instances.
 * @tparam function cb The meta-method
 * @function set_index_miss_handler
 */

/** Set a __newindex metamethod for all selection instances.
 * @tparam function cb The meta-method
 * @function set_newindex_miss_handler
 */

static lua_class_t selection_class;
LUA_OBJECT_FUNCS(selection_class, selection_t, selection)

static void
selection_wipe(selection_t *w)
{
    puts("TODO: Implement selection freeing");
}

static xcb_atom_t
get_atom(const char *name, size_t name_length)
{
    xcb_atom_t atom;
    xcb_intern_atom_cookie_t atom_q = xcb_intern_atom_unchecked(
            globalconf.connection, false, name_length, name);
    xcb_intern_atom_reply_t *atom_r = xcb_intern_atom_reply(
            globalconf.connection, atom_q, NULL);
    if(!atom_r)
        return XCB_NONE;
    atom = atom_r->atom;
    p_delete(&atom_r);
    return atom;
}

/** Create a new selection object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_selection_new(lua_State *L)
{
    size_t name_length;
    const char *name = luaL_checklstring(L, 2, &name_length);
    selection_t *selection = (void*) selection_class.allocator(L);

    /* Query the atom for the given selection name */
    selection->selection = get_atom(name, name_length);

    /* Create a window for this selection */
    selection->window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.screen->root_depth,
            selection->window, globalconf.screen->root, -1, -1, 1, 1, 0,
            XCB_COPY_FROM_PARENT, globalconf.screen->root_visual,
            XCB_CW_EVENT_MASK, (uint32_t[]) { XCB_EVENT_MASK_PROPERTY_CHANGE });

    return 1;
}

/** TODO docs
 *
 * @param A table with coordinates to modify.
 * @return A table with drawin coordinates and geometry.
 * @function request
 */
static int
luaA_selection_request(lua_State *L)
{
    selection_t *selection = luaA_checkudata(L, 1, &selection_class);
    size_t name_length;
    const char *name = luaL_checklstring(L, 2, &name_length);
    luaA_checkfunction(L, 3);

    xcb_atom_t target = get_atom(name, name_length);

    if (selection->callback_function != NULL)
        luaL_error(L, "Selection is currently busy");
    selection->callback_function = luaA_object_ref_item(L, 1, 3);
    xcb_convert_selection(globalconf.connection, selection->window, selection->selection,
            target, SOME_RANDOM_ATOM, globalconf.timestamp);

    lua_pushvalue(L, 1);
    int_array_append(&globalconf.active_selections, luaL_ref(L, LUA_REGISTRYINDEX));
    return 0;
}

static int
selection_push_atom(lua_State *L, xcb_get_property_reply_t *property)
{
    size_t num_atoms = xcb_get_property_value_length(property) / 4;
    xcb_atom_t *atoms = xcb_get_property_value(property);
    xcb_get_atom_name_cookie_t cookies[num_atoms];
    for (size_t i = 0; i < num_atoms; i++)
        cookies[i] = xcb_get_atom_name(globalconf.connection, atoms[i]);
    for (size_t i = 0; i < num_atoms; i++)
    {
        xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(
                globalconf.connection, cookies[i], NULL);
        if(reply)
            lua_pushlstring(L, xcb_get_atom_name_name(reply),
                    xcb_get_atom_name_name_length(reply));
        else
            lua_pushnil(L);
        p_delete(&reply);
    }
    return num_atoms;
}

static int
selection_push_string(lua_State *L, xcb_get_property_reply_t *property)
{
    lua_pushlstring(L, xcb_get_property_value(property),
            xcb_get_property_value_length(property));
    return 1;
}

static int
selection_push_unknown(lua_State *L, xcb_selection_notify_event_t *notify, xcb_get_property_reply_t *property)
{
    lua_pushstring(L, "Unknown/unimplemented property type");
    return 1;
}

static int
selection_push_notify(lua_State *L, xcb_selection_notify_event_t *notify, xcb_get_property_reply_t *property)
{
    if(property->type == XCB_ATOM_ATOM && property->format == 32)
        return selection_push_atom(L, property);
    if((property->type == UTF8_STRING || property->type == XCB_ATOM_STRING ||
                property->type == COMPOUND_TEXT) && property->format == 8)
        return selection_push_string(L, property);
    return selection_push_unknown(L, notify, property);
}

bool
selection_handle_notify(lua_State *L, int ud, xcb_selection_notify_event_t *notify)
{
    ud = luaA_absindex(L, ud);
    selection_t *selection = (void *) lua_topointer(L, ud);

    int nargs = 0;
    if (notify->property != XCB_NONE)
    {
        xcb_get_property_reply_t *property = xcb_get_property_reply(globalconf.connection,
                xcb_get_property(globalconf.connection, 1, selection->window, SOME_RANDOM_ATOM,
                    XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX), NULL);
        if (!property)
            return false;

        nargs = selection_push_notify(L, notify, property);
        p_delete(&property);
    }

    luaA_object_push_item(L, ud, selection->callback_function);
    luaA_dofunction(L, nargs, 0);

    luaA_object_unref_item(L, ud, selection->callback_function);
    selection->callback_function = NULL;

    /* Signal to caller that transfer is done */
    return true;
}

static int
luaA_selection_get_busy(lua_State *L, selection_t *selection)
{
    lua_pushboolean(L, selection->callback_function != NULL);
    return 1;
}

void
selection_class_setup(lua_State *L)
{
    static const struct luaL_Reg selection_methods[] =
    {
        LUA_CLASS_METHODS(key)
        { "__call", luaA_selection_new },
        { NULL, NULL }
    };

    static const struct luaL_Reg selection_meta[] =
    {
        LUA_OBJECT_META(selection)
        LUA_CLASS_META
        { "request", luaA_selection_request },
        { NULL, NULL },
    };

    luaA_class_setup(L, &selection_class, "selection", NULL,
                     (lua_class_allocator_t) selection_new,
                     (lua_class_collector_t) selection_wipe,
                     NULL,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     selection_methods, selection_meta);
    luaA_class_add_property(&selection_class, "busy",
                            (lua_class_propfunc_t) NULL,
                            (lua_class_propfunc_t) luaA_selection_get_busy,
                            (lua_class_propfunc_t) NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
