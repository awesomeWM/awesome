/*
 * xkb.c - keyboard layout control functions
 *
 * Copyright © 2015 Aleksey Fedotov <lexa@cfotr.com>
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

#include "xkb.h"
#include "globalconf.h"
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>


/* \brief switch keyboard layout
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam layout number, integer from 0 to 3
 */
int
luaA_xkb_set_layout_group(lua_State *L)
{
    unsigned group = luaL_checkinteger(L, 1);
    xcb_xkb_latch_lock_state (globalconf.connection, XCB_XKB_ID_USE_CORE_KBD,
                              0, 0, true, group, 0, 0, 0);
    return 0;
}

/* \brief get current layout number
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn current layout number, integer from 0 to 3
 */
int
luaA_xkb_get_layout_group(lua_State *L)
{
    xcb_xkb_get_state_cookie_t state_c;
    state_c = xcb_xkb_get_state_unchecked (globalconf.connection,
                                           XCB_XKB_ID_USE_CORE_KBD);
    xcb_xkb_get_state_reply_t* state_r;
    state_r = xcb_xkb_get_state_reply (globalconf.connection,
                                       state_c, NULL);
    if (!state_r)
    {
        free(state_r);
        return 0;
    }
    lua_pushinteger(L, state_r->group);
    free(state_r);
    return 1;
}

/* \brief get layout short names
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lreturn string describing current layout settings, \
 * example: 'pc+us+de:2+inet(evdev)+group(alt_shift_toggle)+ctrl(nocaps)'
 */
int
luaA_xkb_get_group_names(lua_State *L)
{
    xcb_xkb_get_names_cookie_t name_c;
    name_c = xcb_xkb_get_names_unchecked (globalconf.connection,
                                          XCB_XKB_ID_USE_CORE_KBD,
                                          XCB_XKB_NAME_DETAIL_SYMBOLS);
    xcb_xkb_get_names_reply_t* name_r;
    name_r = xcb_xkb_get_names_reply (globalconf.connection, name_c, NULL);

    if (!name_r)
    {
        luaA_warn(L, "Failed to get xkb symbols name");
        return 0;
    }

    xcb_xkb_get_names_value_list_t name_list;
    void *buffer = xcb_xkb_get_names_value_list(name_r);
    xcb_xkb_get_names_value_list_unpack (
        buffer, name_r->nTypes, name_r->indicators,
        name_r->virtualMods, name_r->groupNames, name_r->nKeys,
        name_r->nKeyAliases, name_r->nRadioGroups, name_r->which,
        &name_list);

    xcb_get_atom_name_cookie_t atom_name_c;
    atom_name_c = xcb_get_atom_name_unchecked(globalconf.connection, name_list.symbolsName);
    xcb_get_atom_name_reply_t *atom_name_r;
    atom_name_r = xcb_get_atom_name_reply(globalconf.connection, atom_name_c, NULL);
    if (!atom_name_r) {
        luaA_warn(L, "Failed to get atom symbols name");
        free(name_r);
        return 0;
    }

    const char *name = xcb_get_atom_name_name(atom_name_r);
    size_t name_len = xcb_get_atom_name_name_length(atom_name_r);
    lua_pushlstring(L, name, name_len);

    free(atom_name_r);
    free(name_r);
    return 1;
}

/** Fill globalconf.xkb_state based on connection and context
*/
static void
xkb_fill_state(void)
{
    xcb_connection_t *conn = globalconf.connection;

    int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1)
        fatal("Failed while getting XKB device id");
    
    struct xkb_keymap *xkb_keymap = xkb_x11_keymap_new_from_device(
                                globalconf.xkb_ctx,
                                conn,
                                device_id,
                                XKB_KEYMAP_COMPILE_NO_FLAGS);


    if (!xkb_keymap)
        fatal("Failed while getting XKB keymap from device");
    
    globalconf.xkb_state = xkb_x11_state_new_from_device(xkb_keymap,
                                                         conn,
                                                         device_id);
    if (!globalconf.xkb_state)
        fatal("Failed while getting XKB state from device");

    /* xkb_keymap is no longer referenced directly; decreasing refcount */
    xkb_keymap_unref(xkb_keymap);
}


/** Loads xkb context, state and keymap to globalconf.
 * These variables should be freed by xkb_free_keymap() afterwards
*/
static void
xkb_init_keymap(void)
{
    globalconf.xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!globalconf.xkb_ctx)
        fatal("Failed while getting XKB context");
    
    xkb_fill_state();
}

/** Frees xkb context, state and keymap from globalconf.
 * This should be used when these variables will not be used anymore
 */
static void
xkb_free_keymap(void)
{
    xkb_state_unref(globalconf.xkb_state);
    xkb_context_unref(globalconf.xkb_ctx);
}

/** Rereads the state of keyboard from X.
 * This call should be used after receiving NewKeyboardNotify or MapNotify,
 * as written in http://xkbcommon.org/doc/current/group__x11.html
 */
static void
xkb_reload_keymap(void)
{
    xkb_state_unref(globalconf.xkb_state);
    xkb_fill_state();
}

/** The xkb notify event handler.
 * \param event The event.
 */
void
event_handle_xkb_notify(xcb_generic_event_t* event)
{
    lua_State *L = globalconf_get_lua_State();

    /* The pad0 field of xcb_generic_event_t contains the event sub-type,
     * unfortunately xkb doesn't provide a usable struct for getting this in a
     * nicer way*/
    switch (event->pad0)
    {
      case XCB_XKB_NEW_KEYBOARD_NOTIFY:
        {
          xcb_xkb_new_keyboard_notify_event_t *new_keyboard_event = (void*)event;

          xkb_reload_keymap();

          if (new_keyboard_event->changed & XCB_XKB_NKN_DETAIL_KEYCODES)
          {
              signal_object_emit(L, &global_signals, "xkb::map_changed", 0);
          }
          break;
        }
      case XCB_XKB_MAP_NOTIFY:
        {
          xkb_reload_keymap();
          break;
        }
      case XCB_XKB_NAMES_NOTIFY:
        {
          signal_object_emit(L, &global_signals, "xkb::map_changed", 0);
          break;
        }
      case XCB_XKB_STATE_NOTIFY:
        {
          xcb_xkb_state_notify_event_t *state_notify_event = (void*)event;

          xkb_state_update_mask(globalconf.xkb_state, 
                                state_notify_event->baseMods,
                                state_notify_event->latchedMods,
                                state_notify_event->lockedMods,
                                state_notify_event->baseGroup,
                                state_notify_event->latchedGroup,
                                state_notify_event->lockedGroup);

          if (state_notify_event->changed & XCB_XKB_STATE_PART_GROUP_STATE)
          {
              lua_pushnumber(L, state_notify_event->group);
              signal_object_emit(L, &global_signals, "xkb::group_changed", 1);
          }

          break;
        }
    }
}

/** Initialize XKB support
 * This call allocates resources, that should be freed by calling xkb_free()
 */
void
xkb_init(void)
{

    /* check that XKB extension present in this X server */
    const xcb_query_extension_reply_t *xkb_r;
    xkb_r = xcb_get_extension_data(globalconf.connection, &xcb_xkb_id);
    if (!xkb_r || !xkb_r->present)
    {
        fatal("Xkb extension not present");
    }

    /* check xkb version */
    xcb_xkb_use_extension_cookie_t ext_cookie = xcb_xkb_use_extension(globalconf.connection, 1, 0);
    xcb_xkb_use_extension_reply_t *ext_reply = xcb_xkb_use_extension_reply (globalconf.connection, ext_cookie, NULL);
    if (!ext_reply || !ext_reply->supported)
    {
        fatal("Required xkb extension is not supported");
    }
    unsigned int map = XCB_XKB_EVENT_TYPE_STATE_NOTIFY | XCB_XKB_EVENT_TYPE_MAP_NOTIFY | XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY;

    //
    // These maps are provided to allow key remapping,
    // that could be used in awesome
    //
    uint16_t map_parts = XCB_XKB_MAP_PART_KEY_TYPES |
                         XCB_XKB_MAP_PART_KEY_SYMS |
                         XCB_XKB_MAP_PART_MODIFIER_MAP |
                         XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                         XCB_XKB_MAP_PART_KEY_ACTIONS |
                         XCB_XKB_MAP_PART_KEY_BEHAVIORS |
                         XCB_XKB_MAP_PART_VIRTUAL_MODS | 
                         XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;

    xcb_xkb_select_events_checked(globalconf.connection,
                                  XCB_XKB_ID_USE_CORE_KBD,
                                  map,
                                  0,
                                  map,
                                  map_parts,
                                  map_parts,
                                  0);

    /* load keymap to use when resolving keypresses */
    xkb_init_keymap();
}

/** Frees resources allocated by xkb_init()
 */
void
xkb_free(void)
{
    xkb_free_keymap();
}
