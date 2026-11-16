---------------------------------------------------------------------------
-- StatusNotifierItem (D-Bus based system tray) support.
--
-- Modern applications (Electron, Qt, libappindicator/libayatana-appindicator,
-- KDE, ...) export their tray icons over D-Bus using the StatusNotifierItem
-- specification instead of the legacy XEmbed protocol.
--
-- This module implements both the `StatusNotifierWatcher` (the registry) and
-- a `StatusNotifierHost` (the tray). It is started by `wibox.widget.systray`,
-- which displays the items next to the XEmbed icons.
--
-- If another program (such as `snixembed` or another panel) already acts as
-- the watcher, awesome uses it instead of providing its own.
--
-- This is a private module.
--
-- See also the
-- [specification](https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/).
--
-- @author WooParadog
-- @copyright 2026 WooParadog
-- @module wibox.widget.systray.statusnotifier
---------------------------------------------------------------------------

local lgi = require("lgi")
local Gio, GLib, GObject = lgi.Gio, lgi.GLib, lgi.GObject
local gobject = require("gears.object")
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")
local watcher = require("wibox.widget.systray.watcher")
local sni_item = require("wibox.widget.systray.item")

local module = gobject { enable_properties = false }

--- Emitted when a new item is ready to be displayed.
-- @signal item::added
-- @tparam wibox.widget.systray.item item The new item.

--- Emitted when an item is removed.
-- @signal item::removed
-- @tparam wibox.widget.systray.item item The removed item.

--- Emitted when an item properties changed.
-- @signal item::updated
-- @tparam wibox.widget.systray.item item The item.

-- The protocol flavors. The order matters for display ordering only.
local prefixes = { "org.kde", "org.freedesktop" }

local started = false
local connection

-- All known items, in registration order.
local items = {}

-- The items for each prefix: prefix -> "service/path" -> item
local items_by_key = {}

local function split_item_name(name)
    -- The item name is "service/path" or (non-standard) only "service".
    local slash = name:find("/", 1, true)

    if not slash then
        return name, "/StatusNotifierItem"
    end

    return name:sub(1, slash - 1), name:sub(slash)
end

local function remove_item(prefix, key)
    local it = items_by_key[prefix][key]

    if not it then return end

    items_by_key[prefix][key] = nil

    local was_ready = false

    for k, v in ipairs(items) do
        if v == it then
            table.remove(items, k)
            was_ready = true
            break
        end
    end

    it:_remove()

    if was_ready then
        module:emit_signal("item::removed", it)
    end
end

local function add_item(prefix, key)
    if key == "" or items_by_key[prefix][key] then return end

    local service, path = split_item_name(key)

    local it = sni_item {
        connection = connection,
        prefix     = prefix,
        service    = service,
        path       = path,
    }

    items_by_key[prefix][key] = it

    it:connect_signal("ready", function()
        if items_by_key[prefix][key] ~= it then return end

        table.insert(items, it)
        module:emit_signal("item::added", it)
    end)

    it:connect_signal("updated", function()
        if items_by_key[prefix][key] ~= it then return end
        module:emit_signal("item::updated", it)
    end)
end

local function clear_items(prefix)
    for key in pairs(items_by_key[prefix]) do
        remove_item(prefix, key)
    end
end

local function start_host(prefix)
    items_by_key[prefix] = {}

    local interface = prefix .. ".StatusNotifierWatcher"
    local ok, pid = pcall(function()
        return Gio.Credentials.new():get_unix_pid()
    end)
    local host_name = string.format("%s.StatusNotifierHost-%d",
        prefix, ok and pid or 0)

    local subscriptions = {}

    -- Register the host name. Some applications check that it exists.
    Gio.bus_own_name_on_connection(connection, host_name,
        Gio.BusNameOwnerFlags.NONE, nil, nil)

    local function on_watcher_appeared(_, _, owner)
        -- Track the items registered with the watcher.
        table.insert(subscriptions, connection:signal_subscribe(
            owner, interface, nil, "/StatusNotifierWatcher", nil,
            Gio.DBusSignalFlags.NONE,
            function(_, _, _, _, signal_name, params)
                if signal_name == "StatusNotifierItemRegistered" then
                    protected_call(add_item, prefix, params.value[1])
                elseif signal_name == "StatusNotifierItemUnregistered" then
                    protected_call(remove_item, prefix, params.value[1])
                end
            end
        ))

        connection:call(
            owner, "/StatusNotifierWatcher", interface,
            "RegisterStatusNotifierHost", GLib.Variant("(s)", { host_name }),
            nil, Gio.DBusCallFlags.NONE, -1, nil,
            function(c, res)
                local _, err = c:call_finish(res)
                if err then
                    gdebug.print_warning("statusnotifier: Failed to register " ..
                        "the host: " .. tostring(err))
                end
            end
        )

        connection:call(
            owner, "/StatusNotifierWatcher", "org.freedesktop.DBus.Properties",
            "Get", GLib.Variant("(ss)", { interface, "RegisteredStatusNotifierItems" }),
            nil, Gio.DBusCallFlags.NONE, -1, nil,
            function(c, res)
                local ret = c:call_finish(res)
                if not ret then return end

                local list = ret:get_child_value(0):get_variant()

                for i = 0, list:n_children() - 1 do
                    protected_call(add_item, prefix, list:get_child_value(i).value)
                end
            end
        )
    end

    local function on_watcher_vanished()
        for _, id in ipairs(subscriptions) do
            connection:signal_unsubscribe(id)
        end
        subscriptions = {}
        clear_items(prefix)
    end

    -- When our own watcher does not own the name, another one does. In both
    -- cases, the host talks to the current owner over D-Bus.
    Gio.bus_watch_name_on_connection(connection, interface,
        Gio.BusNameWatcherFlags.NONE,
        GObject.Closure(function(...)
            protected_call(on_watcher_appeared, ...)
        end),
        GObject.Closure(function(...)
            protected_call(on_watcher_vanished, ...)
        end)
    )
end

--- Start the StatusNotifierWatcher and StatusNotifierHost.
--
-- This is called automatically by `wibox.widget.systray`. Calling it more
-- than once has no effect.
--
-- @staticfct wibox.widget.systray.statusnotifier.start
-- @noreturn
function module.start()
    if started then return end

    started = true

    Gio.bus_get(Gio.BusType.SESSION, nil, function(_, res)
        local conn, err = Gio.bus_get_finish(res)

        if not conn then
            gdebug.print_error("statusnotifier: Cannot connect to the session " ..
                "bus: " .. tostring(err))
            return
        end

        connection = conn

        for _, prefix in ipairs(prefixes) do
            watcher.start(prefix, connection)
            start_host(prefix)
        end
    end)
end

--- Get all items currently displayed.
-- @staticfct wibox.widget.systray.statusnotifier.get_items
-- @treturn table A list of `wibox.widget.systray.item`.
function module.get_items()
    local ret = {}

    for k, v in ipairs(items) do ret[k] = v end

    return ret
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
