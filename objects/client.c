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

/** A process window.
 *
 * Clients are the name used by Awesome (and X11) to refer to a window.
 *
 * A program can have multiple clients (e.g. for dialogs) or none at all (e.g.
 * command line applications).
 * Clients are usually grouped by classes.
 * A class is the name used by X11 to help the window manager distinguish
 * between windows and write rules for them.  A client's behavior is also
 * defined by its `type` and `size_hints` properties.
 * See the `xprop` command line application to query properties for a client.
 *
 * ![Client geometry](../images/client_geo.svg)
 *
 * The client's `:geometry()` function returns a table with *x*, *y*, *width*
 * and *height*.  The area returned **excludes the border width**.
 * All clients also have a `shape_bounding` and `shape_clip` used to "crop" the
 * client's content.
 * Finally, each clients can have titlebars (see `awful.titlebar`).
 *
 * Additionally to the classes described here, one can also use signals as
 * described in @{signals} and X properties as described in @{xproperties}.
 *
 * Some signal names are starting with a dot. These dots are artefacts from
 * the documentation generation, you get the real signal name by
 * removing the starting dot.
 *
 * Accessing client objects can be done in multiple ways depending on the
 * context.
 * To get the currently focused client:
 *
 *    local c = client.focus
 *    if c then
 *        -- do something
 *    end
 *
 * To get a list of all clients, use `client:get`:
 *
 *    for _, c in ipairs(client.get()) do
 *        -- do something
 *    end
 *
 * To execute a callback when a new client is added, use the `manage` signal:
 *
 *    client.connect_signal("manage", function(c)
 *        -- do something
 *    end)
 *
 * To be notified when a property of a client changed:
 *
 *    client.connect_signal("property::name", function(c)
 *        -- do something
 *    end)
 *
 * To be notified when a property of a specific client `c` changed:
 *
 *    c:connect_signal("property::name", function()
 *        -- do something
 *    end)
 *
 * To get all the clients for a screen use either `screen.clients` or
 * `screen.tiled_clients`.
 *
 * @author Julien Danjou &lt;julien@danjou.info&gt;
 * @copyright 2008-2009 Julien Danjou
 * @classmod client
 */

#include "objects/client.h"
#include "common/atoms.h"
#include "common/xutil.h"
#include "event.h"
#include "ewmh.h"
#include "objects/drawable.h"
#include "objects/screen.h"
#include "objects/tag.h"
#include "property.h"
#include "spawn.h"
#include "systray.h"
#include "xwindow.h"

#include "math.h"

#include <xcb/xcb_atom.h>
#include <xcb/shape.h>
#include <cairo-xcb.h>

/** Client class.
 *
 * @table object
 */

/** When a client gains focus.
 * @signal .focus
 */

/** Before manage, after unmanage, and when clients swap.
 * @signal .list
 */

/** When 2 clients are swapped
 * @tparam client client The other client
 * @tparam boolean is_source If self is the source or the destination of the swap
 * @signal .swapped
 */

/**
 * @signal .manage
 */

/**
 * @signal button::press
 */

/**
 * @signal button::release
 */

/**
 * @signal mouse::enter
 */

/**
 * @signal mouse::leave
 */

/**
 * @signal mouse::move
 */

/**
 * @signal property::window
 */

/** When a client should get activated (focused and/or raised).
 *
 * **Contexts are:**
 *
 * * *ewmh*: When a client asks for focus (from `X11` events).
 * * *autofocus.check_focus*: When autofocus is enabled (from
 *   `awful.autofocus`).
 * * *autofocus.check_focus_tag*: When autofocus is enabled
 *   (from `awful.autofocus`).
 * * *client.jumpto*: When a custom lua extension asks a client to be focused
 *   (from `client.jump_to`).
 * * *client.swap.global_bydirection*: When client swapping requires a focus
 *   change (from `awful.client.swap.bydirection`).
 * * *client.movetotag*: When a client is moved to a new tag
 *   (from `client.move_to_tag`).
 * * *client.movetoscreen*: When the client is moved to a new screen
 *   (from `client.move_to_screen`).
 * * *client.focus.byidx*: When selecting a client using its index
 *   (from `awful.client.focus.byidx`).
 * * *client.focus.history.previous*: When cycling through history
 *   (from `awful.client.focus.history.previous`).
 * * *menu.clients*: When using the builtin client menu
 *   (from `awful.menu.clients`).
 * * *rules*: When a new client is focused from a rule (from `awful.rules`).
 * * *screen.focus*: When a screen is focused (from `awful.screen.focus`).
 *
 * Default implementation: `awful.ewmh.activate`.
 *
 * To implement focus stealing filters see `awful.ewmh.add_activate_filter`.
 *
 * @signal request::activate
 * @tparam string context The context where this signal was used.
 * @tparam[opt] table hints A table with additional hints:
 * @tparam[opt=false] boolean hints.raise should the client be raised?
 */

/**
 * @signal request::geometry
 * @tparam client c The client
 * @tparam string context Why and what to resize. This is used for the
 *   handlers to know if they are capable of applying the new geometry.
 * @tparam[opt={}] table Additional arguments. Each context handler may
 *   interpret this differently.
 */

/**
 * @signal request::tag
 */

/**
 * @signal request::urgent
 */

/** When a client gets tagged.
 * @signal .tagged
 * @tag t The tag object.
 */

/** When a client gets unfocused.
 * @signal .unfocus
 */

/**
 * @signal .unmanage
 */

/** When a client gets untagged.
 * @signal .untagged
 * @tag t The tag object.
 */

/**
 * @signal .raised
 */

/**
 * @signal .lowered
 */

/**
 * The focused `client` or nil (in case there is none).
 *
 * @tfield client focus
 */

/**
 * The X window id.
 *
 * **Signal:**
 *
 *  * *property::window*
 *
 * @property window
 * @param string
 */

/**
 * The client title.
 *
 * **Signal:**
 *
 *  * *property::name*
 *
 * @property name
 * @param string
 */

/**
 * True if the client does not want to be in taskbar.
 *
 * **Signal:**
 *
 *  * *property::skip\_taskbar*
 *
 * @property skip_taskbar
 * @param boolean
 */

/**
 * The window type.
 *
 * Valid types are:
 *
 * * **desktop**: The root client, it cannot be moved or resized.
 * * **dock**: A client attached to the side of the screen.
 * * **splash**: A client, usually without titlebar shown when an application starts.
 * * **dialog**: A dialog, see `transient_for`.
 * * **menu**: A context menu.
 * * **toolbar**: A floating toolbar.
 * * **utility**:
 * * **dropdown_menu**: A context menu attached to a parent position.
 * * **popup_menu**: A context menu.
 * * **notification**: A notification popup.
 * * **combo**: A combobox list menu.
 * * **dnd**: A drag and drop indicator.
 * * **normal**: A normal application main window.
 *
 * More information can be found [here](https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm140200472629520)
 *
 * **Signal:**
 *
 *  * *property::type*
 *
 * @property type
 * @param string
 */

/**
 * The client class.
 *
 * If the client has multiple classes, the first one is used.
 *
 * To get a client class from the command line, use te `xprop` command.
 *
 * **Signal:**
 *
 *  * *property::class*
 *
 * @property class
 * @param string
 */

/**
 * The client instance.
 *
 * **Signal:**
 *
 *  * *property::instance*
 *
 * @property instance
 * @param string
 */

/**
 * The client PID, if available.
 *
 * **Signal:**
 *
 *  * *property::pid*
 *
 * @property pid
 * @param number
 */

/**
 * The window role, if available.
 *
 * **Signal:**
 *
 *  * *property::role*
 *
 * @property role
 * @param string
 */

/**
 * The machine client is running on.
 *
 * **Signal:**
 *
 *  * *property::machine*
 *
 * @property machine
 * @param string
 */

/**
 * The client name when iconified.
 *
 * **Signal:**
 *
 *  * *property::icon\_name*
 *
 * @property icon_name
 * @param string
 */

/**
 * The client icon.
 *
 * **Signal:**
 *
 *  * *property::icon*
 *
 * @property icon
 * @param surface
 */

/**
 * The available sizes of client icons. This is a table where each entry
 * contains the width and height of an icon.
 *
 * **Signal:**
 *
 *  * *property::icon\_sizes*
 *
 * @property icon_sizes
 * @tparam table sizes
 * @see get_icon
 */

/**
 * Client screen.
 *
 * **Signal:**
 *
 *  * *property::screen*
 *
 * @property screen
 * @param screen
 */

/**
 * Define if the client must be hidden, i.e. never mapped,
 *   invisible in taskbar.
 *
 * **Signal:**
 *
 *  * *property::hidden*
 *
 * @property hidden
 * @param boolean
 */

/**
 * Define it the client must be iconify, i.e. only visible in
 *   taskbar.
 *
 * **Signal:**
 *
 *  * *property::minimized*
 *
 * @property minimized
 * @param boolean
 */

/**
 * Honor size hints, e.g. respect size ratio.
 *
 * For example, a terminal such as `xterm` require the client size to be a
 * multiple of the character size. Honoring size hints will cause the terminal
 * window to have a small gap at the bottom.
 *
 * This is enabled by default. To disable it by default, see `awful.rules`.
 *
 * **Signal:**
 *
 *  * *property::size\_hints\_honor*
 *
 * @property size_hints_honor
 * @param boolean
 * @see size_hints
 */

/**
 * The client border width.
 * @property border_width
 * @param integer
 */

/**
 * The client border color.
 *
 * **Signal:**
 *
 *  * *property::border\_color*
 *
 * @see gears.color
 *
 * @property border_color
 * @param pattern Any string, gradients and patterns will be converted to a
 *  cairo pattern.
 */

/**
 * The client urgent state.
 *
 * **Signal:**
 *
 *  * *property::urgent*
 *
 * @property urgent
 * @param boolean
 */

/**
 * A cairo surface for the client window content.
 *
 * To get the screenshot, use:
 *
 *    gears.surface(c.content)
 *
 * To save it, use:
 *
 *    gears.surface(c.content):write_to_png(path)
 *
 * @property content
 * @param surface
 */

/**
 * The client opacity.
 *
 * **Signal:**
 *
 *  * *property::opacity*
 *
 * @property opacity
 * @param number Between 0 (transparent) to 1 (opaque)
 */

/**
 * The client is on top of every other windows.
 * @property ontop
 * @param boolean
 */

/**
 * The client is above normal windows.
 *
 * **Signal:**
 *
 *  * *property::above*
 *
 * @property above
 * @param boolean
 */

/**
 * The client is below normal windows.
 *
 * **Signal:**
 *
 *  * *property::below*
 *
 * @property below
 * @param boolean
 */

/**
 * The client is fullscreen or not.
 *
 * **Signal:**
 *
 *  * *property::fullscreen*
 *
 * @property fullscreen
 * @param boolean
 */

/**
 * The client is maximized (horizontally and vertically) or not.
 *
 * **Signal:**
 *
 *  * *property::maximized*
 *
 * @property maximized
 * @param boolean
 */

/**
 * The client is maximized horizontally or not.
 *
 * **Signal:**
 *
 *  * *property::maximized\_horizontal*
 *
 * @property maximized_horizontal
 * @param boolean
 */

/**
 * The client is maximized vertically or not.
 *
 * **Signal:**
 *
 *  * *property::maximized\_vertical*
 *
 * @property maximized_vertical
 * @param boolean
 */

/**
 * The client the window is transient for.
 *
 * **Signal:**
 *
 *  * *property::transient\_for*
 *
 * @property transient_for
 * @param client
 */

/**
 * Window identification unique to a group of windows.
 *
 * **Signal:**
 *
 *  * *property::group\_window*
 *
 * @property group_window
 * @param client
 */

/**
 * Identification unique to windows spawned by the same command.
 * @property leader_window
 * @param client
 */

/**
 * A table with size hints of the client.
 *
 * **Signal:**
 *
 *  * *property::size\_hints*
 *
 * @property size_hints
 * @param table
 * @tfield integer table.user_position
 * @tfield integer table.user_size
 * @tfield integer table.program_position
 * @tfield integer table.program_size
 * @tfield integer table.max_width
 * @tfield integer table.max_height
 * @tfield integer table.min_width
 * @tfield integer table.min_height
 * @tfield integer table.width_inc
 * @tfield integer table.height_inc
 * @see size_hints_honor
 */

/**
 * Set the client sticky, i.e. available on all tags.
 *
 * **Signal:**
 *
 *  * *property::sticky*
 *
 * @property sticky
 * @param boolean
 */

/**
 * Indicate if the client is modal.
 *
 * **Signal:**
 *
 *  * *property::modal*
 *
 * @property modal
 * @param boolean
 */

/**
 * True if the client can receive the input focus.
 *
 * **Signal:**
 *
 *  * *property::focusable*
 *
 * @property focusable
 * @param boolean
 */

/**
 * The client's bounding shape as set by awesome as a (native) cairo surface.
 *
 * **Signal:**
 *
 *  * *property::shape\_bounding*
 *
 * @see gears.surface.apply_shape_bounding
 * @property shape_bounding
 * @param surface
 */

/**
 * The client's clip shape as set by awesome as a (native) cairo surface.
 *
 * **Signal:**
 *
 *  * *property::shape\_clip*
 *
 * @property shape_clip
 * @param surface
 */

/**
 * The client's input shape as set by awesome as a (native) cairo surface.
 *
 * **Signal:**
 *
 *  * *property::shape\_input*
 *
 * @property shape_input
 * @param surface
 */

/**
 * The client's bounding shape as set by the program as a (native) cairo surface.
 *
 * **Signal:**
 *
 *  * *property::shape\_client\_bounding*
 *
 * @property shape_client_bounding
 * @param surface
 */

/**
 * The client's clip shape as set by the program as a (native) cairo surface.
 *
 * **Signal:**
 *
 *  * *property::shape\_client\_clip*
 *
 * @property shape_client_clip
 * @param surface
 */

/**
 * The FreeDesktop StartId.
 *
 * When a client is spawned (like using a terminal or `awful.spawn`, a startup
 * notification identifier is created. When the client is created, this
 * identifier remain the same. This allow to match a spawn event to an actual
 * client.
 *
 * **Signal:**
 *
 *  * *property::startup\_id*
 *
 * @property startup_id
 * @param string
 */

/**
 * If the client that this object refers to is still managed by awesome.
 *
 * To avoid errors, use:
 *
 *    local is_valid = pcall(function() return c.valid end) and c.valid
 *
 * **Signal:**
 *
 *  * *property::valid*
 *
 * @property valid
 * @param boolean
 */

/**
 * The first tag of the client.  Optimized form of `c:tags()[1]`.
 *
 * **Signal:**
 *
 *  * *property::first\_tag*
 *
 * @property first_tag
 * @param tag
 */

/** When the height or width changed.
 * @signal property::size
 * @see client.geometry
 */

/** When the x or y coordinate changed.
 * @signal property::position
 * @see client.geometry
 */

/**
 * The border color when the client is focused.
 *
 * @beautiful beautiful.border_focus
 * @param string
 */

/**
 * The border color when the client is not focused.
 *
 * @beautiful beautiful.border_normal
 * @param string
 */

/**
 * The client border width.
 *
 * @beautiful beautiful.border_width
 * @param integer
 */

/** Return client struts (reserved space at the edge of the screen).
 *
 * @param struts A table with new strut values, or none.
 * @return A table with strut values.
 * @function struts
 */

/** Get or set mouse buttons bindings for a client.
 *
 * @param buttons_table An array of mouse button bindings objects, or nothing.
 * @return A table with all buttons.
 * @function buttons
 */

/** Get the number of instances.
 *
 * @return The number of client objects alive.
 * @function instances
 */

/* Set a __index metamethod for all client instances.
 * @tparam function cb The meta-method
 * @function set_index_miss_handler
 */

/* Set a __newindex metamethod for all client instances.
 * @tparam function cb The meta-method
 * @function set_newindex_miss_handler
 */

typedef enum {
    CLIENT_MAXIMIZED_NONE = 0 << 0,
    CLIENT_MAXIMIZED_V    = 1 << 0,
    CLIENT_MAXIMIZED_H    = 1 << 1,
    CLIENT_MAXIMIZED_BOTH = 1 << 2, /* V|H == BOTH, but ~(V|H) != ~(BOTH)... */
} client_maximized_t;

static area_t titlebar_get_area(client_t *c, client_titlebar_t bar);
static drawable_t *titlebar_get_drawable(lua_State *L, client_t *c, int cl_idx, client_titlebar_t bar);
static void client_resize_do(client_t *c, area_t geometry);
static void client_set_maximized_common(lua_State *L, int cidx, bool s, const char* type, const int val);

/** Collect a client.
 * \param L The Lua VM state.
 * \return The number of element pushed on stack.
 */
static void
client_wipe(client_t *c)
{
    key_array_wipe(&c->keys);
    xcb_icccm_get_wm_protocols_reply_wipe(&c->protocols);
    cairo_surface_array_wipe(&c->icons);
    p_delete(&c->machine);
    p_delete(&c->class);
    p_delete(&c->instance);
    p_delete(&c->icon_name);
    p_delete(&c->alt_icon_name);
    p_delete(&c->name);
    p_delete(&c->alt_name);
    p_delete(&c->startup_id);
}

/** Change the clients urgency flag.
 * \param L The Lua VM state.
 * \param cidx The client index on the stack.
 * \param urgent The new flag state.
 */
void
client_set_urgent(lua_State *L, int cidx, bool urgent)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->urgent != urgent)
    {
        c->urgent = urgent;

        luaA_object_emit_signal(L, cidx, "property::urgent", 0);
    }
}

#define DO_CLIENT_SET_PROPERTY(prop) \
    void \
    client_set_##prop(lua_State *L, int cidx, fieldtypeof(client_t, prop) value) \
    { \
        client_t *c = luaA_checkudata(L, cidx, &client_class); \
        if(c->prop != value) \
        { \
            c->prop = value; \
            luaA_object_emit_signal(L, cidx, "property::" #prop, 0); \
        } \
    }
DO_CLIENT_SET_PROPERTY(group_window)
DO_CLIENT_SET_PROPERTY(type)
DO_CLIENT_SET_PROPERTY(transient_for)
DO_CLIENT_SET_PROPERTY(pid)
DO_CLIENT_SET_PROPERTY(skip_taskbar)
#undef DO_CLIENT_SET_PROPERTY

#define DO_CLIENT_SET_STRING_PROPERTY2(prop, signal) \
    void \
    client_set_##prop(lua_State *L, int cidx, char *value) \
    { \
        client_t *c = luaA_checkudata(L, cidx, &client_class); \
        if (A_STREQ(c->prop, value)) \
        { \
            p_delete(&value); \
            return; \
        } \
        p_delete(&c->prop); \
        c->prop = value; \
        luaA_object_emit_signal(L, cidx, "property::" #signal, 0); \
    }
#define DO_CLIENT_SET_STRING_PROPERTY(prop) \
        DO_CLIENT_SET_STRING_PROPERTY2(prop, prop)
DO_CLIENT_SET_STRING_PROPERTY(name)
DO_CLIENT_SET_STRING_PROPERTY2(alt_name, name)
DO_CLIENT_SET_STRING_PROPERTY(icon_name)
DO_CLIENT_SET_STRING_PROPERTY2(alt_icon_name, icon_name)
DO_CLIENT_SET_STRING_PROPERTY(role)
DO_CLIENT_SET_STRING_PROPERTY(machine)
#undef DO_CLIENT_SET_STRING_PROPERTY

void
client_find_transient_for(client_t *c)
{
    int counter;
    client_t *tc, *tmp;

    tmp = tc = client_getbywin(c->transient_for_window);

    /* Verify that there are no loops in the transient_for relation after we are done */
    for(counter = 0; tmp != NULL && counter <= globalconf.stack.len; counter++)
    {
        if (tmp == c)
            /* We arrived back at the client we started from, so there is a loop */
            counter = globalconf.stack.len+1;
        tmp = tmp->transient_for;
    }
    if (counter <= globalconf.stack.len)
    {
        lua_State *L = globalconf_get_lua_State();

        luaA_object_push(L, c);
        client_set_transient_for(L, -1, tc);
        lua_pop(L, 1);
    }
}

void
client_set_class_instance(lua_State *L, int cidx, const char *class, const char *instance)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);
    p_delete(&c->class);
    p_delete(&c->instance);
    c->class = a_strdup(class);
    luaA_object_emit_signal(L, cidx, "property::class", 0);
    c->instance = a_strdup(instance);
    luaA_object_emit_signal(L, cidx, "property::instance", 0);
}

/** Returns true if a client is tagged with one of the active tags.
 * \param c The client to check.
 * \return true if the client is visible, false otherwise.
 */
bool
client_on_selected_tags(client_t *c)
{
    if(c->sticky)
        return true;

    foreach(tag, globalconf.tags)
        if(tag_get_selected(*tag) && is_client_tagged(c, *tag))
            return true;

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

client_t *
client_getbynofocuswin(xcb_window_t w)
{
    foreach(c, globalconf.clients)
        if((*c)->nofocus_window == w)
            return *c;

    return NULL;
}

/** Get a client by its frame window.
 * \param w The client window to find.
 * \return A client pointer if found, NULL otherwise.
 */
client_t *
client_getbyframewin(xcb_window_t w)
{
    foreach(c, globalconf.clients)
        if((*c)->frame_window == w)
            return *c;

    return NULL;
}

/** Unfocus a client (internal).
 * \param c The client.
 */
static void
client_unfocus_internal(client_t *c)
{
    lua_State *L = globalconf_get_lua_State();
    globalconf.focus.client = NULL;

    luaA_object_push(L, c);
    luaA_object_emit_signal(L, -1, "unfocus", 0);
    lua_pop(L, 1);
}

/** Unfocus a client.
 * \param c The client.
 */
static void
client_unfocus(client_t *c)
{
    client_unfocus_internal(c);
    globalconf.focus.need_update = true;
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

/** Prepare banning a client by running all needed lua events.
 * \param c The client.
 */
void client_ban_unfocus(client_t *c)
{
    /* Wait until the last moment to take away the focus from the window. */
    if(globalconf.focus.client == c)
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
        client_ignore_enterleave_events();
        xcb_unmap_window(globalconf.connection, c->frame_window);
        client_restore_enterleave_events();

        c->isbanned = true;

        client_ban_unfocus(c);
    }
}

/** This is part of The Bob Marley Algorithm: we ignore enter and leave window
 * in certain cases, like map/unmap or move, so we don't get spurious events.
 * The implementation works by noting the range of sequence numbers for which we
 * should ignore events. We grab the server to make sure that only we could
 * generate events in this range.
 */
void
client_ignore_enterleave_events(void)
{
    assert(globalconf.pending_enter_leave_begin.sequence == 0);
    globalconf.pending_enter_leave_begin = xcb_grab_server(globalconf.connection);
    /* If the connection is broken, we get a request with sequence number 0
     * which would then trigger an assertion in
     * client_restore_enterleave_events(). Handle this nicely.
     */
    if(xcb_connection_has_error(globalconf.connection))
        fatal("X server connection broke (error %d)",
                xcb_connection_has_error(globalconf.connection));
    assert(globalconf.pending_enter_leave_begin.sequence != 0);
}

void
client_restore_enterleave_events(void)
{
    sequence_pair_t pair;

    assert(globalconf.pending_enter_leave_begin.sequence != 0);
    pair.begin = globalconf.pending_enter_leave_begin;
    pair.end = xcb_no_operation(globalconf.connection);
    xcb_ungrab_server(globalconf.connection);
    globalconf.pending_enter_leave_begin.sequence = 0;
    sequence_pair_array_append(&globalconf.ignore_enter_leave_events, pair);
}

/** Record that a client got focus.
 * \param c The client.
 * \return true if the client focus changed, false otherwise.
 */
bool
client_focus_update(client_t *c)
{
    lua_State *L = globalconf_get_lua_State();

    if(globalconf.focus.client && globalconf.focus.client != c)
    {
        /* When we are called due to a FocusIn event (=old focused client
         * already unfocused), we don't want to cause a SetInputFocus,
         * because the client which has focus now could be using globally
         * active input model (or 'no input').
         */
        client_unfocus_internal(globalconf.focus.client);
    }

    bool focused_new = globalconf.focus.client != c;
    globalconf.focus.client = c;

    /* According to EWMH, we have to remove the urgent state from a client.
     * This should be done also for the current/focused client (FS#1310). */
    luaA_object_push(L, c);
    client_set_urgent(L, -1, false);

    if(focused_new)
        luaA_object_emit_signal(L, -1, "focus", 0);

    lua_pop(L, 1);

    return focused_new;
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

    if(client_focus_update(c))
        globalconf.focus.need_update = true;
}

static xcb_window_t
client_get_nofocus_window(client_t *c)
{
    if (c->nofocus_window == XCB_NONE) {
        c->nofocus_window = xcb_generate_id(globalconf.connection);
        xcb_create_window(globalconf.connection, globalconf.default_depth, c->nofocus_window, c->frame_window,
                          -2, -2, 1, 1, 0, XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
                          0, NULL);
        xcb_map_window(globalconf.connection, c->nofocus_window);
        xwindow_grabkeys(c->nofocus_window, &c->keys);
    }
    return c->nofocus_window;
}

void
client_focus_refresh(void)
{
    client_t *c = globalconf.focus.client;
    xcb_window_t win = globalconf.focus.window_no_focus;

    if(!globalconf.focus.need_update)
        return;
    globalconf.focus.need_update = false;

    if(c && client_on_selected_tags(c))
    {
        /* Make sure this window is unbanned and e.g. not minimized */
        client_unban(c);
        /* Sets focus on window - using xcb_set_input_focus or WM_TAKE_FOCUS */
        if(!c->nofocus)
            win = c->window;
        else
            win = client_get_nofocus_window(c);

        if(client_hasproto(c, WM_TAKE_FOCUS))
            xwindow_takefocus(c->window);
    }

    /* If nothing has the focus or the currently focused client does not want
     * us to focus it, this sets the focus to the root window. This makes sure
     * the previously focused client actually gets unfocused. Alternatively, the
     * new client gets the input focus.
     */
    xcb_set_input_focus(globalconf.connection, XCB_INPUT_FOCUS_PARENT,
                        win, globalconf.timestamp);
}

static void
client_border_refresh(void)
{
    foreach(c, globalconf.clients)
        window_border_refresh((window_t *) *c);
}

static void
client_geometry_refresh(void)
{
    bool ignored_enterleave = false;
    foreach(_c, globalconf.clients)
    {
        client_t *c = *_c;

        /* Compute the client window's and frame window's geometry */
        area_t geometry = c->geometry;
        area_t real_geometry = c->geometry;
        if (!c->fullscreen)
        {
            if ((real_geometry.width < c->titlebar[CLIENT_TITLEBAR_LEFT].size
                    + c->titlebar[CLIENT_TITLEBAR_RIGHT].size) ||
                    (real_geometry.height < c->titlebar[CLIENT_TITLEBAR_TOP].size
                    + c->titlebar[CLIENT_TITLEBAR_BOTTOM].size))
                warn("Resizing a window to a negative size!? Have width %d-%d-%d=%d"
                        " and height %d-%d-%d=%d", real_geometry.width,
                        c->titlebar[CLIENT_TITLEBAR_LEFT].size,
                        c->titlebar[CLIENT_TITLEBAR_RIGHT].size,
                        real_geometry.width -
                            c->titlebar[CLIENT_TITLEBAR_LEFT].size -
                            c->titlebar[CLIENT_TITLEBAR_RIGHT].size,
                        real_geometry.height,
                        c->titlebar[CLIENT_TITLEBAR_TOP].size,
                        c->titlebar[CLIENT_TITLEBAR_BOTTOM].size,
                        real_geometry.height -
                            c->titlebar[CLIENT_TITLEBAR_TOP].size -
                            c->titlebar[CLIENT_TITLEBAR_BOTTOM].size);

            real_geometry.x = c->titlebar[CLIENT_TITLEBAR_LEFT].size;
            real_geometry.y = c->titlebar[CLIENT_TITLEBAR_TOP].size;
            real_geometry.width -= c->titlebar[CLIENT_TITLEBAR_LEFT].size;
            real_geometry.width -= c->titlebar[CLIENT_TITLEBAR_RIGHT].size;
            real_geometry.height -= c->titlebar[CLIENT_TITLEBAR_TOP].size;
            real_geometry.height -= c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;

            if (real_geometry.width == 0 || real_geometry.height == 0)
                warn("Resizing a window to size zero!?");
        } else {
            real_geometry.x = 0;
            real_geometry.y = 0;
        }

        /* Is there anything to do? */
        if (AREA_EQUAL(geometry, c->x11_frame_geometry)
                && AREA_EQUAL(real_geometry, c->x11_client_geometry)) {
            if (c->got_configure_request) {
                /* ICCCM 4.1.5 / 4.2.3, if nothing was changed, send an event saying so */
                client_send_configure(c);
                c->got_configure_request = false;
            }
            continue;
        }

        if (!ignored_enterleave) {
            client_ignore_enterleave_events();
            ignored_enterleave = true;
        }

        xcb_configure_window(globalconf.connection, c->frame_window,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (uint32_t[]) { geometry.x, geometry.y, geometry.width, geometry.height });
        xcb_configure_window(globalconf.connection, c->window,
                XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                (uint32_t[]) { real_geometry.x, real_geometry.y, real_geometry.width, real_geometry.height });

        c->x11_frame_geometry = geometry;
        c->x11_client_geometry = real_geometry;

        /* ICCCM 4.2.3 says something else, but Java always needs this... */
        client_send_configure(c);
        c->got_configure_request = false;
    }
    if (ignored_enterleave)
        client_restore_enterleave_events();
}

void
client_refresh(void)
{
    client_geometry_refresh();
    client_border_refresh();
    client_focus_refresh();
}

void
client_destroy_later(void)
{
    bool ignored_enterleave = false;
    foreach(window, globalconf.destroy_later_windows)
    {
        if (!ignored_enterleave) {
            client_ignore_enterleave_events();
            ignored_enterleave = true;
        }
        xcb_destroy_window(globalconf.connection, *window);
    }
    if (ignored_enterleave)
        client_restore_enterleave_events();

    /* Everything's done, clear the list */
    globalconf.destroy_later_windows.len = 0;
}

static void
border_width_callback(client_t *c, uint16_t old_width, uint16_t new_width)
{
    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY)
    {
        area_t geometry = c->geometry;
        int16_t diff = new_width - old_width;
        int16_t diff_x = 0, diff_y = 0;
        xwindow_translate_for_gravity(c->size_hints.win_gravity,
                                      diff, diff, diff, diff,
                                      &diff_x, &diff_y);
        geometry.x += diff_x;
        geometry.y += diff_y;
        /* inform client about changes */
        client_resize_do(c, geometry);
    }
}

static void
client_update_properties(lua_State *L, int cidx, client_t *c)
{
    /* get all hints */
    xcb_get_property_cookie_t wm_normal_hints   = property_get_wm_normal_hints(c);
    xcb_get_property_cookie_t wm_hints          = property_get_wm_hints(c);
    xcb_get_property_cookie_t wm_transient_for  = property_get_wm_transient_for(c);
    xcb_get_property_cookie_t wm_client_leader  = property_get_wm_client_leader(c);
    xcb_get_property_cookie_t wm_client_machine = property_get_wm_client_machine(c);
    xcb_get_property_cookie_t wm_window_role    = property_get_wm_window_role(c);
    xcb_get_property_cookie_t net_wm_pid        = property_get_net_wm_pid(c);
    xcb_get_property_cookie_t net_wm_icon       = property_get_net_wm_icon(c);
    xcb_get_property_cookie_t wm_name           = property_get_wm_name(c);
    xcb_get_property_cookie_t net_wm_name       = property_get_net_wm_name(c);
    xcb_get_property_cookie_t wm_icon_name      = property_get_wm_icon_name(c);
    xcb_get_property_cookie_t net_wm_icon_name  = property_get_net_wm_icon_name(c);
    xcb_get_property_cookie_t wm_class          = property_get_wm_class(c);
    xcb_get_property_cookie_t wm_protocols      = property_get_wm_protocols(c);
    xcb_get_property_cookie_t opacity           = xwindow_get_opacity_unchecked(c->window);

    /* update strut */
    ewmh_process_client_strut(c);

    /* Now process all replies */
    property_update_wm_normal_hints(c, wm_normal_hints);
    property_update_wm_hints(c, wm_hints);
    property_update_wm_transient_for(c, wm_transient_for);
    property_update_wm_client_leader(c, wm_client_leader);
    property_update_wm_client_machine(c, wm_client_machine);
    property_update_wm_window_role(c, wm_window_role);
    property_update_net_wm_pid(c, net_wm_pid);
    property_update_net_wm_icon(c, net_wm_icon);
    property_update_wm_name(c, wm_name);
    property_update_net_wm_name(c, net_wm_name);
    property_update_wm_icon_name(c, wm_icon_name);
    property_update_net_wm_icon_name(c, net_wm_icon_name);
    property_update_wm_class(c, wm_class);
    property_update_wm_protocols(c, wm_protocols);
    window_set_opacity(L, cidx, xwindow_get_opacity_from_cookie(opacity));
}

/** Manage a new client.
 * \param w The window.
 * \param wgeom Window geometry.
 * \param startup True if we are managing at startup time.
 */
void
client_manage(xcb_window_t w, xcb_get_geometry_reply_t *wgeom, xcb_get_window_attributes_reply_t *wattr)
{
    lua_State *L = globalconf_get_lua_State();
    const uint32_t select_input_val[] = { CLIENT_SELECT_INPUT_EVENT_MASK };

    if(systray_iskdedockapp(w))
    {
        systray_request_handle(w);
        return;
    }

    /* If this is a new client that just has been launched, then request its
     * startup id. */
    xcb_get_property_cookie_t startup_id_q = xcb_get_property(globalconf.connection, false,
                                                              w, _NET_STARTUP_ID,
                                                              XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);

    /* Make sure the window is automatically mapped if awesome exits or dies. */
    xcb_change_save_set(globalconf.connection, XCB_SET_MODE_INSERT, w);
    if (globalconf.have_shape)
        xcb_shape_select_input(globalconf.connection, w, 1);

    client_t *c = client_new(L);
    xcb_screen_t *s = globalconf.screen;
    c->border_width_callback = (void (*) (void *, uint16_t, uint16_t)) border_width_callback;

    /* consider the window banned */
    c->isbanned = true;
    /* Store window and visual */
    c->window = w;
    c->visualtype = draw_find_visual(globalconf.screen, wattr->visual);
    c->frame_window = xcb_generate_id(globalconf.connection);
    xcb_create_window(globalconf.connection, globalconf.default_depth, c->frame_window, s->root,
                      wgeom->x, wgeom->y, wgeom->width, wgeom->height,
                      wgeom->border_width, XCB_COPY_FROM_PARENT, globalconf.visual->visual_id,
                      XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY | XCB_CW_WIN_GRAVITY
                      | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
                      (const uint32_t [])
                      {
                          globalconf.screen->black_pixel,
                          XCB_GRAVITY_NORTH_WEST,
                          XCB_GRAVITY_NORTH_WEST,
                          1,
                          FRAME_SELECT_INPUT_EVENT_MASK,
                          globalconf.default_cmap
                      });

    /* The client may already be mapped, thus we must be sure that we don't send
     * ourselves an UnmapNotify due to the xcb_reparent_window().
     *
     * Grab the server to make sure we don't lose any events.
     */
    uint32_t no_event[] = { 0 };
    xcb_grab_server(globalconf.connection);

    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 no_event);
    xcb_reparent_window(globalconf.connection, w, c->frame_window, 0, 0);
    xcb_map_window(globalconf.connection, w);
    xcb_change_window_attributes(globalconf.connection,
                                 globalconf.screen->root,
                                 XCB_CW_EVENT_MASK,
                                 ROOT_WINDOW_EVENT_MASK);
    xcb_ungrab_server(globalconf.connection);

    /* Do this now so that we don't get any events for the above
     * (Else, reparent could cause an UnmapNotify) */
    xcb_change_window_attributes(globalconf.connection, w, XCB_CW_EVENT_MASK, select_input_val);

    /* The frame window gets the border, not the real client window */
    xcb_configure_window(globalconf.connection, w,
                         XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         (uint32_t[]) { 0 });

    /* Move this window to the bottom of the stack. Without this we would force
     * other windows which will be above this one to redraw themselves because
     * this window occludes them for a tiny moment. The next stack_refresh()
     * will fix this up and move the window to its correct place. */
    xcb_configure_window(globalconf.connection, c->frame_window,
                         XCB_CONFIG_WINDOW_STACK_MODE,
                         (uint32_t[]) { XCB_STACK_MODE_BELOW});

    /* Duplicate client and push it in client list */
    lua_pushvalue(L, -1);
    client_array_push(&globalconf.clients, luaA_object_ref(L, -1));

    /* Set the right screen */
    screen_client_moveto(c, screen_getbycoord(wgeom->x, wgeom->y), false);

    /* Store initial geometry and emits signals so we inform that geometry have
     * been set. */

    c->geometry.x = wgeom->x;
    c->geometry.y = wgeom->y;
    c->geometry.width = wgeom->width;
    c->geometry.height = wgeom->height;

    luaA_object_emit_signal(L, -1, "property::x", 0);
    luaA_object_emit_signal(L, -1, "property::y", 0);
    luaA_object_emit_signal(L, -1, "property::width", 0);
    luaA_object_emit_signal(L, -1, "property::height", 0);
    luaA_object_emit_signal(L, -1, "property::window", 0);
    luaA_object_emit_signal(L, -1, "property::geometry", 0);

    /* Set border width */
    window_set_border_width(L, -1, wgeom->border_width);

    /* we honor size hints by default */
    c->size_hints_honor = true;
    luaA_object_emit_signal(L, -1, "property::size_hints_honor", 0);

    /* update all properties */
    client_update_properties(L, -1, c);

    /* check if this is a TRANSIENT_FOR of another client */
    foreach(oc, globalconf.clients)
        if ((*oc)->transient_for_window == w)
            client_find_transient_for(*oc);

    /* Then check clients hints */
    ewmh_client_check_hints(c);

    /* Push client in stack */
    stack_client_push(c);

    /* Put the window in normal state. */
    xwindow_set_state(c->window, XCB_ICCCM_WM_STATE_NORMAL);

    /* Request our response */
    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(globalconf.connection, startup_id_q, NULL);
    /* Say spawn that a client has been started, with startup id as argument */
    char *startup_id = xutil_get_text_property_from_reply(reply);
    p_delete(&reply);

    if (startup_id == NULL && c->leader_window != XCB_NONE) {
        /* GTK hides this property elsewhere. No idea why. */
        startup_id_q = xcb_get_property(globalconf.connection, false,
                                        c->leader_window, _NET_STARTUP_ID,
                                        XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
        reply = xcb_get_property_reply(globalconf.connection, startup_id_q, NULL);
        startup_id = xutil_get_text_property_from_reply(reply);
        p_delete(&reply);
    }
    c->startup_id = startup_id;

    spawn_start_notify(c, startup_id);

    luaA_class_emit_signal(L, &client_class, "list", 0);

    /* client is still on top of the stack; emit signal */
    luaA_object_emit_signal(L, -1, "manage", 0);
    /* pop client */
    lua_pop(L, 1);
}

static void
client_remove_titlebar_geometry(client_t *c, area_t *geometry)
{
    geometry->x += c->titlebar[CLIENT_TITLEBAR_LEFT].size;
    geometry->y += c->titlebar[CLIENT_TITLEBAR_TOP].size;
    geometry->width -= c->titlebar[CLIENT_TITLEBAR_LEFT].size;
    geometry->width -= c->titlebar[CLIENT_TITLEBAR_RIGHT].size;
    geometry->height -= c->titlebar[CLIENT_TITLEBAR_TOP].size;
    geometry->height -= c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;
}

static void
client_add_titlebar_geometry(client_t *c, area_t *geometry)
{
    geometry->x -= c->titlebar[CLIENT_TITLEBAR_LEFT].size;
    geometry->y -= c->titlebar[CLIENT_TITLEBAR_TOP].size;
    geometry->width += c->titlebar[CLIENT_TITLEBAR_LEFT].size;
    geometry->width += c->titlebar[CLIENT_TITLEBAR_RIGHT].size;
    geometry->height += c->titlebar[CLIENT_TITLEBAR_TOP].size;
    geometry->height += c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;
}

/** Send a synthetic configure event to a window.
 */
void
client_send_configure(client_t *c)
{
    area_t geometry = c->geometry;

    if (!c->fullscreen)
        client_remove_titlebar_geometry(c, &geometry);
    xwindow_configure(c->window, geometry, c->border_width);
}

/** Apply size hints to the client's new geometry.
 */
static area_t
client_apply_size_hints(client_t *c, area_t geometry)
{
    int32_t minw = 0, minh = 0;
    int32_t basew = 0, baseh = 0, real_basew = 0, real_baseh = 0;

    if (c->fullscreen)
        return geometry;

    /* Size hints are applied to the window without any decoration */
    client_remove_titlebar_geometry(c, &geometry);

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
        basew = c->size_hints.base_width;
        baseh = c->size_hints.base_height;
        real_basew = basew;
        real_baseh = baseh;
    }
    else if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
        /* base size is substituted with min size if not specified */
        basew = c->size_hints.min_width;
        baseh = c->size_hints.min_height;
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
        minw = c->size_hints.min_width;
        minh = c->size_hints.min_height;
    }
    else if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
        /* min size is substituted with base size if not specified */
        minw = c->size_hints.base_width;
        minh = c->size_hints.base_height;
    }

    /* Handle the size aspect ratio */
    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_ASPECT
            && c->size_hints.min_aspect_den > 0
            && c->size_hints.max_aspect_den > 0
            && geometry.height > real_baseh
            && geometry.width > real_basew)
    {
        /* ICCCM mandates:
         * If a base size is provided along with the aspect ratio fields, the base size should be subtracted from the
         * window size prior to checking that the aspect ratio falls in range. If a base size is not provided, nothing
         * should be subtracted from the window size. (The minimum size is not to be used in place of the base size for
         * this purpose.)
         */
         double dx = geometry.width - real_basew;
         double dy = geometry.height - real_baseh;
         double ratio = dx / dy;
         double min = c->size_hints.min_aspect_num / (double) c->size_hints.min_aspect_den;
         double max = c->size_hints.max_aspect_num / (double) c->size_hints.max_aspect_den;

         if(max > 0 && min > 0 && ratio > 0)
         {
             if(ratio < min)
             {
                 /* dx is lower than allowed, make dy lower to compensate this (+ 0.5 to force proper rounding). */
                 dy = dx / min + 0.5;
                 geometry.width  = dx + real_basew;
                 geometry.height = dy + real_baseh;
             } else if(ratio > max)
             {
                 /* dx is too high, lower it (+0.5 for proper rounding) */
                 dx = dy * max + 0.5;
                 geometry.width  = dx + real_basew;
                 geometry.height = dy + real_baseh;
             }
         }
    }

    /* Handle the minimum size */
    geometry.width = MAX(geometry.width, minw);
    geometry.height = MAX(geometry.height, minh);

    /* Handle the maximum size */
    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
    {
        if(c->size_hints.max_width)
            geometry.width = MIN(geometry.width, c->size_hints.max_width);
        if(c->size_hints.max_height)
            geometry.height = MIN(geometry.height, c->size_hints.max_height);
    }

    /* Handle the size increment */
    if(c->size_hints.flags & (XCB_ICCCM_SIZE_HINT_P_RESIZE_INC | XCB_ICCCM_SIZE_HINT_BASE_SIZE)
       && c->size_hints.width_inc && c->size_hints.height_inc)
    {
        uint16_t t1 = geometry.width, t2 = geometry.height;
        unsigned_subtract(t1, basew);
        unsigned_subtract(t2, baseh);
        geometry.width -= t1 % c->size_hints.width_inc;
        geometry.height -= t2 % c->size_hints.height_inc;
    }

    client_add_titlebar_geometry(c, &geometry);
    return geometry;
}

static void
client_resize_do(client_t *c, area_t geometry)
{
    lua_State *L = globalconf_get_lua_State();

    screen_t *new_screen = c->screen;
    if(!screen_area_in_screen(new_screen, geometry))
        new_screen = screen_getbycoord(geometry.x, geometry.y);

    /* Also store geometry including border */
    area_t old_geometry = c->geometry;
    c->geometry = geometry;

    luaA_object_push(L, c);
    if (!AREA_EQUAL(old_geometry, geometry))
        luaA_object_emit_signal(L, -1, "property::geometry", 0);
    if (old_geometry.x != geometry.x || old_geometry.y != geometry.y)
    {
        luaA_object_emit_signal(L, -1, "property::position", 0);
        if (old_geometry.x != geometry.x)
            luaA_object_emit_signal(L, -1, "property::x", 0);
        else
            luaA_object_emit_signal(L, -1, "property::y", 0);
    }
    if (old_geometry.width != geometry.width || old_geometry.height != geometry.height)
    {
        luaA_object_emit_signal(L, -1, "property::size", 0);
        if (old_geometry.width != geometry.width)
            luaA_object_emit_signal(L, -1, "property::width", 0);
        else
            luaA_object_emit_signal(L, -1, "property::height", 0);
    }
    lua_pop(L, 1);

    screen_client_moveto(c, new_screen, false);

    /* Update all titlebars */
    for (client_titlebar_t bar = CLIENT_TITLEBAR_TOP; bar < CLIENT_TITLEBAR_COUNT; bar++) {
        if (c->titlebar[bar].drawable == NULL && c->titlebar[bar].size == 0)
            continue;

        luaA_object_push(L, c);
        drawable_t *drawable = titlebar_get_drawable(L, c, -1, bar);
        luaA_object_push_item(L, -1, drawable);

        area_t area = titlebar_get_area(c, bar);

        /* Convert to global coordinates */
        area.x += geometry.x;
        area.y += geometry.y;
        if (c->fullscreen)
            area.width = area.height = 0;
        drawable_set_geometry(L, -1, area);

        /* Pop the client and the drawable */
        lua_pop(L, 2);
    }
}

/** Resize client window.
 * The sizes given as parameters are with borders!
 * \param c Client to resize.
 * \param geometry New window geometry.
 * \param honor_hints Use size hints.
 * \return true if an actual resize occurred.
 */
bool
client_resize(client_t *c, area_t geometry, bool honor_hints)
{
    area_t area;

    /* offscreen appearance fixes */
    area = display_area_get();

    if(geometry.x > area.width)
        geometry.x = area.width - geometry.width;
    if(geometry.y > area.height)
        geometry.y = area.height - geometry.height;
    if(geometry.x + geometry.width < 0)
        geometry.x = 0;
    if(geometry.y + geometry.height < 0)
        geometry.y = 0;

    if (honor_hints) {
        /* We could get integer underflows in client_remove_titlebar_geometry()
         * without these checks here.
         */
        if(geometry.width < c->titlebar[CLIENT_TITLEBAR_LEFT].size + c->titlebar[CLIENT_TITLEBAR_RIGHT].size)
            return false;
        if(geometry.height < c->titlebar[CLIENT_TITLEBAR_TOP].size + c->titlebar[CLIENT_TITLEBAR_BOTTOM].size)
            return false;
        geometry = client_apply_size_hints(c, geometry);
    }

    if(geometry.width < c->titlebar[CLIENT_TITLEBAR_LEFT].size + c->titlebar[CLIENT_TITLEBAR_RIGHT].size)
        return false;
    if(geometry.height < c->titlebar[CLIENT_TITLEBAR_TOP].size + c->titlebar[CLIENT_TITLEBAR_BOTTOM].size)
        return false;

    if(geometry.width == 0 || geometry.height == 0)
        return false;

    if(!AREA_EQUAL(c->geometry, geometry))
    {
        client_resize_do(c, geometry);

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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->minimized != s)
    {
        c->minimized = s;
        banning_need_update();
        if(s)
        {
            /* ICCCM: To transition from ICONIC to NORMAL state, the client
             * should just map the window. Thus, iconic clients need to be
             * unmapped, else the MapWindow request doesn't have any effect.
             */
            xwindow_set_state(c->window, XCB_ICCCM_WM_STATE_ICONIC);

            uint32_t no_event[] = { 0 };
            const uint32_t client_select_input_val[] = { CLIENT_SELECT_INPUT_EVENT_MASK };
            const uint32_t frame_select_input_val[] = { FRAME_SELECT_INPUT_EVENT_MASK };
            xcb_grab_server(globalconf.connection);
            xcb_change_window_attributes(globalconf.connection,
                                         globalconf.screen->root,
                                         XCB_CW_EVENT_MASK,
                                         no_event);
            xcb_change_window_attributes(globalconf.connection,
                                         c->frame_window,
                                         XCB_CW_EVENT_MASK,
                                         no_event);
            xcb_change_window_attributes(globalconf.connection,
                                         c->window,
                                         XCB_CW_EVENT_MASK,
                                         no_event);
            xcb_unmap_window(globalconf.connection, c->window);
            xcb_change_window_attributes(globalconf.connection,
                                         globalconf.screen->root,
                                         XCB_CW_EVENT_MASK,
                                         ROOT_WINDOW_EVENT_MASK);
            xcb_change_window_attributes(globalconf.connection,
                                         c->frame_window,
                                         XCB_CW_EVENT_MASK,
                                         frame_select_input_val);
            xcb_change_window_attributes(globalconf.connection,
                                         c->window,
                                         XCB_CW_EVENT_MASK,
                                         client_select_input_val);
            xcb_ungrab_server(globalconf.connection);
        }
        else
        {
            xwindow_set_state(c->window, XCB_ICCCM_WM_STATE_NORMAL);
            xcb_map_window(globalconf.connection, c->window);
        }
        if(strut_has_value(&c->strut))
            screen_update_workarea(c->screen);
        luaA_object_emit_signal(L, cidx, "property::minimized", 0);
    }
}

/** Set a client hidden, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client hidden.
 */
static void
client_set_hidden(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->hidden != s)
    {
        c->hidden = s;
        banning_need_update();
        if(strut_has_value(&c->strut))
            screen_update_workarea(c->screen);
        luaA_object_emit_signal(L, cidx, "property::hidden", 0);
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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->sticky != s)
    {
        c->sticky = s;
        banning_need_update();
        if(strut_has_value(&c->strut))
            screen_update_workarea(c->screen);
        luaA_object_emit_signal(L, cidx, "property::sticky", 0);
    }
}

/** Set a client focusable, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client's focusable property.
 */
static void
client_set_focusable(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->focusable != s || !c->focusable_set)
    {
        c->focusable = s;
        c->focusable_set = true;
        luaA_object_emit_signal(L, cidx, "property::focusable", 0);
    }
}

/** Unset a client's focusable property and make it use the default again.
 * \param L The Lua VM state.
 * \param cidx The client index.
 */
static void
client_unset_focusable(lua_State *L, int cidx)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->focusable_set)
    {
        c->focusable_set = false;
        luaA_object_emit_signal(L, cidx, "property::focusable", 0);
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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->fullscreen != s)
    {
        /* become fullscreen! */
        if(s)
        {
            /* You can only be part of one of the special layers. */
            client_set_below(L, cidx, false);
            client_set_above(L, cidx, false);
            client_set_ontop(L, cidx, false);
        }
        int abs_cidx = luaA_absindex(L, cidx); \
        lua_pushstring(L, "fullscreen");
        c->fullscreen = s;
        luaA_object_emit_signal(L, abs_cidx, "request::geometry", 1);
        luaA_object_emit_signal(L, abs_cidx, "property::fullscreen", 0);
        /* Force a client resize, so that titlebars get shown/hidden */
        client_resize_do(c, c->geometry);
        stack_windows();
    }
}

/** Set a client horizontally|vertically maximized.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s The maximized status.
 */
void
client_set_maximized_common(lua_State *L, int cidx, bool s, const char* type, const int val)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    /* Store the current and next state on 2 bit */
    const client_maximized_t current = (
        (c->maximized_vertical   ? CLIENT_MAXIMIZED_V    : CLIENT_MAXIMIZED_NONE)|
        (c->maximized_horizontal ? CLIENT_MAXIMIZED_H    : CLIENT_MAXIMIZED_NONE)|
        (c->maximized            ? CLIENT_MAXIMIZED_BOTH : CLIENT_MAXIMIZED_NONE)
    );
    client_maximized_t next = s ? (val | current) : (current & (~val));

    /* When both are already set during startup, assume `maximized` is true*/
    if (next == (CLIENT_MAXIMIZED_H|CLIENT_MAXIMIZED_V) && !globalconf.loop)
        next = CLIENT_MAXIMIZED_BOTH;

    if(current != next)
    {
        int abs_cidx   = luaA_absindex(L, cidx);
        int max_before = c->maximized;
        int h_before   = c->maximized_horizontal;
        int v_before   = c->maximized_vertical;

        /*Update the client properties */
        c->maximized_horizontal = !!(next & CLIENT_MAXIMIZED_H   );
        c->maximized_vertical   = !!(next & CLIENT_MAXIMIZED_V   );
        c->maximized            = !!(next & CLIENT_MAXIMIZED_BOTH);


        /* Request the changes to be applied */
        lua_pushstring(L, type);
        luaA_object_emit_signal(L, abs_cidx, "request::geometry", 1);

        /* Notify changes in the relevant properties */
        if (h_before != c->maximized_horizontal)
            luaA_object_emit_signal(L, abs_cidx, "property::maximized_horizontal", 0);
        if (v_before != c->maximized_vertical)
            luaA_object_emit_signal(L, abs_cidx, "property::maximized_vertical", 0);
        if(max_before != c->maximized)
            luaA_object_emit_signal(L, abs_cidx, "property::maximized", 0);

        stack_windows();
    }
}

void
client_set_maximized(lua_State *L, int cidx, bool s)
{
    return client_set_maximized_common(
        L, cidx, s, "maximized", CLIENT_MAXIMIZED_BOTH
    );
}

void
client_set_maximized_horizontal(lua_State *L, int cidx, bool s)
{
    return client_set_maximized_common(
        L, cidx, s, "maximized_horizontal", CLIENT_MAXIMIZED_H
    );
}

void
client_set_maximized_vertical(lua_State *L, int cidx, bool s)
{
    return client_set_maximized_common(
        L, cidx, s, "maximized_vertical", CLIENT_MAXIMIZED_V
    );
}

/** Set a client above, or not.
 * \param L The Lua VM state.
 * \param cidx The client index.
 * \param s Set or not the client above.
 */
void
client_set_above(lua_State *L, int cidx, bool s)
{
    client_t *c = luaA_checkudata(L, cidx, &client_class);

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
        stack_windows();
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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

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
        stack_windows();
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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

    if(c->modal != s)
    {
        c->modal = s;
        stack_windows();
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
    client_t *c = luaA_checkudata(L, cidx, &client_class);

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
        stack_windows();
        luaA_object_emit_signal(L, cidx, "property::ontop", 0);
    }
}

/** Unban a client and move it back into the viewport.
 * \param c The client.
 */
void
client_unban(client_t *c)
{
    lua_State *L = globalconf_get_lua_State();
    if(c->isbanned)
    {
        client_ignore_enterleave_events();
        xcb_map_window(globalconf.connection, c->frame_window);
        client_restore_enterleave_events();

        c->isbanned = false;

        /* An unbanned client shouldn't be minimized or hidden */
        luaA_object_push(L, c);
        client_set_minimized(L, -1, false);
        client_set_hidden(L, -1, false);
        lua_pop(L, 1);

        if (globalconf.focus.client == c)
            globalconf.focus.need_update = true;
    }
}

/** Unmanage a client.
 * \param c The client.
 * \param window_valid Is the client's window still valid?
 */
void
client_unmanage(client_t *c, bool window_valid)
{
    lua_State *L = globalconf_get_lua_State();

    /* Reset transient_for attributes of windows that might be referring to us */
    foreach(_tc, globalconf.clients)
    {
        client_t *tc = *_tc;
        if(tc->transient_for == c)
            tc->transient_for = NULL;
    }

    if(globalconf.focus.client == c)
        client_unfocus(c);

    /* remove client from global list and everywhere else */
    foreach(elem, globalconf.clients)
        if(*elem == c)
        {
            client_array_remove(&globalconf.clients, elem);
            break;
        }
    stack_client_remove(c);
    for(int i = 0; i < globalconf.tags.len; i++)
        untag_client(c, globalconf.tags.tab[i]);

    luaA_object_push(L, c);
    luaA_object_emit_signal(L, -1, "unmanage", 0);
    lua_pop(L, 1);

    luaA_class_emit_signal(L, &client_class, "list", 0);

    if(strut_has_value(&c->strut))
        screen_update_workarea(c->screen);

    /* Get rid of all titlebars */
    for (client_titlebar_t bar = CLIENT_TITLEBAR_TOP; bar < CLIENT_TITLEBAR_COUNT; bar++) {
        if (c->titlebar[bar].drawable == NULL)
            continue;

        if (globalconf.drawable_under_mouse == c->titlebar[bar].drawable) {
            /* Leave drawable before we invalidate the client */
            lua_pushnil(L);
            event_drawable_under_mouse(L, -1);
            lua_pop(L, 1);
        }

        /* Forget about the drawable */
        luaA_object_push(L, c);
        luaA_object_unref_item(L, -1, c->titlebar[bar].drawable);
        c->titlebar[bar].drawable = NULL;
        lua_pop(L, 1);
    }

    /* Clear our event mask so that we don't receive any events from now on,
     * especially not for the following requests. */
    if(window_valid)
        xcb_change_window_attributes(globalconf.connection,
                                     c->window,
                                     XCB_CW_EVENT_MASK,
                                     (const uint32_t []) { 0 });
    xcb_change_window_attributes(globalconf.connection,
                                 c->frame_window,
                                 XCB_CW_EVENT_MASK,
                                 (const uint32_t []) { 0 });

    if(window_valid)
    {
        xcb_unmap_window(globalconf.connection, c->window);
        xcb_reparent_window(globalconf.connection, c->window, globalconf.screen->root,
                c->geometry.x, c->geometry.y);
    }

    if (c->nofocus_window != XCB_NONE)
        window_array_append(&globalconf.destroy_later_windows, c->nofocus_window);
    window_array_append(&globalconf.destroy_later_windows, c->frame_window);

    if(window_valid)
    {
        /* Remove this window from the save set since this shouldn't be made visible
         * after a restart anymore. */
        xcb_change_save_set(globalconf.connection, XCB_SET_MODE_DELETE, c->window);
        if (globalconf.have_shape)
            xcb_shape_select_input(globalconf.connection, c->window, 0);

        /* Do this last to avoid races with clients. According to ICCCM, clients
         * arent allowed to re-use the window until after this. */
        xwindow_set_state(c->window, XCB_ICCCM_WM_STATE_WITHDRAWN);
    }

    /* set client as invalid */
    c->window = XCB_NONE;

    luaA_object_unref(L, c);
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
        ev.data.data32[1] = globalconf.timestamp;
        ev.type = WM_PROTOCOLS;
        ev.data.data32[0] = WM_DELETE_WINDOW;

        xcb_send_event(globalconf.connection, false, c->window,
                       XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
    }
    else
        xcb_kill_client(globalconf.connection, c->window);
}

/** Get all clients into a table.
 *
 * @tparam[opt] integer screen A screen number to filter clients on.
 * @tparam[opt] boolean stacked Return clients in stacking order? (ordered from
 *   top to bottom).
 * @treturn table A table with clients.
 * @function get
 */
static int
luaA_client_get(lua_State *L)
{
    int i = 1;
    screen_t *screen = NULL;
    bool stacked = false;

    if(!lua_isnoneornil(L, 1))
        screen = luaA_checkscreen(L, 1);

    if(!lua_isnoneornil(L, 2))
        stacked = luaA_checkboolean(L, 2);

    lua_newtable(L);
    if(stacked)
    {
        foreach_reverse(c, globalconf.stack)
            if(screen == NULL || (*c)->screen == screen)
            {
                luaA_object_push(L, *c);
                lua_rawseti(L, -2, i++);
            }
    }
    else
    {
        foreach(c, globalconf.clients)
            if(screen == NULL || (*c)->screen == screen)
            {
                luaA_object_push(L, *c);
                lua_rawseti(L, -2, i++);
            }
    }

    return 1;
}

/** Check if a client is visible on its screen.
 *
 * @return A boolean value, true if the client is visible, false otherwise.
 * @function isvisible
 */
static int
luaA_client_isvisible(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    lua_pushboolean(L, client_isvisible(c));
    return 1;
}

/** Set client icons.
 * \param L The Lua VM state.
 * \param array Array of icons to set.
 */
void
client_set_icons(client_t *c, cairo_surface_array_t array)
{
    cairo_surface_array_wipe(&c->icons);
    c->icons = array;

    lua_State *L = globalconf_get_lua_State();
    luaA_object_push(L, c);
    luaA_object_emit_signal(L, -1, "property::icon", 0);
    luaA_object_emit_signal(L, -1, "property::icon_sizes", 0);
    lua_pop(L, 1);
}

/** Set a client icon.
 * \param L The Lua VM state.
 * \param cidx The client index on the stack.
 * \param iidx The image index on the stack.
 */
static void
client_set_icon(client_t *c, cairo_surface_t *s)
{
    cairo_surface_array_t array;
    cairo_surface_array_init(&array);
    if (s && cairo_surface_status(s) == CAIRO_STATUS_SUCCESS)
        cairo_surface_array_push(&array, draw_dup_image_surface(s));
    client_set_icons(c, array);
}


/** Set a client icon.
 * \param c The client to change.
 * \param icon A bitmap containing the icon.
 * \param mask A mask for the bitmap (optional)
 */
void
client_set_icon_from_pixmaps(client_t *c, xcb_pixmap_t icon, xcb_pixmap_t mask)
{
    xcb_get_geometry_cookie_t geom_icon_c, geom_mask_c;
    xcb_get_geometry_reply_t *geom_icon_r, *geom_mask_r = NULL;
    cairo_surface_t *s_icon, *result;

    geom_icon_c = xcb_get_geometry_unchecked(globalconf.connection, icon);
    if (mask)
        geom_mask_c = xcb_get_geometry_unchecked(globalconf.connection, mask);
    geom_icon_r = xcb_get_geometry_reply(globalconf.connection, geom_icon_c, NULL);
    if (mask)
        geom_mask_r = xcb_get_geometry_reply(globalconf.connection, geom_mask_c, NULL);

    if (!geom_icon_r || (mask && !geom_mask_r))
        goto out;
    if ((geom_icon_r->depth != 1 && geom_icon_r->depth != globalconf.screen->root_depth)
            || (geom_mask_r && geom_mask_r->depth != 1))
    {
        warn("Got pixmaps with depth (%d, %d) while processing icon, but only depth 1 and %d are allowed",
                geom_icon_r->depth, geom_mask_r ? geom_mask_r->depth : 0, globalconf.screen->root_depth);
        goto out;
    }

    if (geom_icon_r->depth == 1)
        s_icon = cairo_xcb_surface_create_for_bitmap(globalconf.connection,
                globalconf.screen, icon, geom_icon_r->width, geom_icon_r->height);
    else
        s_icon = cairo_xcb_surface_create(globalconf.connection, icon, globalconf.default_visual,
                geom_icon_r->width, geom_icon_r->height);
    result = s_icon;

    if (mask)
    {
        cairo_surface_t *s_mask;
        cairo_t *cr;

        result = cairo_surface_create_similar(s_icon, CAIRO_CONTENT_COLOR_ALPHA, geom_icon_r->width, geom_icon_r->height);
        s_mask = cairo_xcb_surface_create_for_bitmap(globalconf.connection,
                globalconf.screen, mask, geom_icon_r->width, geom_icon_r->height);
        cr = cairo_create(result);

        cairo_set_source_surface(cr, s_icon, 0, 0);
        cairo_mask_surface(cr, s_mask, 0, 0);
        cairo_surface_destroy(s_mask);
        cairo_destroy(cr);
    }

    client_set_icon(c, result);

    cairo_surface_destroy(result);
    if (result != s_icon)
        cairo_surface_destroy(s_icon);

out:
    p_delete(&geom_icon_r);
    p_delete(&geom_mask_r);
}


/** Kill a client.
 *
 * @function kill
 */
static int
luaA_client_kill(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    client_kill(c);
    return 0;
}

/** Swap a client with another one in global client list.
 * @client c A client to swap with.
 * @function swap
 */
static int
luaA_client_swap(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    client_t *swap = luaA_checkudata(L, 2, &client_class);

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

        luaA_class_emit_signal(L, &client_class, "list", 0);

        luaA_object_push(L, swap);
        lua_pushboolean(L, true);
        luaA_object_emit_signal(L, -4, "swapped", 2);

        luaA_object_push(L, swap);
        luaA_object_push(L, c);
        lua_pushboolean(L, false);
        luaA_object_emit_signal(L, -3, "swapped", 2);
    }

    return 0;
}

/** Access or set the client tags.
 *
 * Use the `first_tag` field to access the first tag of a client directly.
 *
 * **Signal:**
 *
 *  * *property::tags*
 *
 * @tparam table tags_table A table with tags to set, or `nil` to get the
 *   current tags.
 * @treturn table A table with all tags.
 * @function tags
 */
static int
luaA_client_tags(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    int j = 0;

    if(lua_gettop(L) == 2)
    {
        luaA_checktable(L, 2);
        for(int i = 0; i < globalconf.tags.len; i++)
        {
            /* Only untag if we aren't going to add this tag again */
            bool found = false;
            lua_pushnil(L);
            while(lua_next(L, 2))
            {
                tag_t *t = lua_touserdata(L, -1);
                /* Pop the value from lua_next */
                lua_pop(L, 1);
                if (t != globalconf.tags.tab[i])
                    continue;

                /* Pop the key from lua_next */
                lua_pop(L, 1);
                found = true;
                break;
            }
            if(!found)
                untag_client(c, globalconf.tags.tab[i]);
        }
        lua_pushnil(L);
        while(lua_next(L, 2))
            tag_client(L, c);

        lua_pop(L, 1);

        luaA_object_emit_signal(L, -1, "property::tags", 0);
    }

    lua_newtable(L);
    foreach(tag, globalconf.tags)
        if(is_client_tagged(c, *tag))
        {
            luaA_object_push(L, *tag);
            lua_rawseti(L, -2, ++j);
        }

    return 1;
}

/** Get the first tag of a client.
 */
static int
luaA_client_get_first_tag(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);

    foreach(tag, globalconf.tags)
        if(is_client_tagged(c, *tag))
        {
            luaA_object_push(L, *tag);
            return 1;
        }

    return 0;
}

/** Raise a client on top of others which are on the same layer.
 *
 * @function raise
 */
static int
luaA_client_raise(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);

    /* Avoid sending the signal if nothing was done */
    if (c->transient_for == NULL &&
        globalconf.stack.len &&
        globalconf.stack.tab[globalconf.stack.len-1] == c
    )
        return 0;

    client_raise(c);

    return 0;
}

/** Lower a client on bottom of others which are on the same layer.
 *
 * @function lower
 */
static int
luaA_client_lower(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);

    /* Avoid sending the signal if nothing was done */
    if (globalconf.stack.len && globalconf.stack.tab[0] == c)
        return 0;

    stack_client_push(c);

    /* Traverse all transient layers. */
    for(client_t *tc = c->transient_for; tc; tc = tc->transient_for)
        stack_client_push(tc);

    /* Notify the listeners */
    luaA_object_push(L, c);
    luaA_object_emit_signal(L, -1, "lowered", 0);
    lua_pop(L, 1);

    return 0;
}

/** Stop managing a client.
 *
 * @function unmanage
 */
static int
luaA_client_unmanage(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    client_unmanage(c, true);
    return 0;
}

static area_t
titlebar_get_area(client_t *c, client_titlebar_t bar)
{
    area_t result = c->geometry;
    result.x = result.y = 0;

    // Let's try some ascii art:
    // ---------------------------
    // |         Top             |
    // |-------------------------|
    // |L|                     |R|
    // |e|                     |i|
    // |f|                     |g|
    // |t|                     |h|
    // | |                     |t|
    // |-------------------------|
    // |        Bottom           |
    // ---------------------------

    switch (bar) {
    case CLIENT_TITLEBAR_BOTTOM:
        result.y = c->geometry.height - c->titlebar[bar].size;
        /* Fall through */
    case CLIENT_TITLEBAR_TOP:
        result.height = c->titlebar[bar].size;
        break;
    case CLIENT_TITLEBAR_RIGHT:
        result.x = c->geometry.width - c->titlebar[bar].size;
        /* Fall through */
    case CLIENT_TITLEBAR_LEFT:
        result.y = c->titlebar[CLIENT_TITLEBAR_TOP].size;
        result.width = c->titlebar[bar].size;
        result.height -= c->titlebar[CLIENT_TITLEBAR_TOP].size;
        result.height -= c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;
        break;
    default:
        fatal("Unknown titlebar kind %d\n", (int) bar);
    }

    return result;
}

drawable_t *
client_get_drawable_offset(client_t *c, int *x, int *y)
{
    for (client_titlebar_t bar = CLIENT_TITLEBAR_TOP; bar < CLIENT_TITLEBAR_COUNT; bar++) {
        area_t area = titlebar_get_area(c, bar);
        if (AREA_LEFT(area) > *x || AREA_RIGHT(area) <= *x)
            continue;
        if (AREA_TOP(area) > *y || AREA_BOTTOM(area) <= *y)
            continue;

        *x -= area.x;
        *y -= area.y;
        return c->titlebar[bar].drawable;
    }

    return NULL;
}

drawable_t *
client_get_drawable(client_t *c, int x, int y)
{
    return client_get_drawable_offset(c, &x, &y);
}

static void
client_refresh_titlebar_partial(client_t *c, client_titlebar_t bar, int16_t x, int16_t y, uint16_t width, uint16_t height)
{
    if(c->titlebar[bar].drawable == NULL
            || c->titlebar[bar].drawable->pixmap == XCB_NONE
            || !c->titlebar[bar].drawable->refreshed)
        return;

    /* Is the titlebar part of the area that should get redrawn? */
    area_t area = titlebar_get_area(c, bar);
    if (AREA_LEFT(area) >= x + width || AREA_RIGHT(area) <= x)
        return;
    if (AREA_TOP(area) >= y + height || AREA_BOTTOM(area) <= y)
        return;

    /* Redraw the affected parts */
    cairo_surface_flush(c->titlebar[bar].drawable->surface);
    xcb_copy_area(globalconf.connection, c->titlebar[bar].drawable->pixmap, c->frame_window,
            globalconf.gc, x - area.x, y - area.y, x, y, width, height);
}

#define HANDLE_TITLEBAR_REFRESH(name, index)                                                \
static void                                                                                 \
client_refresh_titlebar_ ## name(client_t *c)                                               \
{                                                                                           \
    area_t area = titlebar_get_area(c, index);                                              \
    client_refresh_titlebar_partial(c, index, area.x, area.y, area.width, area.height);     \
}
HANDLE_TITLEBAR_REFRESH(top, CLIENT_TITLEBAR_TOP)
HANDLE_TITLEBAR_REFRESH(right, CLIENT_TITLEBAR_RIGHT)
HANDLE_TITLEBAR_REFRESH(bottom, CLIENT_TITLEBAR_BOTTOM)
HANDLE_TITLEBAR_REFRESH(left, CLIENT_TITLEBAR_LEFT)

/**
 * Refresh all titlebars that are in the specified rectangle
 */
void
client_refresh_partial(client_t *c, int16_t x, int16_t y, uint16_t width, uint16_t height)
{
    for (client_titlebar_t bar = CLIENT_TITLEBAR_TOP; bar < CLIENT_TITLEBAR_COUNT; bar++) {
        client_refresh_titlebar_partial(c, bar, x, y, width, height);
    }
}

static drawable_t *
titlebar_get_drawable(lua_State *L, client_t *c, int cl_idx, client_titlebar_t bar)
{
    if (c->titlebar[bar].drawable == NULL)
    {
        cl_idx = luaA_absindex(L, cl_idx);
        switch (bar) {
        case CLIENT_TITLEBAR_TOP:
            drawable_allocator(L, (drawable_refresh_callback *) client_refresh_titlebar_top, c);
            break;
        case CLIENT_TITLEBAR_BOTTOM:
            drawable_allocator(L, (drawable_refresh_callback *) client_refresh_titlebar_bottom, c);
            break;
        case CLIENT_TITLEBAR_RIGHT:
            drawable_allocator(L, (drawable_refresh_callback *) client_refresh_titlebar_right, c);
            break;
        case CLIENT_TITLEBAR_LEFT:
            drawable_allocator(L, (drawable_refresh_callback *) client_refresh_titlebar_left, c);
            break;
        default:
            fatal("Unknown titlebar kind %d\n", (int) bar);
        }
        c->titlebar[bar].drawable = luaA_object_ref_item(L, cl_idx, -1);
    }

    return c->titlebar[bar].drawable;
}

static void
titlebar_resize(lua_State *L, int cidx, client_t *c, client_titlebar_t bar, int size)
{
    const char *property_name;

    if (size < 0)
        return;

    if (size == c->titlebar[bar].size)
        return;

    /* Now resize the client (and titlebars!) suitably (the client without
     * titlebars should keep its current size!) */
    area_t geometry = c->geometry;
    int change = size - c->titlebar[bar].size;
    int16_t diff_top = 0, diff_bottom = 0, diff_right = 0, diff_left = 0;
    switch (bar) {
    case CLIENT_TITLEBAR_TOP:
        geometry.height += change;
        diff_top = change;
        property_name = "property::titlebar_top";
        break;
    case CLIENT_TITLEBAR_BOTTOM:
        geometry.height += change;
        diff_bottom = change;
        property_name = "property::titlebar_bottom";
        break;
    case CLIENT_TITLEBAR_RIGHT:
        geometry.width += change;
        diff_right = change;
        property_name = "property::titlebar_right";
        break;
    case CLIENT_TITLEBAR_LEFT:
        geometry.width += change;
        diff_left = change;
        property_name = "property::titlebar_left";
        break;
    default:
        fatal("Unknown titlebar kind %d\n", (int) bar);
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY)
    {
        int16_t diff_x = 0, diff_y = 0;
        xwindow_translate_for_gravity(c->size_hints.win_gravity,
                                      diff_left, diff_top,
                                      diff_right, diff_bottom,
                                      &diff_x, &diff_y);
        geometry.x += diff_x;
        geometry.y += diff_y;
    }

    c->titlebar[bar].size = size;
    client_resize_do(c, geometry);

    luaA_object_emit_signal(L, cidx, property_name, 0);
}

#define HANDLE_TITLEBAR(name, index)                              \
static int                                                        \
luaA_client_titlebar_ ## name(lua_State *L)                       \
{                                                                 \
    client_t *c = luaA_checkudata(L, 1, &client_class);           \
                                                                  \
    if (lua_gettop(L) == 2)                                       \
    {                                                             \
        if (lua_isnil(L, 2))                                      \
            titlebar_resize(L, 1, c, index, 0);                   \
        else                                                      \
            titlebar_resize(L, 1, c, index, ceil(luaA_checknumber_range(L, 2, 0, MAX_X11_SIZE))); \
    }                                                             \
                                                                  \
    luaA_object_push_item(L, 1, titlebar_get_drawable(L, c, 1, index)); \
    lua_pushinteger(L, c->titlebar[index].size);                   \
    return 2;                                                     \
}
HANDLE_TITLEBAR(top, CLIENT_TITLEBAR_TOP)
HANDLE_TITLEBAR(right, CLIENT_TITLEBAR_RIGHT)
HANDLE_TITLEBAR(bottom, CLIENT_TITLEBAR_BOTTOM)
HANDLE_TITLEBAR(left, CLIENT_TITLEBAR_LEFT)

/** Return or set client geometry.
 *
 * @tparam table|nil geo A table with new coordinates, or nil.
 * @treturn table A table with client geometry and coordinates.
 * @function geometry
 */
static int
luaA_client_geometry(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);

    if(lua_gettop(L) == 2 && !lua_isnil(L, 2))
    {
        area_t geometry;

        luaA_checktable(L, 2);
        geometry.x = round(luaA_getopt_number_range(L, 2, "x", c->geometry.x, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
        geometry.y = round(luaA_getopt_number_range(L, 2, "y", c->geometry.y, MIN_X11_COORDINATE, MAX_X11_COORDINATE));
        if(client_isfixed(c))
        {
            geometry.width = c->geometry.width;
            geometry.height = c->geometry.height;
        }
        else
        {
            geometry.width = ceil(luaA_getopt_number_range(L, 2, "width", c->geometry.width, MIN_X11_SIZE, MAX_X11_SIZE));
            geometry.height = ceil(luaA_getopt_number_range(L, 2, "height", c->geometry.height, MIN_X11_SIZE, MAX_X11_SIZE));
        }

        client_resize(c, geometry, c->size_hints_honor);
    }

    return luaA_pusharea(L, c->geometry);
}

/** Apply size hints to a size.
 *
 * @param width Desired width of client
 * @param height Desired height of client
 * @return Actual width of client
 * @return Actual height of client
 * @function apply_size_hints
 */
static int
luaA_client_apply_size_hints(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    area_t geometry = c->geometry;
    if(!client_isfixed(c))
    {
        geometry.width = ceil(luaA_checknumber_range(L, 2, MIN_X11_SIZE, MAX_X11_SIZE));
        geometry.height = ceil(luaA_checknumber_range(L, 3, MIN_X11_SIZE, MAX_X11_SIZE));
    }

    if (c->size_hints_honor)
        geometry = client_apply_size_hints(c, geometry);

    lua_pushinteger(L, geometry.width);
    lua_pushinteger(L, geometry.height);
    return 2;
}

static int
luaA_client_set_screen(lua_State *L, client_t *c)
{
    screen_client_moveto(c, luaA_checkscreen(L, -1), true);
    return 0;
}

static int
luaA_client_set_hidden(lua_State *L, client_t *c)
{
    client_set_hidden(L, -3, luaA_checkboolean(L, -1));
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
luaA_client_set_maximized(lua_State *L, client_t *c)
{
    client_set_maximized(L, -3, luaA_checkboolean(L, -1));
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
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);
    client_set_icon(c, surf);
    return 0;
}

static int
luaA_client_set_focusable(lua_State *L, client_t *c)
{
    if(lua_isnil(L, -1))
        client_unset_focusable(L, -3);
    else
        client_set_focusable(L, -3, luaA_checkboolean(L, -1));
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
    luaA_object_emit_signal(L, -3, "property::size_hints_honor", 0);
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

/** Set the client name.
 * \param L The Lua VM state.
 * \param client The client to name.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_set_name(lua_State *L, client_t *c)
{
    const char *name = luaL_checkstring(L, -1);
    client_set_name(L, 1, a_strdup(name));
    return 0;
}

static int
luaA_client_get_icon_name(lua_State *L, client_t *c)
{
    lua_pushstring(L, c->icon_name ? c->icon_name : c->alt_icon_name);
    return 1;
}

LUA_OBJECT_EXPORT_OPTIONAL_PROPERTY(client, client_t, screen, luaA_object_push, NULL)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, class, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, instance, lua_pushstring)
LUA_OBJECT_EXPORT_OPTIONAL_PROPERTY(client, client_t, machine, lua_pushstring, NULL)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, role, lua_pushstring)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, transient_for, luaA_object_push)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, skip_taskbar, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, leader_window, lua_pushinteger)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, group_window, lua_pushinteger)
LUA_OBJECT_EXPORT_OPTIONAL_PROPERTY(client, client_t, pid, lua_pushinteger, 0)
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
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, maximized, lua_pushboolean)
LUA_OBJECT_EXPORT_PROPERTY(client, client_t, startup_id, lua_pushstring)

static int
luaA_client_get_content(lua_State *L, client_t *c)
{
    cairo_surface_t *surface;
    int width  = c->geometry.width;
    int height = c->geometry.height;

    /* Just the client size without decorations */
    width  -= c->titlebar[CLIENT_TITLEBAR_LEFT].size + c->titlebar[CLIENT_TITLEBAR_RIGHT].size;
    height -= c->titlebar[CLIENT_TITLEBAR_TOP].size + c->titlebar[CLIENT_TITLEBAR_BOTTOM].size;

    surface = cairo_xcb_surface_create(globalconf.connection, c->window,
                                       c->visualtype, width, height);

    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surface);
    return 1;
}

static int
luaA_client_get_icon(lua_State *L, client_t *c)
{
    if(c->icons.len == 0)
        return 0;

    /* Pick the closest available size, only picking a smaller icon if no bigger
     * one is available.
     */
    cairo_surface_t *found = NULL;
    int found_size = 0;
    int preferred_size = globalconf.preferred_icon_size;

    foreach(surf, c->icons)
    {
        int width = cairo_image_surface_get_width(*surf);
        int height = cairo_image_surface_get_height(*surf);
        int size = MAX(width, height);

        /* pick the icon if it's a better match than the one we already have */
        bool found_icon_too_small = found_size < preferred_size;
        bool found_icon_too_large = found_size > preferred_size;
        bool icon_empty = width == 0 || height == 0;
        bool better_because_bigger =  found_icon_too_small && size > found_size;
        bool better_because_smaller = found_icon_too_large &&
            size >= preferred_size && size < found_size;
        if (!icon_empty && (better_because_bigger || better_because_smaller || found_size == 0))
        {
            found = *surf;
            found_size = size;
        }
    }

    /* lua gets its own reference which it will have to destroy */
    lua_pushlightuserdata(L, cairo_surface_reference(found));
    return 1;
}

static int
luaA_client_get_focusable(lua_State *L, client_t *c)
{
    bool ret;

    if (c->focusable_set)
        ret = c->focusable;

    /* A client can be focused if it doesnt have the "nofocus" hint...*/
    else if (!c->nofocus)
        ret = true;
    else
        /* ...or if it knows the WM_TAKE_FOCUS protocol */
        ret = client_hasproto(c, WM_TAKE_FOCUS);

    lua_pushboolean(L, ret);
    return 1;
}

static int
luaA_client_get_size_hints(lua_State *L, client_t *c)
{
    const char *u_or_p = NULL;

    lua_createtable(L, 0, 1);

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_US_POSITION)
        u_or_p = "user_position";
    else if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_POSITION)
        u_or_p = "program_position";

    if(u_or_p)
    {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, c->size_hints.x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, c->size_hints.y);
        lua_setfield(L, -2, "y");
        lua_setfield(L, -2, u_or_p);
        u_or_p = NULL;
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_US_SIZE)
        u_or_p = "user_size";
    else if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_SIZE)
        u_or_p = "program_size";

    if(u_or_p)
    {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, c->size_hints.width);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, c->size_hints.height);
        lua_setfield(L, -2, "height");
        lua_setfield(L, -2, u_or_p);
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
        lua_pushinteger(L, c->size_hints.min_width);
        lua_setfield(L, -2, "min_width");
        lua_pushinteger(L, c->size_hints.min_height);
        lua_setfield(L, -2, "min_height");
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
    {
        lua_pushinteger(L, c->size_hints.max_width);
        lua_setfield(L, -2, "max_width");
        lua_pushinteger(L, c->size_hints.max_height);
        lua_setfield(L, -2, "max_height");
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
    {
        lua_pushinteger(L, c->size_hints.width_inc);
        lua_setfield(L, -2, "width_inc");
        lua_pushinteger(L, c->size_hints.height_inc);
        lua_setfield(L, -2, "height_inc");
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_ASPECT)
    {
        lua_pushinteger(L, c->size_hints.min_aspect_num);
        lua_setfield(L, -2, "min_aspect_num");
        lua_pushinteger(L, c->size_hints.min_aspect_den);
        lua_setfield(L, -2, "min_aspect_den");
        lua_pushinteger(L, c->size_hints.max_aspect_num);
        lua_setfield(L, -2, "max_aspect_num");
        lua_pushinteger(L, c->size_hints.max_aspect_den);
        lua_setfield(L, -2, "max_aspect_den");
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
        lua_pushinteger(L, c->size_hints.base_width);
        lua_setfield(L, -2, "base_width");
        lua_pushinteger(L, c->size_hints.base_height);
        lua_setfield(L, -2, "base_height");
    }

    if(c->size_hints.flags & XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY)
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

/** Get the client's child window bounding shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_get_client_shape_bounding(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = xwindow_get_shape(c->window, XCB_SHAPE_SK_BOUNDING);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Get the client's frame window bounding shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_get_shape_bounding(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = xwindow_get_shape(c->frame_window, XCB_SHAPE_SK_BOUNDING);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the client's frame window bounding shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_set_shape_bounding(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);
    xwindow_set_shape(c->frame_window,
            c->geometry.width + (c->border_width * 2),
            c->geometry.height + (c->border_width * 2),
            XCB_SHAPE_SK_BOUNDING, surf, -c->border_width);
    luaA_object_emit_signal(L, -3, "property::shape_bounding", 0);
    return 0;
}

/** Get the client's child window clip shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_get_client_shape_clip(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = xwindow_get_shape(c->window, XCB_SHAPE_SK_CLIP);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Get the client's frame window clip shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_get_shape_clip(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = xwindow_get_shape(c->frame_window, XCB_SHAPE_SK_CLIP);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the client's frame window clip shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_set_shape_clip(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);
    xwindow_set_shape(c->frame_window, c->geometry.width, c->geometry.height,
            XCB_SHAPE_SK_CLIP, surf, 0);
    luaA_object_emit_signal(L, -3, "property::shape_clip", 0);
    return 0;
}

/** Get the client's frame window input shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_get_shape_input(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = xwindow_get_shape(c->frame_window, XCB_SHAPE_SK_INPUT);
    if (!surf)
        return 0;
    /* lua has to make sure to free the ref or we have a leak */
    lua_pushlightuserdata(L, surf);
    return 1;
}

/** Set the client's frame window input shape.
 * \param L The Lua VM state.
 * \param client The client object.
 * \return The number of elements pushed on stack.
 */
static int
luaA_client_set_shape_input(lua_State *L, client_t *c)
{
    cairo_surface_t *surf = NULL;
    if(!lua_isnil(L, -1))
        surf = (cairo_surface_t *)lua_touserdata(L, -1);
    xwindow_set_shape(c->frame_window,
            c->geometry.width + (c->border_width * 2),
            c->geometry.height + (c->border_width * 2),
            XCB_SHAPE_SK_INPUT, surf, -c->border_width);
    luaA_object_emit_signal(L, -3, "property::shape_input", 0);
    return 0;
}

/** Get or set keys bindings for a client.
 *
 * @param keys_table An array of key bindings objects, or nothing.
 * @return A table with all keys.
 * @function keys
 */
static int
luaA_client_keys(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    key_array_t *keys = &c->keys;

    if(lua_gettop(L) == 2)
    {
        luaA_key_array_set(L, 1, 2, keys);
        luaA_object_emit_signal(L, 1, "property::keys", 0);
        xwindow_grabkeys(c->window, keys);
        if (c->nofocus_window)
            xwindow_grabkeys(c->nofocus_window, &c->keys);
    }

    return luaA_key_array_get(L, 1, keys);
}

static int
luaA_client_get_icon_sizes(lua_State *L)
{
    int index = 1;
    client_t *c = luaA_checkudata(L, 1, &client_class);

    lua_newtable(L);
    foreach (s, c->icons) {
        /* Create a table { width, height } and append it to the table */
        lua_createtable(L, 2, 0);

        lua_pushinteger(L, cairo_image_surface_get_width(*s));
        lua_rawseti(L, -2, 1);

        lua_pushinteger(L, cairo_image_surface_get_height(*s));
        lua_rawseti(L, -2, 2);

        lua_rawseti(L, -2, index++);
    }
    return 1;
}

/** Get the client's n-th icon.
 *
 * **Signal:**
 *
 *  * *property::icon*
 *
 * @tparam interger index The index in the list of icons to get.
 * @treturn surface A lightuserdata for a cairo surface. This reference must be
 * destroyed!
 * @function get_icon
 */
static int
luaA_client_get_some_icon(lua_State *L)
{
    client_t *c = luaA_checkudata(L, 1, &client_class);
    int index = luaL_checkinteger(L, 2);
    luaL_argcheck(L, (index >= 1 && index <= c->icons.len), 2,
            "invalid icon index");
    lua_pushlightuserdata(L, cairo_surface_reference(c->icons.tab[index-1]));
    return 1;
}

static int
client_tostring(lua_State *L, client_t *c)
{
    char *name = c->name ? c->name : c->alt_name;
    ssize_t len = a_strlen(name);
    ssize_t limit = 20;

    lua_pushlstring(L, name, MIN(len, limit));
    if (len > limit)
        lua_pushstring(L, "...");
    return len > limit ? 2 : 1;
}

/* Client module.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_index(lua_State *L)
{
    const char *buf = luaL_checkstring(L, 2);

    if (A_STREQ(buf, "focus"))
        return luaA_object_push(L, globalconf.focus.client);
    return 0;
}

/* Client module new index.
 * \param L The Lua VM state.
 * \return The number of pushed elements.
 */
static int
luaA_client_module_newindex(lua_State *L)
{
    const char *buf = luaL_checkstring(L, 2);
    client_t *c;

    if (A_STREQ(buf, "focus"))
    {
        c = luaA_checkudataornil(L, 3, &client_class);
        if (c)
            client_focus(c);
        else if (globalconf.focus.client)
            client_unfocus(globalconf.focus.client);
    }

    return 0;
}

static bool
client_checker(client_t *c)
{
    return c->window != XCB_NONE;
}

void
client_class_setup(lua_State *L)
{
    static const struct luaL_Reg client_methods[] =
    {
        LUA_CLASS_METHODS(client)
        { "get", luaA_client_get },
        { "__index", luaA_client_module_index },
        { "__newindex", luaA_client_module_newindex },
        { NULL, NULL }
    };

    static const struct luaL_Reg client_meta[] =
    {
        LUA_OBJECT_META(client)
        LUA_CLASS_META
        { "keys", luaA_client_keys },
        { "isvisible", luaA_client_isvisible },
        { "geometry", luaA_client_geometry },
        { "apply_size_hints", luaA_client_apply_size_hints },
        { "tags", luaA_client_tags },
        { "kill", luaA_client_kill },
        { "swap", luaA_client_swap },
        { "raise", luaA_client_raise },
        { "lower", luaA_client_lower },
        { "unmanage", luaA_client_unmanage },
        { "titlebar_top", luaA_client_titlebar_top },
        { "titlebar_right", luaA_client_titlebar_right },
        { "titlebar_bottom", luaA_client_titlebar_bottom },
        { "titlebar_left", luaA_client_titlebar_left },
        { "get_icon", luaA_client_get_some_icon },
        { NULL, NULL }
    };

    luaA_class_setup(L, &client_class, "client", &window_class,
                     (lua_class_allocator_t) client_new,
                     (lua_class_collector_t) client_wipe,
                     (lua_class_checker_t) client_checker,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     client_methods, client_meta);
    luaA_class_set_tostring(&client_class, (lua_class_propfunc_t) client_tostring);
    luaA_class_add_property(&client_class, "name",
                            (lua_class_propfunc_t) luaA_client_set_name,
                            (lua_class_propfunc_t) luaA_client_get_name,
                            (lua_class_propfunc_t) luaA_client_set_name);
    luaA_class_add_property(&client_class, "transient_for",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_transient_for,
                            NULL);
    luaA_class_add_property(&client_class, "skip_taskbar",
                            (lua_class_propfunc_t) luaA_client_set_skip_taskbar,
                            (lua_class_propfunc_t) luaA_client_get_skip_taskbar,
                            (lua_class_propfunc_t) luaA_client_set_skip_taskbar);
    luaA_class_add_property(&client_class, "content",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_content,
                            NULL);
    luaA_class_add_property(&client_class, "type",
                            NULL,
                            (lua_class_propfunc_t) luaA_window_get_type,
                            NULL);
    luaA_class_add_property(&client_class, "class",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_class,
                            NULL);
    luaA_class_add_property(&client_class, "instance",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_instance,
                            NULL);
    luaA_class_add_property(&client_class, "role",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_role,
                            NULL);
    luaA_class_add_property(&client_class, "pid",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_pid,
                            NULL);
    luaA_class_add_property(&client_class, "leader_window",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_leader_window,
                            NULL);
    luaA_class_add_property(&client_class, "machine",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_machine,
                            NULL);
    luaA_class_add_property(&client_class, "icon_name",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_icon_name,
                            NULL);
    luaA_class_add_property(&client_class, "screen",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_screen,
                            (lua_class_propfunc_t) luaA_client_set_screen);
    luaA_class_add_property(&client_class, "hidden",
                            (lua_class_propfunc_t) luaA_client_set_hidden,
                            (lua_class_propfunc_t) luaA_client_get_hidden,
                            (lua_class_propfunc_t) luaA_client_set_hidden);
    luaA_class_add_property(&client_class, "minimized",
                            (lua_class_propfunc_t) luaA_client_set_minimized,
                            (lua_class_propfunc_t) luaA_client_get_minimized,
                            (lua_class_propfunc_t) luaA_client_set_minimized);
    luaA_class_add_property(&client_class, "fullscreen",
                            (lua_class_propfunc_t) luaA_client_set_fullscreen,
                            (lua_class_propfunc_t) luaA_client_get_fullscreen,
                            (lua_class_propfunc_t) luaA_client_set_fullscreen);
    luaA_class_add_property(&client_class, "modal",
                            (lua_class_propfunc_t) luaA_client_set_modal,
                            (lua_class_propfunc_t) luaA_client_get_modal,
                            (lua_class_propfunc_t) luaA_client_set_modal);
    luaA_class_add_property(&client_class, "group_window",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_group_window,
                            NULL);
    luaA_class_add_property(&client_class, "maximized",
                            (lua_class_propfunc_t) luaA_client_set_maximized,
                            (lua_class_propfunc_t) luaA_client_get_maximized,
                            (lua_class_propfunc_t) luaA_client_set_maximized);
    luaA_class_add_property(&client_class, "maximized_horizontal",
                            (lua_class_propfunc_t) luaA_client_set_maximized_horizontal,
                            (lua_class_propfunc_t) luaA_client_get_maximized_horizontal,
                            (lua_class_propfunc_t) luaA_client_set_maximized_horizontal);
    luaA_class_add_property(&client_class, "maximized_vertical",
                            (lua_class_propfunc_t) luaA_client_set_maximized_vertical,
                            (lua_class_propfunc_t) luaA_client_get_maximized_vertical,
                            (lua_class_propfunc_t) luaA_client_set_maximized_vertical);
    luaA_class_add_property(&client_class, "icon",
                            (lua_class_propfunc_t) luaA_client_set_icon,
                            (lua_class_propfunc_t) luaA_client_get_icon,
                            (lua_class_propfunc_t) luaA_client_set_icon);
    luaA_class_add_property(&client_class, "icon_sizes",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_icon_sizes,
                            NULL);
    luaA_class_add_property(&client_class, "ontop",
                            (lua_class_propfunc_t) luaA_client_set_ontop,
                            (lua_class_propfunc_t) luaA_client_get_ontop,
                            (lua_class_propfunc_t) luaA_client_set_ontop);
    luaA_class_add_property(&client_class, "above",
                            (lua_class_propfunc_t) luaA_client_set_above,
                            (lua_class_propfunc_t) luaA_client_get_above,
                            (lua_class_propfunc_t) luaA_client_set_above);
    luaA_class_add_property(&client_class, "below",
                            (lua_class_propfunc_t) luaA_client_set_below,
                            (lua_class_propfunc_t) luaA_client_get_below,
                            (lua_class_propfunc_t) luaA_client_set_below);
    luaA_class_add_property(&client_class, "sticky",
                            (lua_class_propfunc_t) luaA_client_set_sticky,
                            (lua_class_propfunc_t) luaA_client_get_sticky,
                            (lua_class_propfunc_t) luaA_client_set_sticky);
    luaA_class_add_property(&client_class, "size_hints_honor",
                            (lua_class_propfunc_t) luaA_client_set_size_hints_honor,
                            (lua_class_propfunc_t) luaA_client_get_size_hints_honor,
                            (lua_class_propfunc_t) luaA_client_set_size_hints_honor);
    luaA_class_add_property(&client_class, "urgent",
                            (lua_class_propfunc_t) luaA_client_set_urgent,
                            (lua_class_propfunc_t) luaA_client_get_urgent,
                            (lua_class_propfunc_t) luaA_client_set_urgent);
    luaA_class_add_property(&client_class, "size_hints",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_size_hints,
                            NULL);
    luaA_class_add_property(&client_class, "focusable",
                            (lua_class_propfunc_t) luaA_client_set_focusable,
                            (lua_class_propfunc_t) luaA_client_get_focusable,
                            (lua_class_propfunc_t) luaA_client_set_focusable);
    luaA_class_add_property(&client_class, "shape_bounding",
                            (lua_class_propfunc_t) luaA_client_set_shape_bounding,
                            (lua_class_propfunc_t) luaA_client_get_shape_bounding,
                            (lua_class_propfunc_t) luaA_client_set_shape_bounding);
    luaA_class_add_property(&client_class, "shape_clip",
                            (lua_class_propfunc_t) luaA_client_set_shape_clip,
                            (lua_class_propfunc_t) luaA_client_get_shape_clip,
                            (lua_class_propfunc_t) luaA_client_set_shape_clip);
    luaA_class_add_property(&client_class, "shape_input",
                            (lua_class_propfunc_t) luaA_client_set_shape_input,
                            (lua_class_propfunc_t) luaA_client_get_shape_input,
                            (lua_class_propfunc_t) luaA_client_set_shape_input);
    luaA_class_add_property(&client_class, "startup_id",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_startup_id,
                            NULL);
    luaA_class_add_property(&client_class, "client_shape_bounding",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_client_shape_bounding,
                            NULL);
    luaA_class_add_property(&client_class, "client_shape_clip",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_client_shape_clip,
                            NULL);
    luaA_class_add_property(&client_class, "first_tag",
                            NULL,
                            (lua_class_propfunc_t) luaA_client_get_first_tag,
                            NULL);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
