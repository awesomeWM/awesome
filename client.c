/*
 * client.c - client management
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb_atom.h>
#include <xcb/xcb_image.h>

#include "tag.h"
#include "ewmh.h"
#include "screen.h"
#include "titlebar.h"
#include "systray.h"
#include "property.h"
#include "spawn.h"
#include "luaa.h"
#include "common/atoms.h"
#include "common/xutil.h"

client_t *
luaA_client_checkudata(lua_State *L, int ud)
{
    client_t *c = luaA_checkudata(L, ud, &client_class);
    if(c->invalid)
        luaL_error(L, "client is invalid\n");
    return c;
}

/** Collect a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 */
static int
luaA_client_gc(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    button_array_wipe(&c->buttons);
    key_array_wipe(&c->keys);
    xcb_get_wm_protocols_reply_wipe(&c->protocols);
    p_delete(&c->machine);
    p_delete(&c->class);
    p_delete(&c->instance);
    p_delete(&c->icon_name);
    p_delete(&c->alt_icon_name);
    p_delete(&c->name);
    p_delete(&c->alt_name);
    return luaA_object_gc(L);
}

/** Change the client opacity.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param opacity The opacity.
 */
void
client_set_opacity(lua_State *L, int cidx, double opacity)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->opacity != opacity)
    {
        c->opacity = opacity;
        window_opacity_set(c->window, opacity);
        luaA_object_emit_signal(L, cidx, "property::opacity", 0);
    }
}

/** Change the clients urgency flag.
 * \param L The Lua VM state.
 * \param cidx The client index on the stack.
 * \param urgent The new flag state.
 */
void
client_set_urgent(lua_State *L, int cidx, bool urgent)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->urgent != urgent)
    {
        xcb_get_property_cookie_t hints =
            xcb_get_wm_hints_unchecked(globalconf.connection, c->window);

        c->urgent = urgent;
        ewmh_client_update_hints(c);

        /* update ICCCM hints */
        xcb_wm_hints_t wmh;
        xcb_get_wm_hints_reply(globalconf.connection, hints, &wmh, NULL);

        if(urgent)
            wmh.flags |= XCB_WM_HINT_X_URGENCY;
        else
            wmh.flags &= ~XCB_WM_HINT_X_URGENCY;

        xcb_set_wm_hints(globalconf.connection, c->window, &wmh);

        hook_property(c, "urgent");
        luaA_object_emit_signal(L, cidx, "property::urgent", 0);
    }
}

void
client_set_group_window(lua_State *L, int cidx, xcb_window_t window)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->group_window != window)
    {
        c->group_window = window;
        luaA_object_emit_signal(L, cidx, "property::group_window", 0);
    }
}

void
client_set_type(lua_State *L, int cidx, window_type_t type)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->type != type)
    {
        c->type = type;
        luaA_object_emit_signal(L, cidx, "property::type", 0);
    }
}

void
client_set_pid(lua_State *L, int cidx, uint32_t pid)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->pid != pid)
    {
        c->pid = pid;
        luaA_object_emit_signal(L, cidx, "property::pid", 0);
    }
}

void
client_set_icon_name(lua_State *L, int cidx, char *icon_name)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->icon_name);
    c->icon_name = icon_name;
    luaA_object_emit_signal(L, cidx, "property::icon_name", 0);
    hook_property(c, "icon_name");
}

void
client_set_alt_icon_name(lua_State *L, int cidx, char *icon_name)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->alt_icon_name);
    c->alt_icon_name = icon_name;
    luaA_object_emit_signal(L, cidx, "property::icon_name", 0);
    hook_property(c, "icon_name");
}

void
client_set_role(lua_State *L, int cidx, char *role)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->role);
    c->role = role;
    luaA_object_emit_signal(L, cidx, "property::role", 0);
}

void
client_set_machine(lua_State *L, int cidx, char *machine)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->machine);
    c->machine = machine;
    luaA_object_emit_signal(L, cidx, "property::machine", 0);
}

void
client_set_class_instance(lua_State *L, int cidx, const char *class, const char *instance)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->class);
    p_delete(&c->instance);
    c->class = a_strdup(class);
    c->instance = a_strdup(instance);
    luaA_object_emit_signal(L, cidx, "property::class", 0);
    luaA_object_emit_signal(L, cidx, "property::instance", 0);
}

void
client_set_transient_for(lua_State *L, int cidx, client_t *transient_for)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->transient_for != transient_for)
    {
        c->transient_for = transient_for;
        luaA_object_emit_signal(L, cidx, "property::transient_for", 0);
    }
}

void
client_set_name(lua_State *L, int cidx, char *name)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->name);
    c->name = name;
    hook_property(c, "name");
    luaA_object_emit_signal(L, cidx, "property::name", 0);
}

void
client_set_alt_name(lua_State *L, int cidx, char *name)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    p_delete(&c->alt_name);
    c->alt_name = name;
    hook_property(c, "name");
    luaA_object_emit_signal(L, cidx, "property::name", 0);
}

/** Returns true if a client is tagged
 * with one of the tags of the specified screen.
 * \param c The client to check.
 * \param screen Virtual screen.
 * \return true if the client is visible, false otherwise.
 */
bool
client_maybevisible(client_t *c, screen_t *screen)
{
    if(screen && c->screen == screen)
    {
        if(c->sticky || c->type == WINDOW_TYPE_DESKTOP)
            return true;

        foreach(tag, screen->tags)
            if(tag_get_selected(*tag) && is_client_tagged(c, *tag))
                return true;
    }
    return false;
}

/** Get a client by its window.
 * \param w The client window to find.
 * \return A client pointer if found, NULL otherwise.
 */
client_t *
client_getbywin(xcb_window_t w)
{
    foreach(c, globalconf.clients)
        if((*c)->window == w)
            return *c;

    return NULL;
}

/** Record that a client lost focus.
 * \param c Client being unfocused
 */
void
client_unfocus_update(client_t *c)
{
    globalconf.screens.tab[c->phys_screen].client_focus = NULL;
    ewmh_update_net_active_window(c->phys_screen);

    /* Call hook */
    if(globalconf.hooks.unfocus != LUA_REFNIL)
    {
        luaA_object_push(globalconf.L, c);
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.unfocus, 1, 0);
    }

    luaA_object_push(globalconf.L, c);
    luaA_class_emit_signal(globalconf.L, &client_class, "unfocus", 1);
}

/** Unfocus a client.
 * \param c The client.
 */
void
client_unfocus(client_t *c)
{

    xcb_window_t root_win = xutil_screen_get(globalconf.connection, c->phys_screen)->root;
    /* Set focus on root window, so no events leak to the current window.
     * This kind of inlines client_set_focus(), but a root window will never have
     * the WM_TAKE_FOCUS protocol.
     */
    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_PARENT,
                        root_win, XCB_CURRENT_TIME);

    client_unfocus_update(c);
}

/** Check if client supports atom a protocol in WM_PROTOCOL.
 * \param c The client.
 * \param atom The protocol atom to check for.
 * \return True if client has the atom in protocol, false otherwise.
 */
bool
client_hasproto(client_t *c, xcb_atom_t atom)
{
    for(uint32_t i = 0; i < c->protocols.atoms_len; i++)
        if(c->protocols.atoms[i] == atom)
            return true;
    return false;
}

/** Sets focus on window - using xcb_set_input_focus or WM_TAKE_FOCUS
 * \param c Client that should get focus
 * \param set_input_focus Should we call xcb_set_input_focus
 */
void
client_set_focus(client_t *c, bool set_input_focus)
{
    bool takefocus = client_hasproto(c, WM_TAKE_FOCUS);
    if(set_input_focus)
        xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_PARENT,
                            c->window, XCB_CURRENT_TIME);
    if(takefocus)
        window_takefocus(c->window);
}

/** Prepare banning a client by running all needed lua events.
 * \param c The client.
 */
void client_ban_unfocus(client_t *c)
{
    if(globalconf.screens.tab[c->phys_screen].prev_client_focus == c)
        globalconf.screens.tab[c->phys_screen].prev_client_focus = NULL;

    /* Wait until the last moment to take away the focus from the window. */
    if(globalconf.screens.tab[c->phys_screen].client_focus == c)
        client_unfocus(c);
}

/** Ban client and move it out of the viewport.
 * \param c The client.
 */
void
client_ban(client_t *c)
{
    if(!c->isbanned)
    {
        xcb_unmap_window(globalconf.connection, c->window);

        c->isbanned = true;

        client_ban_unfocus(c);
    }
}

/** This is part of The Bob Marley Algorithm: we ignore enter and leave window
 * in certain cases, like map/unmap or move, so we don't get spurious events.
 */
void
client_ignore_enterleave_events(void)
{
    foreach(c, globalconf.clients)
        xcb_change_window_attributes(globalconf.connection,
                                     (*c)->window,
                                     XCB_CW_EVENT_MASK,
                                     (const uint32_t []) { CLIENT_SELECT_INPUT_EVENT_MASK & ~(XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW) });
}

void
client_restore_enterleave_events(void)
{
    foreach(c, globalconf.clients)
        xcb_change_window_attributes(globalconf.connection,
                                     (*c)->window,
                                     XCB_CW_EVENT_MASK,
                                     (const uint32_t []) { CLIENT_SELECT_INPUT_EVENT_MASK });
}

/** Record that a client got focus.
 * \param c The client.
 */
void
client_focus_update(client_t *c)
{
    if(!client_maybevisible(c, c->screen))
    {
        /* Focus previously focused client */
        client_focus(globalconf.screen_focus->prev_client_focus);
        return;
    }

    if(globalconf.screen_focus
        && globalconf.screen_focus->client_focus)
    {
        if (globalconf.screen_focus->client_focus != c)
            client_unfocus_update(globalconf.screen_focus->client_focus);
        else
            /* Already focused */
            return;
    }
    luaA_object_push(globalconf.L, c);
    client_set_minimized(globalconf.L, -1, false);

    /* unban the client before focusing for consistency */
    client_unban(c);

    globalconf.screen_focus = &globalconf.screens.tab[c->phys_screen];
    globalconf.screen_focus->prev_client_focus = c;
    globalconf.screen_focus->client_focus = c;

    /* according to EWMH, we have to remove the urgent state from a client */
    client_set_urgent(globalconf.L, -1, false);

    ewmh_update_net_active_window(c->phys_screen);

    /* execute hook */
    if(globalconf.hooks.focus != LUA_REFNIL)
    {
        luaA_object_push(globalconf.L, c);
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.focus, 1, 0);
    }

    luaA_class_emit_signal(globalconf.L, &client_class, "focus", 1);
}

/** Give focus to client, or to first client if client is NULL.
 * \param c The client.
 */
void
client_focus(client_t *c)
{
    /* We have to set focus on first client */
    if(!c && globalconf.clients.len && !(c = globalconf.clients.tab[0]))
        return;

    if(!client_maybevisible(c, c->screen))
        return;

    if (!c->nofocus)
        client_focus_update(c);

    client_set_focus(c, !c->nofocus);
}

/** Stack a window below.
 * \param c The client.
 * \param previous The previous window on the stack.
 * \return The next-previous!
 */
static xcb_window_t
client_stack_above(client_t *c, xcb_window_t previous)
{
    uint32_t config_win_vals[2];

    config_win_vals[0] = previous;
    config_win_vals[1] = XCB_STACK_MODE_ABOVE;

    xcb_configure_window(globalconf.connection, c->window,
                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                         config_win_vals);

    config_win_vals[0] = c->window;

    if(c->titlebar)
    {
        xcb_configure_window(globalconf.connection,
                             c->titlebar->window,
                             XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                             config_win_vals);
        previous = c->titlebar->window;
    }
    else
        previous = c->window;

    /* stack transient window on top of their parents */
    foreach(node, globalconf.stack)
        if((*node)->transient_for == c)
            previous = client_stack_above(*node, previous);

    return previous;
}

/** Stacking layout layers */
typedef enum
{
    /** This one is a special layer */
    LAYER_IGNORE,
    LAYER_DESKTOP,
    LAYER_BELOW,
    LAYER_NORMAL,
    LAYER_ABOVE,
    LAYER_FULLSCREEN,
    LAYER_ONTOP,
    /** This one only used for counting and is not a real layer */
    LAYER_COUNT
} layer_t;

/** Get the real layer of a client according to its attribute (fullscreen, …)
 * \param c The client.
 * \return The real layer.
 */
static layer_t
client_layer_translator(client_t *c)
{
    /* first deal with user set attributes */
    if(c->ontop)
        return LAYER_ONTOP;
    else if(c->fullscreen)
        return LAYER_FULLSCREEN;
    else if(c->above)
        return LAYER_ABOVE;
    else if(c->below)
        return LAYER_BELOW;

    /* check for transient attr */
    if(c->transient_for)
        return LAYER_IGNORE;

    /* then deal with windows type */
    switch(c->type)
    {
      case WINDOW_TYPE_DESKTOP:
        return LAYER_DESKTOP;
      default:
        break;
    }

    return LAYER_NORMAL;
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
void
client_stack_refresh()
{
    uint32_t config_win_vals[2];
    layer_t layer;

    if (!globalconf.client_need_stack_refresh)
        return;
    globalconf.client_need_stack_refresh = false;

    config_win_vals[0] = XCB_NONE;
    config_win_vals[1] = XCB_STACK_MODE_ABOVE;

    /* stack desktop windows */
    for(layer = LAYER_DESKTOP; layer < LAYER_BELOW; layer++)
        foreach(node, globalconf.stack)
            if(client_layer_translator(*node) == layer)
                config_win_vals[0] = client_stack_above(*node,
                                                        config_win_vals[0]);

    /* first stack not ontop wibox window */
    foreach(_sb, globalconf.wiboxes)
    {
        wibox_t *sb = *_sb;
        if(!sb->ontop)
        {
            xcb_configure_window(globalconf.connection,
                                 sb->window,
                                 XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                 config_win_vals);
            config_win_vals[0] = sb->window;
        }
    }

    /* then stack clients */
    for(layer = LAYER_BELOW; layer < LAYER_COUNT; layer++)
        foreach(node, globalconf.stack)
            if(client_layer_translator(*node) == layer)
                config_win_vals[0] = client_stack_above(*node,
                                                        config_win_vals[0]);

    /* then stack ontop wibox window */
    foreach(_sb, globalconf.wiboxes)
    {
        wibox_t *sb = *_sb;
        if(sb->ontop)
        {
            xcb_configure_window(globalconf.connection,
                                 sb->window,
                                 XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                                 config_win_vals);
            config_win_vals[0] = sb->window;
        }
    }
}

/** Manage a new client.
 * \param w The window.
 * \param wgeom Window geometry.
 * \param phys_screen Physical screen number.
 * \param startup True if we are managing at startup time.
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, int phys_screen, bool startup)
{
    const uint32_t select_input_val[] = { CLIENT_SELECT_INPUT_EVENT_MASK };

    if(systray_iskdedockapp(w))
    {
        systray_request_handle(w, phys_screen, NULL);
        return;
    }

    /* If this is a new client that just has been launched, then request its
     * startup id. */
    xcb_get_property_cookie_t startup_id_q = { 0 };
    if(!startup)
        startup_id_q = xcb_get_any_property(globalconf.connection,
                                            false, w, _NET_STARTUP_ID, UINT_MAX);

    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);

    /* Make sure the window is automatically mapped if awesome exits or dies. */
    xcb_change_save_set(globalconf.connection, XCB_SET_MODE_INSERT, w);

    /* Move this window to the bottom of the stack. Without this we would force
     * other windows which will be above this one to redraw themselves because
     * this window occludes them for a tiny moment. The next stack_refresh()
     * will fix this up and move the window to its correct place. */
    xcb_configure_window(globalconf.connection, w,
                         XCB_CONFIG_WINDOW_STACK_MODE,
                         (uint32_t[]) { XCB_STACK_MODE_BELOW});

    client_t *c = client_new(globalconf.L);

    /* This cannot change, ever. */
    c->phys_screen = phys_screen;
    /* consider the window banned */
    c->isbanned = true;
    /* Store window */
    c->window = w;
    luaA_object_emit_signal(globalconf.L, -1, "property::window", 0);

    /* Duplicate client and push it in client list */
    lua_pushvalue(globalconf.L, -1);
    client_array_push(&globalconf.clients, luaA_object_ref(globalconf.L, -1));

    /* Set the right screen */
    screen_client_moveto(c, screen_getbycoord(&globalconf.screens.tab[phys_screen], wgeom->x, wgeom->y), false);

    /* Store initial geometry and emits signals so we inform that geometry have
     * been set. */
#define HANDLE_GEOM(attr) \
    c->geometry.attr = wgeom->attr; \
    c->geometries.internal.attr = wgeom->attr; \
    luaA_object_emit_signal(globalconf.L, -1, "property::" #attr, 0);
HANDLE_GEOM(x)
HANDLE_GEOM(y)
HANDLE_GEOM(width)
HANDLE_GEOM(height)
#undef HANDLE_GEOM

    luaA_object_emit_signal(globalconf.L, -1, "property::geometry", 0);

    /* Push client */
    client_set_border_width(globalconf.L, -1, wgeom->border_width);

    /* we honor size hints by default */
    c->size_hints_honor = true;
    luaA_object_emit_signal(globalconf.L, -1, "property::size_hints_honor", 0);

    /* update hints */
    property_update_wm_normal_hints(c, NULL);
    property_update_wm_hints(c, NULL);
    property_update_wm_transient_for(c, NULL);
    property_update_wm_client_leader(c, NULL);
    property_update_wm_client_machine(c, NULL);
    property_update_wm_window_role(c, NULL);
    property_update_net_wm_pid(c, NULL);
    property_update_net_wm_icon(c, NULL);

    /* get opacity */
    client_set_opacity(globalconf.L, -1, window_opacity_get(c->window));

    /* Then check clients hints */
    ewmh_client_check_hints(c);

    /* Push client in stack */
    client_raise(c);

    /* update window title */
    property_update_wm_name(c, NULL);
    property_update_net_wm_name(c, NULL);
    property_update_wm_icon_name(c, NULL);
    property_update_net_wm_icon_name(c, NULL);
    property_update_wm_class(c, NULL);
    property_update_wm_protocols(c, NULL);

    /* update strut */
    ewmh_process_client_strut(c, NULL);

    ewmh_update_net_client_list(c->phys_screen);

    /* Always stay in NORMAL_STATE. Even though iconified seems more
     * appropriate sometimes. The only possible loss is that clients not using
     * visibility events may continue to process data (when banned).
     * Without any exposes or other events the cost should be fairly limited though.
     *
     * Some clients may expect the window to be unmapped when STATE_ICONIFIED.
     * Two conflicting parts of the ICCCM v2.0 (section 4.1.4):
     *
     * "Normal -> Iconic - The client should send a ClientMessage event as described later in this section."
     * (note no explicit mention of unmapping, while Normal->Widthdrawn does mention that)
     *
     * "Once a client's window has left the Withdrawn state, the window will be mapped
     * if it is in the Normal state and the window will be unmapped if it is in the Iconic state."
     *
     * At this stage it's just safer to keep it in normal state and avoid confusion.
     */
    window_state_set(c->window, XCB_WM_STATE_NORMAL);

    if(!startup)
    {
        /* Request our response */
        xcb_get_property_reply_t *reply =
            xcb_get_property_reply(globalconf.connection, startup_id_q, NULL);
        /* Say spawn that a client has been started, with startup id as argument */
        char *startup_id = xutil_get_text_property_from_reply(reply);
        p_delete(&reply);
        spawn_start_notify(c, startup_id);
        p_delete(&startup_id);
    }

    /* Call hook to notify list change */
    if(globalconf.hooks.clients != LUA_REFNIL)
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.clients, 0, 0);

    luaA_class_emit_signal(globalconf.L, &client_class, "list", 0);

    /* call hook */
    if(globalconf.hooks.manage != LUA_REFNIL)
    {
        luaA_object_push(globalconf.L, c);
        lua_pushboolean(globalconf.L, startup);
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.manage, 2, 0);
    }

    /* client is still on top of the stack; push startup value,
     * and emit signals with 2 args */
    lua_pushboolean(globalconf.L, startup);
    luaA_class_emit_signal(globalconf.L, &client_class, "manage", 2);
}

/** Compute client geometry with respect to its geometry hints.
 * \param c The client.
 * \param geometry The geometry that the client might receive.
 * \return The geometry the client must take respecting its hints.
 */
area_t
client_geometry_hints(client_t *c, area_t geometry)
{
    int32_t basew, baseh, minw, minh;

    /* base size is substituted with min size if not specified */
    if(c->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
    {
        basew = c->size_hints.base_width;
        baseh = c->size_hints.base_height;
    }
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
    {
        basew = c->size_hints.min_width;
        baseh = c->size_hints.min_height;
    }
    else
        basew = baseh = 0;

    /* min size is substituted with base size if not specified */
    if(c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
    {
        minw = c->size_hints.min_width;
        minh = c->size_hints.min_height;
    }
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
    {
        minw = c->size_hints.base_width;
        minh = c->size_hints.base_height;
    }
    else
        minw = minh = 0;

    if(c->size_hints.flags & XCB_SIZE_HINT_P_ASPECT
       && c->size_hints.min_aspect_num > 0
       && c->size_hints.min_aspect_den > 0
       && geometry.height - baseh > 0
       && geometry.width - basew > 0)
    {
        double dx = (double) (geometry.width - basew);
        double dy = (double) (geometry.height - baseh);
        double min = (double) c->size_hints.min_aspect_num / (double) c->size_hints.min_aspect_den;
        double max = (double) c->size_hints.max_aspect_num / (double) c->size_hints.min_aspect_den;
        double ratio = dx / dy;
        if(max > 0 && min > 0 && ratio > 0)
        {
            if(ratio < min)
            {
                dy = (dx * min + dy) / (min * min + 1);
                dx = dy * min;
                geometry.width = (int) dx + basew;
                geometry.height = (int) dy + baseh;
            }
            else if(ratio > max)
            {
                dy = (dx * min + dy) / (max * max + 1);
                dx = dy * min;
                geometry.width = (int) dx + basew;
                geometry.height = (int) dy + baseh;
            }
        }
    }

    if(minw)
        geometry.width = MAX(geometry.width, minw);
    if(minh)
        geometry.height = MAX(geometry.height, minh);

    if(c->size_hints.flags & XCB_SIZE_HINT_P_MAX_SIZE)
    {
        if(c->size_hints.max_width)
            geometry.width = MIN(geometry.width, c->size_hints.max_width);
        if(c->size_hints.max_height)
            geometry.height = MIN(geometry.height, c->size_hints.max_height);
    }

    if(c->size_hints.flags & (XCB_SIZE_HINT_P_RESIZE_INC | XCB_SIZE_HINT_BASE_SIZE)
       && c->size_hints.width_inc && c->size_hints.height_inc)
    {
        uint16_t t1 = geometry.width, t2 = geometry.height;
        unsigned_subtract(t1, basew);
        unsigned_subtract(t2, baseh);
        geometry.width -= t1 % c->size_hints.width_inc;
        geometry.height -= t2 % c->size_hints.height_inc;
    }

    return geometry;
}

/** Resize client window.
 * The sizes given as parameters are with titlebar and borders!
 * \param c Client to resize.
 * \param geometry New window geometry.
 * \param hints Use size hints.
 * \return true if an actual resize occurred.
 */
bool
client_resize(client_t *c, area_t geometry, bool hints)
{
    area_t geometry_internal;
    area_t area;

    /* offscreen appearance fixes */
    area = display_area_get(c->phys_screen);

    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height;
    if(geometry.x + geometry.width < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height < 0)
        geometry.y = 0;

    /* Real client geometry, please keep it contained to C code at the very least. */
    geometry_internal = titlebar_geometry_remove(c->titlebar, c->border_width, geometry);

    if(hints && !c->fullscreen)
        geometry_internal = client_geometry_hints(c, geometry_internal);

    if(geometry_internal.width == 0 || geometry_internal.height == 0)
        return false;

    /* Also let client hints propagate to the "official" geometry. */
    geometry = titlebar_geometry_add(c->titlebar, c->border_width, geometry_internal);

    if(c->geometries.internal.x != geometry_internal.x
       || c->geometries.internal.y != geometry_internal.y
       || c->geometries.internal.width != geometry_internal.width
       || c->geometries.internal.height != geometry_internal.height)
    {
        screen_t *new_screen = screen_getbycoord(c->screen,
                                                 geometry_internal.x, geometry_internal.y);

        /* Values to configure a window is an array where values are
         * stored according to 'value_mask' */
        uint32_t values[4];

        c->geometries.internal.x = values[0] = geometry_internal.x;
        c->geometries.internal.y = values[1] = geometry_internal.y;
        c->geometries.internal.width = values[2] = geometry_internal.width;
        c->geometries.internal.height = values[3] = geometry_internal.height;

        /* Also store geometry including border and titlebar. */
        c->geometry = geometry;

        titlebar_update_geometry(c);

        /* Ignore all spurious enter/leave notify events */
        client_ignore_enterleave_events();

        xcb_configure_window(globalconf.connection, c->window,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                             | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             values);

        client_restore_enterleave_events();

        screen_client_moveto(c, new_screen, false);

        /* execute hook */
        hook_property(c, "geometry");

        luaA_object_push(globalconf.L, c);
        luaA_object_emit_signal(globalconf.L, -1, "property::geometry", 0);
        /** \todo This need to be VERIFIED before it is emitted! */
        luaA_object_emit_signal(globalconf.L, -1, "property::x", 0);
        luaA_object_emit_signal(globalconf.L, -1, "property::y", 0);
        luaA_object_emit_signal(globalconf.L, -1, "property::width", 0);
        luaA_object_emit_signal(globalconf.L, -1, "property::height", 0);
        lua_pop(globalconf.L, 1);

        return true;
    }

    return false;
}

/** Set a client minimized, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client minimized.
 */
void
client_set_minimized(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->minimized != s)
    {
        c->minimized = s;
        banning_need_update((c)->screen);
        if(s)
            window_state_set(c->window, XCB_WM_STATE_ICONIC);
        else
            window_state_set(c->window, XCB_WM_STATE_NORMAL);
        ewmh_client_update_hints(c);
        if(strut_has_value(&c->strut))
            screen_emit_signal(globalconf.L, c->screen, "property::workarea", 0);
        /* execute hook */
        hook_property(c, "minimized");
        luaA_object_emit_signal(L, cidx, "property::minimized", 0);
    }
}

/** Set a client sticky, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client sticky.
 */
void
client_set_sticky(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->sticky != s)
    {
        c->sticky = s;
        banning_need_update((c)->screen);
        ewmh_client_update_hints(c);
        hook_property(c, "sticky");
        luaA_object_emit_signal(L, cidx, "property::sticky", 0);
    }
}

/** Set a client fullscreen, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client fullscreen.
 */
void
client_set_fullscreen(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->fullscreen != s)
    {
        area_t geometry;

        /* become fullscreen! */
        if(s)
        {
            /* Make sure the current geometry is stored without titlebar. */
            titlebar_ban(c->titlebar);
            /* remove any max state */
            client_set_maximized_horizontal(L, cidx, false);
            client_set_maximized_vertical(L, cidx, false);
            /* You can only be part of one of the special layers. */
            client_set_below(L, cidx, false);
            client_set_above(L, cidx, false);
            client_set_ontop(L, cidx, false);

            geometry = screen_area_get(c->screen, false);
            c->geometries.fullscreen = c->geometry;
            c->border_width_fs = c->border_width;
            client_set_border_width(L, cidx, 0);
            c->fullscreen = true;
        }
        else
        {
            geometry = c->geometries.fullscreen;
            c->fullscreen = false;
            client_set_border_width(L, cidx, c->border_width_fs);
        }
        client_resize(c, geometry, false);
        client_stack();
        ewmh_client_update_hints(c);
        hook_property(c, "fullscreen");
        luaA_object_emit_signal(L, cidx, "property::fullscreen", 0);
    }
}

/** Set a client horizontally maximized.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s The maximized status.
 */
void
client_set_maximized_horizontal(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->maximized_horizontal != s)
    {
        area_t geometry;

        if((c->maximized_horizontal = s))
        {
            /* remove fullscreen mode */
            client_set_fullscreen(L, cidx, false);

            geometry = screen_area_get(c->screen, true);
            geometry.y = c->geometry.y;
            geometry.height = c->geometry.height;
            c->geometries.max.x = c->geometry.x;
            c->geometries.max.width = c->geometry.width;
        }
        else
        {
            geometry = c->geometry;
            geometry.x = c->geometries.max.x;
            geometry.width = c->geometries.max.width;
        }

        client_resize(c, geometry, c->size_hints_honor);
        client_stack();
        ewmh_client_update_hints(c);
        hook_property(c, "maximized_horizontal");
        luaA_object_emit_signal(L, cidx, "property::maximized_horizontal", 0);
    }
}

/** Set a client vertically maximized.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s The maximized status.
 */
void
client_set_maximized_vertical(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->maximized_vertical != s)
    {
        area_t geometry;

        if((c->maximized_vertical = s))
        {
            /* remove fullscreen mode */
            client_set_fullscreen(L, cidx, false);

            geometry = screen_area_get(c->screen, true);
            geometry.x = c->geometry.x;
            geometry.width = c->geometry.width;
            c->geometries.max.y = c->geometry.y;
            c->geometries.max.height = c->geometry.height;
        }
        else
        {
            geometry = c->geometry;
            geometry.y = c->geometries.max.y;
            geometry.height = c->geometries.max.height;
        }

        client_resize(c, geometry, c->size_hints_honor);
        client_stack();
        ewmh_client_update_hints(c);
        hook_property(c, "maximized_vertical");
        luaA_object_emit_signal(L, cidx, "property::maximized_vertical", 0);
    }
}

/** Set a client above, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client above.
 */
void
client_set_above(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->above != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_set_below(L, cidx, false);
            client_set_ontop(L, cidx, false);
            client_set_fullscreen(L, cidx, false);
        }
        c->above = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hook_property(c, "above");
        luaA_object_emit_signal(L, cidx, "property::above", 0);
    }
}

/** Set a client below, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client below.
 */
void
client_set_below(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->below != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_set_above(L, cidx, false);
            client_set_ontop(L, cidx, false);
            client_set_fullscreen(L, cidx, false);
        }
        c->below = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hook_property(c, "below");
        luaA_object_emit_signal(L, cidx, "property::below", 0);
    }
}

/** Set a client modal, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client modal attribute.
 */
void
client_set_modal(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->modal != s)
    {
        c->modal = s;
        client_stack();
        ewmh_client_update_hints(c);
        /* execute hook */
        hook_property(c, "modal");
        luaA_object_emit_signal(L, cidx, "property::modal", 0);
    }
}

/** Set a client ontop, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client ontop attribute.
 */
void
client_set_ontop(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->ontop != s)
    {
        /* You can only be part of one of the special layers. */
        if(s)
        {
            client_set_above(L, cidx, false);
            client_set_below(L, cidx, false);
            client_set_fullscreen(L, cidx, false);
        }
        c->ontop = s;
        client_stack();
        /* execute hook */
        hook_property(c, "ontop");
        luaA_object_emit_signal(L, cidx, "property::ontop", 0);
    }
}

/** Set a client skip taskbar attribute.
 * \param L Tha Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client skip taskbar attribute.
 */
void
client_set_skip_taskbar(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_client_checkudata(L, cidx);

    if(c->skip_taskbar != s)
    {
        c->skip_taskbar = s;
        ewmh_client_update_hints(c);
        luaA_object_emit_signal(L, cidx, "property::skip_taskbar", 0);
    }
}

/** Unban a client and move it back into the viewport.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    if(c->isbanned)
    {
        xcb_map_window(globalconf.connection, c->window);

        c->isbanned = false;
    }
}

/** Unmanage a client.
 * \param c The client.
 */
void
client_unmanage(client_t *c)
{
    tag_array_t *tags = &c->screen->tags;

    /* Reset transient_for attributes of widows that maybe referring to us */
    foreach(_tc, globalconf.clients)
    {
        client_t *tc = *_tc;
        if(tc->transient_for == c)
            tc->transient_for = NULL;
    }

    if(globalconf.screens.tab[c->phys_screen].prev_client_focus == c)
        globalconf.screens.tab[c->phys_screen].prev_client_focus = NULL;

    if(globalconf.screens.tab[c->phys_screen].client_focus == c)
        client_unfocus(c);

    /* remove client from global list and everywhere else */
    foreach(elem, globalconf.clients)
        if(*elem == c)
        {
            client_array_remove(&globalconf.clients, elem);
            break;
        }
    stack_client_remove(c);
    for(int i = 0; i < tags->len; i++)
        untag_client(c, tags->tab[i]);

    /* call hook */
    if(globalconf.hooks.unmanage != LUA_REFNIL)
    {
        luaA_object_push(globalconf.L, c);
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.unmanage, 1, 0);
    }

    luaA_object_push(globalconf.L, c);
    luaA_class_emit_signal(globalconf.L, &client_class, "unmanage", 1);

    /* Call hook to notify list change */
    if(globalconf.hooks.clients != LUA_REFNIL)
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.clients, 0, 0);

    luaA_class_emit_signal(globalconf.L, &client_class, "list", 0);

    if(strut_has_value(&c->strut))
        screen_emit_signal(globalconf.L, c->screen, "property::workarea", 0);

    window_state_set(c->window, XCB_WM_STATE_WITHDRAWN);

    titlebar_client_detach(c);

    ewmh_update_net_client_list(c->phys_screen);

    /* set client as invalid */
    c->invalid = true;

    luaA_object_unref(globalconf.L, c);
}

/** Kill a client via a WM_DELETE_WINDOW request or KillClient if not
 * supported.
 * \param c The client to kill.
 */
void
client_kill(client_t *c)
{
    if(client_hasproto(c, WM_DELETE_WINDOW))
    {
        xcb_client_message_event_t ev;

        /* Initialize all of event's fields first */
        p_clear(&ev, 1);

        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = c->window;
        ev.format = 32;
        ev.data.data32[1] = XCB_CURRENT_TIME;
        ev.type = WM_PROTOCOLS;
        ev.data.data32[0] = WM_DELETE_WINDOW;

        xcb_send_event(globalconf.connection, false, c->window,
                       XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    }
    else
        xcb_kill_client(globalconf.connection, c->window);
}

/** Get all clients into a table.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam An optional screen number.
 * \lreturn A table with all clients.
 */
static int
luaA_client_get(lua_State *L)
{
    int i = 1, screen;

    screen = luaL_optnumber(L, 1, 0) - 1;

    lua_newtable(L);

    if(screen == -1)
        foreach(c, globalconf.clients)
        {
            luaA_object_push(L, *c);
            lua_rawseti(L, -2, i++);
        }
    else
    {
        luaA_checkscreen(screen);
        foreach(c, globalconf.clients)
            if((*c)->screen == &globalconf.screens.tab[screen])
            {
                luaA_object_push(L, *c);
                lua_rawseti(L, -2, i++);
            }
    }

    return 1;
}

/** Check if a client is visible on its screen.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lreturn A boolean value, true if the client is visible, false otherwise.
 */
static int
luaA_client_isvisible(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    lua_pushboolean(L, client_isvisible(c, c->screen));
    return 1;
}

/** Set client border width.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param width The border width.
 */
void
client_set_border_width(lua_State *L, int cidx, int width)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    uint32_t w = width;

    if(width > 0 && (c->type == WINDOW_TYPE_DOCK
                     || c->type == WINDOW_TYPE_SPLASH
                     || c->type == WINDOW_TYPE_DESKTOP
                     || c->fullscreen))
        return;

    if(width == c->border_width || width < 0)
        return;

    /* disallow change of border width if the client is fullscreen */
    if(c->fullscreen)
        return;

    /* Update geometry with the new border. */
    c->geometry.width -= 2 * c->border_width;
    c->geometry.height -= 2 * c->border_width;

    c->border_width = width;
    xcb_configure_window(globalconf.connection, c->window,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH, &w);

    c->geometry.width += 2 * c->border_width;
    c->geometry.height += 2 * c->border_width;

    /* Changing border size also affects the size of the titlebar. */
    titlebar_update_geometry(c);

    hook_property(c, "border_width");
    luaA_object_emit_signal(L, cidx, "property::border_width", 0);
}

/** Set a client icon.
 * \param L The Lua VM state.
 * \param cidx The client index on the stack.
 * \param iidx The image index on the stack.
 */
void
client_set_icon(lua_State *L, int cidx, int iidx)
{
    client_t *c = luaA_client_checkudata(L, cidx);
    /* convert index to absolute */
    cidx = luaA_absindex(L, cidx);
    iidx = luaA_absindex(L, iidx);
    luaA_checkudata(L, iidx, &image_class);
    luaA_object_unref_item(L, cidx, c->icon);
    c->icon = luaA_object_ref_item(L, cidx, iidx);
    luaA_object_emit_signal(L, cidx < iidx ? cidx : cidx - 1, "property::icon", 0);
    /* execute hook */
    hook_property(c, "icon");
}

/** Kill a client.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_kill(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    client_kill(c);
    return 0;
}

/** Swap a client with another one.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 * \lparam A client to swap with.
 */
static int
luaA_client_swap(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    client_t *swap = luaA_client_checkudata(L, 2);

    if(c != swap)
    {
        client_t **ref_c = NULL, **ref_swap = NULL;
        foreach(item, globalconf.clients)
        {
            if(*item == c)
                ref_c = item;
            else if(*item == swap)
                ref_swap = item;
            if(ref_c && ref_swap)
                break;
        }
        /* swap ! */
        *ref_c = swap;
        *ref_swap = c;

        /* Call hook to notify list change */
        if(globalconf.hooks.clients != LUA_REFNIL)
            luaA_dofunction_from_registry(L, globalconf.hooks.clients, 0, 0);

        luaA_class_emit_signal(globalconf.L, &client_class, "list", 0);
    }

    return 0;
}

/** Access or set the client tags.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \lparam A table with tags to set, or none to get the current tags table.
 * \return The clients tag.
 */
static int
luaA_client_tags(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    tag_array_t *tags = &c->screen->tags;
    int j = 0;

    if(lua_gettop(L) == 2)
    {
        luaA_checktable(L, 2);
        for(int i = 0; i < tags->len; i++)
            untag_client(c, tags->tab[i]);
        lua_pushnil(L);
        while(lua_next(L, 2))
            tag_client(c);
        lua_pop(L, 1);
    }

    lua_newtable(L);
    foreach(tag, *tags)
        if(is_client_tagged(c, *tag))
        {
            luaA_object_push(L, *tag);
            lua_rawseti(L, -2, ++j);
        }

    return 1;
}

/** Raise a client on top of others which are on the same layer.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_raise(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    client_raise(c);
    return 0;
}

/** Lower a client on bottom of others which are on the same layer.
 * \param L The Lua VM state.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_lower(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);

    stack_client_push(c);

    /* Traverse all transient layers. */
    for(client_t *tc = c->transient_for; tc; tc = tc->transient_for)
        stack_client_push(tc);

    client_stack();

    return 0;
}

/** Redraw a client by unmapping and mapping it quickly.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_redraw(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);

    xcb_unmap_window(globalconf.connection, c->window);
    xcb_map_window(globalconf.connection, c->window);

    /* Set the focus on the current window if the redraw has been
       performed on the window where the pointer is currently on
       because after the unmapping/mapping, the focus is lost */
    if(globalconf.screen_focus->client_focus == c)
    {
        client_unfocus(c);
        client_focus(c);
    }

    return 0;
}

/** Stop managing a client.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lvalue A client.
 */
static int
luaA_client_unmanage(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    client_unmanage(c);
    return 0;
}

/** Return client geometry.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new coordinates, or none.
 * \lreturn A table with client coordinates.
 */
static int
luaA_client_geometry(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        area_t geometry;

        luaA_checktable(L, 2);
        geometry.x = luaA_getopt_number(L, 2, "x", c->geometry.x);
        geometry.y = luaA_getopt_number(L, 2, "y", c->geometry.y);
        if(client_isfixed(c))
        {
            geometry.width = c->geometry.width;
            geometry.height = c->geometry.height;
        }
        else
        {
            geometry.width = luaA_getopt_number(L, 2, "width", c->geometry.width);
            geometry.height = luaA_getopt_number(L, 2, "height", c->geometry.height);
        }

        client_resize(c, geometry, c->size_hints_honor);
    }

    return luaA_pusharea(L, c->geometry);
}

/** Return client struts (reserved space at the edge of the screen).
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam A table with new strut values, or none.
 * \lreturn A table with strut values.
 */
static int
luaA_client_struts(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);

    if(lua_gettop(L) == 2)
    {
        luaA_tostrut(L, 2, &c->strut);
        ewmh_update_strut(c->window, &c->strut);
        hook_property(c, "struts");
        luaA_object_emit_signal(L, 1, "property::struts", 0);
        screen_emit_signal(L, c->screen, "property::workarea", 0);
    }

    return luaA_pushstrut(L, c->strut);
}

static int
luaA_client_set_screen(lua_State *L, client_t *c)
{
    if(globalconf.xinerama_is_active)
    {
        int screen = luaL_checknumber(L, -1) - 1;
        luaA_checkscreen(screen);
        screen_client_moveto(c, &globalconf.screens.tab[screen], true);
    }
    return 0;
}

static int
luaA_client_set_hidden(lua_State *L, client_t *c)
{
    bool b = luaA_checkboolean(L, -1);
    if(b != c->hidden)
    {
        c->hidden = b;
        banning_need_update((c)->screen);
        hook_property(c, "hidden");
        if(strut_has_value(&c->strut))
            screen_emit_signal(globalconf.L, c->screen, "property::workarea", 0);
        luaA_object_emit_signal(L, -3, "property::hidden", 0);
    }
    return 0;
}

static int
luaA_client_set_minimized(lua_State *L, client_t *c)
{
    client_set_minimized(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_fullscreen(lua_State *L, client_t *c)
{
    client_set_fullscreen(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_modal(lua_State *L, client_t *c)
{
    client_set_modal(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_maximized_horizontal(lua_State *L, client_t *c)
{
    client_set_maximized_horizontal(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_maximized_vertical(lua_State *L, client_t *c)
{
    client_set_maximized_vertical(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_icon(lua_State *L, client_t *c)
{
    client_set_icon(L, -3, -1);
    return 0;
}

static int
luaA_client_set_opacity(lua_State *L, client_t *c)
{
    if(lua_isnil(L, -1))
        client_set_opacity(L, -3, -1);
    else
        client_set_opacity(L, -3, luaL_checknumber(L, -1));
    return 0;
}

static int
luaA_client_set_sticky(lua_State *L, client_t *c)
{
    client_set_sticky(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_size_hints_honor(lua_State *L, client_t *c)
{
    c->size_hints_honor = luaA_checkboolean(L, -1);
    hook_property(c, "size_hints_honor");
    luaA_object_emit_signal(L, -3, "property::size_hints_honor", 0);
    return 0;
}

static int
luaA_client_set_border_width(lua_State *L, client_t *c)
{
    client_set_border_width(L, -3, luaL_checknumber(L, -1));
    return 0;
}

static int
luaA_client_set_ontop(lua_State *L, client_t *c)
{
    client_set_ontop(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_below(lua_State *L, client_t *c)
{
    client_set_below(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_above(lua_State *L, client_t *c)
{
    client_set_above(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_urgent(lua_State *L, client_t *c)
{
    client_set_urgent(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_set_border_color(lua_State *L, client_t *c)
{
    size_t len;
    const char *buf;
    if((buf = luaL_checklstring(L, -1, &len))
       && xcolor_init_reply(xcolor_init_unchecked(&c->border_color, buf, len)))
    {
        xcb_change_window_attributes(globalconf.connection, c->window,
                                     XCB_CW_BORDER_PIXEL, &c->border_color.pixel);
        luaA_object_emit_signal(L, -3, "property::border_color", 0);
    }
    return 0;
}

static int
luaA_client_set_titlebar(lua_State *L, client_t *c)
{
    if(lua_isnil(L, -1))
        titlebar_client_detach(c);
    else
        titlebar_client_attach(c);
    return 0;
}

static int
luaA_client_set_skip_taskbar(lua_State *L, client_t *c)
{
    client_set_skip_taskbar(L, -3, luaA_checkboolean(L, -1));
    return 0;
}

static int
luaA_client_get_name(lua_State *L, client_t *c)
{
    lua_pushstring(L, c->name ? c->name : c->alt_name);
    return 1;
}

static int
luaA_client_get_icon_name(lua_State *L, client_t *c)
{
    lua_pushstring(L, c->icon_name ? c->icon_name : c->alt_icon_name);
    return 1;
}

LUA_OBJECT_EXPORT_PROPERTY(client, client_t, class, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, instance, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, machine, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, role, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, transient_for, luaA_object_push)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, skip_taskbar, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, window, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, leader_window, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, group_window, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, pid, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, hidden, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, minimized, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, fullscreen, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, modal, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, ontop, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, urgent, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, above, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, below, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, sticky, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, size_hints_honor, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, maximized_horizontal, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, maximized_vertical, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, opacity, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, border_width, lua_pushnumber)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, border_color, luaA_pushxcolor)

static int
luaA_client_get_content(lua_State *L, client_t *c)
{
    xcb_image_t *ximage = xcb_image_get(globalconf.connection,
                                        c->window,
                                        0, 0,
                                        c->geometries.internal.width,
                                        c->geometries.internal.height,
                                        ~0, XCB_IMAGE_FORMAT_Z_PIXMAP);
    int retval = 0;

    if(ximage)
    {
        if(ximage->bpp >= 24)
        {
            uint32_t *data = p_alloca(uint32_t, ximage->width * ximage->height);

            for(int y = 0; y < ximage->height; y++)
                for(int x = 0; x < ximage->width; x++)
                {
                    data[y * ximage->width + x] = xcb_image_get_pixel(ximage, x, y);
                    data[y * ximage->width + x] |= 0xff000000; /* set alpha to 0xff */
                }

            retval = image_new_from_argb32(ximage->width, ximage->height, data);
        }
        xcb_image_destroy(ximage);
    }

    return retval;
}

static int
luaA_client_get_type(lua_State *L, client_t *c)
{
    switch(c->type)
    {
      case WINDOW_TYPE_DESKTOP:
        lua_pushliteral(L, "desktop");
        break;
      case WINDOW_TYPE_DOCK:
        lua_pushliteral(L, "dock");
        break;
      case WINDOW_TYPE_SPLASH:
        lua_pushliteral(L, "splash");
        break;
      case WINDOW_TYPE_DIALOG:
        lua_pushliteral(L, "dialog");
        break;
      case WINDOW_TYPE_MENU:
        lua_pushliteral(L, "menu");
        break;
      case WINDOW_TYPE_TOOLBAR:
        lua_pushliteral(L, "toolbar");
        break;
      case WINDOW_TYPE_UTILITY:
        lua_pushliteral(L, "utility");
        break;
      case WINDOW_TYPE_DROPDOWN_MENU:
        lua_pushliteral(L, "dropdown_menu");
        break;
      case WINDOW_TYPE_POPUP_MENU:
        lua_pushliteral(L, "popup_menu");
        break;
      case WINDOW_TYPE_TOOLTIP:
        lua_pushliteral(L, "tooltip");
        break;
      case WINDOW_TYPE_NOTIFICATION:
        lua_pushliteral(L, "notification");
        break;
      case WINDOW_TYPE_COMBO:
        lua_pushliteral(L, "combo");
        break;
      case WINDOW_TYPE_DND:
        lua_pushliteral(L, "dnd");
        break;
      case WINDOW_TYPE_NORMAL:
        lua_pushliteral(L, "normal");
        break;
    }
    return 1;
}

static int
luaA_client_get_screen(lua_State *L, client_t *c)
{
    if(!c->screen)
        return 0;
    lua_pushnumber(L, 1 + screen_array_indexof(&globalconf.screens, c->screen));
    return 1;
}

static int
luaA_client_get_icon(lua_State *L, client_t *c)
{
    return luaA_object_push_item(L, -2, c->icon);
}

static int
luaA_client_get_titlebar(lua_State *L, client_t *c)
{
    return luaA_object_push(L, c->titlebar);
}

static int
luaA_client_get_size_hints(lua_State *L, client_t *c)
{
    const char *u_or_p = NULL;

    lua_createtable(L, 0, 1);

    if(c->size_hints.flags & XCB_SIZE_HINT_US_POSITION)
        u_or_p = "user_position";
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_POSITION)
        u_or_p = "program_position";

    if(u_or_p)
    {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, c->size_hints.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, c->size_hints.y);
        lua_setfield(L, -2, "y");
        lua_setfield(L, -2, u_or_p);
        u_or_p = NULL;
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_US_SIZE)
        u_or_p = "user_size";
    else if(c->size_hints.flags & XCB_SIZE_HINT_P_SIZE)
        u_or_p = "program_size";

    if(u_or_p)
    {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, c->size_hints.width);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, c->size_hints.height);
        lua_setfield(L, -2, "height");
        lua_setfield(L, -2, u_or_p);
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_P_MIN_SIZE)
    {
        lua_pushnumber(L, c->size_hints.min_width);
        lua_setfield(L, -2, "min_width");
        lua_pushnumber(L, c->size_hints.min_height);
        lua_setfield(L, -2, "min_height");
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_P_MAX_SIZE)
    {
        lua_pushnumber(L, c->size_hints.max_width);
        lua_setfield(L, -2, "max_width");
        lua_pushnumber(L, c->size_hints.max_height);
        lua_setfield(L, -2, "max_height");
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_P_RESIZE_INC)
    {
        lua_pushnumber(L, c->size_hints.width_inc);
        lua_setfield(L, -2, "width_inc");
        lua_pushnumber(L, c->size_hints.height_inc);
        lua_setfield(L, -2, "height_inc");
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_P_ASPECT)
    {
        lua_pushnumber(L, c->size_hints.min_aspect_num);
        lua_setfield(L, -2, "min_aspect_num");
        lua_pushnumber(L, c->size_hints.min_aspect_den);
        lua_setfield(L, -2, "min_aspect_den");
        lua_pushnumber(L, c->size_hints.max_aspect_num);
        lua_setfield(L, -2, "max_aspect_num");
        lua_pushnumber(L, c->size_hints.max_aspect_den);
        lua_setfield(L, -2, "max_aspect_den");
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_BASE_SIZE)
    {
        lua_pushnumber(L, c->size_hints.base_width);
        lua_setfield(L, -2, "base_width");
        lua_pushnumber(L, c->size_hints.base_height);
        lua_setfield(L, -2, "base_height");
    }

    if(c->size_hints.flags & XCB_SIZE_HINT_P_WIN_GRAVITY)
    {
        switch(c->size_hints.win_gravity)
        {
          default:
            lua_pushliteral(L, "north_west");
            break;
          case XCB_GRAVITY_NORTH:
            lua_pushliteral(L, "north");
            break;
          case XCB_GRAVITY_NORTH_EAST:
            lua_pushliteral(L, "north_east");
            break;
          case XCB_GRAVITY_WEST:
            lua_pushliteral(L, "west");
            break;
          case XCB_GRAVITY_CENTER:
            lua_pushliteral(L, "center");
            break;
          case XCB_GRAVITY_EAST:
            lua_pushliteral(L, "east");
            break;
          case XCB_GRAVITY_SOUTH_WEST:
            lua_pushliteral(L, "south_west");
            break;
          case XCB_GRAVITY_SOUTH:
            lua_pushliteral(L, "south");
            break;
          case XCB_GRAVITY_SOUTH_EAST:
            lua_pushliteral(L, "south_east");
            break;
          case XCB_GRAVITY_STATIC:
            lua_pushliteral(L, "static");
            break;
        }
        lua_setfield(L, -2, "win_gravity");
    }

    return 1;
}

/** Get or set mouse buttons bindings for a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of mouse button bindings objects, or nothing.
 * \return The array of mouse button bindings objects of this client.
 */
static int
luaA_client_buttons(lua_State *L)
{
    client_t *client = luaA_client_checkudata(L, 1);
    button_array_t *buttons = &client->buttons;

    if(lua_gettop(L) == 2)
    {
        luaA_button_array_set(L, 1, 2, buttons);
        luaA_object_emit_signal(L, 1, "property::buttons", 0);
        window_buttons_grab(client->window, &client->buttons);
    }

    return luaA_button_array_get(L, 1, buttons);
}

/** Get or set keys bindings for a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 * \luastack
 * \lvalue A client.
 * \lparam An array of key bindings objects, or nothing.
 * \return The array of key bindings objects of this client.
 */
static int
luaA_client_keys(lua_State *L)
{
    client_t *c = luaA_client_checkudata(L, 1);
    key_array_t *keys = &c->keys;

    if(lua_gettop(L) == 2)
    {
        luaA_key_array_set(L, 1, 2, keys);
        luaA_object_emit_signal(L, 1, "property::keys", 0);
        xcb_ungrab_key(globalconf.connection, XCB_GRAB_ANY, c->window, XCB_BUTTON_MASK_ANY);
        window_grabkeys(c->window, keys);
    }

    return luaA_key_array_get(L, 1, keys);
}

/* Client module.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_index(lua_State *L)
{
    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);

    switch(a_tokenize(buf, len))
    {
      case A_TK_FOCUS:
        return luaA_object_push(globalconf.L, globalconf.screen_focus->client_focus);
        break;
      default:
        return 0;
    }
}

/* Client module new index.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_newindex(lua_State *L)
{
    size_t len;
    const char *buf = luaL_checklstring(L, 2, &len);
    client_t *c;

    switch(a_tokenize(buf, len))
    {
      case A_TK_FOCUS:
        c = luaA_client_checkudata(L, 3);
        client_focus(c);
        break;
      default:
        break;
    }

    return 0;
}

void
client_class_setup(lua_State *L)
{
    static const struct luaL_reg client_methods[] =
    {
        LUA_CLASS_METHODS(client)
        { "get", luaA_client_get },
        { "__index", luaA_client_module_index },
        { "__newindex", luaA_client_module_newindex },
        { NULL, NULL }
    };

    static const struct luaL_reg client_meta[] =
    {
        LUA_OBJECT_META(client)
        LUA_CLASS_META
        { "buttons", luaA_client_buttons },
        { "keys", luaA_client_keys },
        { "isvisible", luaA_client_isvisible },
        { "geometry", luaA_client_geometry },
        { "struts", luaA_client_struts },
        { "tags", luaA_client_tags },
        { "kill", luaA_client_kill },
        { "swap", luaA_client_swap },
        { "raise", luaA_client_raise },
        { "lower", luaA_client_lower },
        { "redraw", luaA_client_redraw },
        { "unmanage", luaA_client_unmanage },
        { "__gc", luaA_client_gc },
        { NULL, NULL }
    };

    luaA_class_setup(L, &client_class, "client", (lua_class_allocator_t) client_new,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     client_methods, client_meta);
    luaA_class_add_property(&client_class, A_TK_NAME,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_name,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_TRANSIENT_FOR,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_transient_for,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_SKIP_TASKBAR,
                            (lua_class_propfunc_t) luaA_client_set_skip_taskbar,
                            (lua_class_propfunc_t) luaA_client_get_skip_taskbar,
                            (lua_class_propfunc_t) luaA_client_set_skip_taskbar);
    luaA_class_add_property(&client_class, A_TK_CONTENT,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_content,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_TYPE,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_type,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_CLASS,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_class,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_INSTANCE,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_instance,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_ROLE,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_role,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_PID,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_pid,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_WINDOW,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_window,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_LEADER_WINDOW,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_leader_window,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_MACHINE,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_machine,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_ICON_NAME,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_icon_name,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_SCREEN,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_screen,
                            (lua_class_propfunc_t) luaA_client_set_screen);
    luaA_class_add_property(&client_class, A_TK_HIDDEN,
                            (lua_class_propfunc_t) luaA_client_set_hidden,
                            (lua_class_propfunc_t) luaA_client_get_hidden,
                            (lua_class_propfunc_t) luaA_client_set_hidden);
    luaA_class_add_property(&client_class, A_TK_MINIMIZED,
                            (lua_class_propfunc_t) luaA_client_set_minimized,
                            (lua_class_propfunc_t) luaA_client_get_minimized,
                            (lua_class_propfunc_t) luaA_client_set_minimized);
    luaA_class_add_property(&client_class, A_TK_FULLSCREEN,
                            (lua_class_propfunc_t) luaA_client_set_fullscreen,
                            (lua_class_propfunc_t) luaA_client_get_fullscreen,
                            (lua_class_propfunc_t) luaA_client_set_fullscreen);
    luaA_class_add_property(&client_class, A_TK_MODAL,
                            (lua_class_propfunc_t) luaA_client_set_modal,
                            (lua_class_propfunc_t) luaA_client_get_modal,
                            (lua_class_propfunc_t) luaA_client_set_modal);
    luaA_class_add_property(&client_class, A_TK_GROUP_WINDOW,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_group_window,
                            NULL);
    luaA_class_add_property(&client_class, A_TK_MAXIMIZED_HORIZONTAL,
                            (lua_class_propfunc_t) luaA_client_set_maximized_horizontal,
                            (lua_class_propfunc_t) luaA_client_get_maximized_horizontal,
                            (lua_class_propfunc_t) luaA_client_set_maximized_horizontal);
    luaA_class_add_property(&client_class, A_TK_MAXIMIZED_VERTICAL,
                            (lua_class_propfunc_t) luaA_client_set_maximized_vertical,
                            (lua_class_propfunc_t) luaA_client_get_maximized_vertical,
                            (lua_class_propfunc_t) luaA_client_set_maximized_vertical);
    luaA_class_add_property(&client_class, A_TK_ICON,
                            (lua_class_propfunc_t) luaA_client_set_icon,
                            (lua_class_propfunc_t) luaA_client_get_icon,
                            (lua_class_propfunc_t) luaA_client_set_icon);
    luaA_class_add_property(&client_class, A_TK_OPACITY,
                            (lua_class_propfunc_t) luaA_client_set_opacity,
                            (lua_class_propfunc_t) luaA_client_get_opacity,
                            (lua_class_propfunc_t) luaA_client_set_opacity);
    luaA_class_add_property(&client_class, A_TK_ONTOP,
                            (lua_class_propfunc_t) luaA_client_set_ontop,
                            (lua_class_propfunc_t) luaA_client_get_ontop,
                            (lua_class_propfunc_t) luaA_client_set_ontop);
    luaA_class_add_property(&client_class, A_TK_ABOVE,
                            (lua_class_propfunc_t) luaA_client_set_above,
                            (lua_class_propfunc_t) luaA_client_get_above,
                            (lua_class_propfunc_t) luaA_client_set_above);
    luaA_class_add_property(&client_class, A_TK_BELOW,
                            (lua_class_propfunc_t) luaA_client_set_below,
                            (lua_class_propfunc_t) luaA_client_get_below,
                            (lua_class_propfunc_t) luaA_client_set_below);
    luaA_class_add_property(&client_class, A_TK_STICKY,
                            (lua_class_propfunc_t) luaA_client_set_sticky,
                            (lua_class_propfunc_t) luaA_client_get_sticky,
                            (lua_class_propfunc_t) luaA_client_set_sticky);
    luaA_class_add_property(&client_class, A_TK_SIZE_HINTS_HONOR,
                            (lua_class_propfunc_t) luaA_client_set_size_hints_honor,
                            (lua_class_propfunc_t) luaA_client_get_size_hints_honor,
                            (lua_class_propfunc_t) luaA_client_set_size_hints_honor);
    luaA_class_add_property(&client_class, A_TK_BORDER_WIDTH,
                            (lua_class_propfunc_t) luaA_client_set_border_width,
                            (lua_class_propfunc_t) luaA_client_get_border_width,
                            (lua_class_propfunc_t) luaA_client_set_border_width);
    luaA_class_add_property(&client_class, A_TK_BORDER_COLOR,
                            (lua_class_propfunc_t) luaA_client_set_border_color,
                            (lua_class_propfunc_t) luaA_client_get_border_color,
                            (lua_class_propfunc_t) luaA_client_set_border_color);
    luaA_class_add_property(&client_class, A_TK_TITLEBAR,
                            (lua_class_propfunc_t) luaA_client_set_titlebar,
                            (lua_class_propfunc_t) luaA_client_get_titlebar,
                            (lua_class_propfunc_t) luaA_client_set_titlebar);
    luaA_class_add_property(&client_class, A_TK_URGENT,
                            (lua_class_propfunc_t) luaA_client_set_urgent,
                            (lua_class_propfunc_t) luaA_client_get_urgent,
                            (lua_class_propfunc_t) luaA_client_set_urgent);
    luaA_class_add_property(&client_class, A_TK_SIZE_HINTS,
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_size_hints,
                            NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
