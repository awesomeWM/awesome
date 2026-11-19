---------------------------------------------------------------------------
-- A StatusNotifierWatcher D-Bus service.
--
-- The watcher is the registry where StatusNotifierItems (the tray icons) and
-- StatusNotifierHosts (the "trays" displaying them) register themselves.
-- Only one watcher can exist on the session bus. When another watcher (for
-- example `snixembed` or another panel) already owns the name, awesome waits
-- in the queue and the host simply uses the other watcher.
--
-- Both the `org.kde` and the `org.freedesktop` flavors of the protocol are
-- supported. In practice, almost all applications use the KDE one.
--
-- Many applications (Chromium and Electron based ones in particular) only
-- register their item once, when they start. So that their icons survive an
-- `awesome.restart`, the watcher adopts the items which are already on the bus
-- when it acquires its name:
--
-- * the items registered with the previous awesome instance, which saves them
--   in `$XDG_RUNTIME_DIR` when restarting;
-- * the items owning a well-known `<prefix>.StatusNotifierItem-*` name.
--
-- @author WooParadog
-- @copyright 2026 WooParadog
-- @module wibox.widget.systray.watcher
---------------------------------------------------------------------------

local lgi = require("lgi")
local Gio, GLib, GObject = lgi.Gio, lgi.GLib, lgi.GObject
local gdebug = require("gears.debug")
local protected_call = require("gears.protected_call")

local capi = { awesome = awesome }

local module = {}

local watchers = {}

local OBJECT_PATH = "/StatusNotifierWatcher"

-- How long to wait for an existing item to answer before ignoring it.
local ADOPT_TIMEOUT = 2000

-- Where the items are saved across `awesome.restart`.
local function state_file(prefix)
    local display = (os.getenv("DISPLAY") or ""):gsub("[^%w]", "_")

    return GLib.build_filenamev {
        GLib.get_user_runtime_dir(),
        "awesome-sni-" .. display .. "-" .. prefix,
    }
end

local function split_item(item)
    local slash = item:find("/", 1, true)

    if not slash then return item, "/StatusNotifierItem" end

    return item:sub(1, slash - 1), item:sub(slash)
end

local function make_interface_info(prefix)
    local function arg(name, signature)
        return Gio.DBusArgInfo { name = name, signature = signature }
    end

    local method, signal, property = Gio.DBusMethodInfo, Gio.DBusSignalInfo,
        Gio.DBusPropertyInfo
    local readable = Gio.DBusPropertyInfoFlags.READABLE

    return Gio.DBusInterfaceInfo {
        name = prefix .. ".StatusNotifierWatcher",
        methods = {
            method { name = "RegisterStatusNotifierItem",
                in_args = { arg("service", "s") } },
            method { name = "RegisterStatusNotifierHost",
                in_args = { arg("service", "s") } },
        },
        signals = {
            signal { name = "StatusNotifierItemRegistered",
                args = { arg("service", "s") } },
            signal { name = "StatusNotifierItemUnregistered",
                args = { arg("service", "s") } },
            signal { name = "StatusNotifierHostRegistered" },
            signal { name = "StatusNotifierHostUnregistered" },
        },
        properties = {
            property { name = "RegisteredStatusNotifierItems",
                signature = "as", flags = readable },
            property { name = "IsStatusNotifierHostRegistered",
                signature = "b", flags = readable },
            property { name = "ProtocolVersion",
                signature = "i", flags = readable },
        },
    }
end

local function new_watcher(prefix, connection)
    local self = {
        prefix     = prefix,
        connection = connection,
        interface  = prefix .. ".StatusNotifierWatcher",
        items      = {}, -- Ordered list of "service/path" strings.
        hosts      = {}, -- service -> true
        name_watch = {}, -- bus name -> { watch_id, count }
    }

    local function emit(name, params)
        if not self.owned then return end

        self.connection:emit_signal(
            nil, OBJECT_PATH, self.interface, name, params
        )
    end

    local function emit_properties_changed(props)
        if not self.owned then return end

        local changed = {}

        for _, p in ipairs(props) do
            if p == "RegisteredStatusNotifierItems" then
                changed[p] = GLib.Variant("as", self.items)
            elseif p == "IsStatusNotifierHostRegistered" then
                changed[p] = GLib.Variant("b", next(self.hosts) ~= nil)
            end
        end

        self.connection:emit_signal(
            nil, OBJECT_PATH, "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            GLib.Variant("(sa{sv}as)", { self.interface, changed, {} })
        )
    end

    -- Call `on_vanish` once `name` leaves the bus.
    local function watch_name(name, on_vanish)
        if self.name_watch[name] then
            table.insert(self.name_watch[name].callbacks, on_vanish)
            return
        end

        local entry = { callbacks = { on_vanish } }
        self.name_watch[name] = entry

        entry.id = Gio.bus_watch_name_on_connection(
            self.connection, name, Gio.BusNameWatcherFlags.NONE,
            nil,
            GObject.Closure(function()
                -- Avoid unwatching from within the callback.
                GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, function()
                    if self.name_watch[name] == entry then
                        self.name_watch[name] = nil
                        Gio.bus_unwatch_name(entry.id)
                        for _, cb in ipairs(entry.callbacks) do
                            protected_call(cb)
                        end
                    end
                    return false
                end)
            end)
        )
    end

    local function remove_item(item)
        for k, v in ipairs(self.items) do
            if v == item then
                table.remove(self.items, k)
                emit("StatusNotifierItemUnregistered", GLib.Variant("(s)", { item }))
                emit_properties_changed { "RegisteredStatusNotifierItems" }
                return
            end
        end
    end

    local function add_item(service, path)
        local item = service .. path

        for _, v in ipairs(self.items) do
            if v == item then return end
        end

        table.insert(self.items, item)

        -- If the name is already gone, the item is removed right away.
        watch_name(service, function() remove_item(item) end)

        emit("StatusNotifierItemRegistered", GLib.Variant("(s)", { item }))
        emit_properties_changed { "RegisteredStatusNotifierItems" }
    end

    local function call_bus(method, params, callback)
        self.connection:call(
            "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", method, params, nil,
            Gio.DBusCallFlags.NONE, -1, nil,
            function(c, res)
                local ret = c:call_finish(res)
                if ret then protected_call(callback, ret.value[1]) end
            end
        )
    end

    -- Add an existing item, if it really exports a StatusNotifierItem, then
    -- call `on_done` (whether it was added or not).
    local function adopt(service, path, on_done)
        self.connection:call(
            service, path, "org.freedesktop.DBus.Properties", "Get",
            GLib.Variant("(ss)", { self.prefix .. ".StatusNotifierItem", "Id" }),
            nil, Gio.DBusCallFlags.NO_AUTO_START, ADOPT_TIMEOUT, nil,
            function(c, res)
                local ret = c:call_finish(res)
                if ret and self.owned then
                    protected_call(add_item, service, path)
                end
                if on_done then protected_call(on_done) end
            end
        )
    end

    -- Is the application owning `unique_name` already registered?
    local function has_owner(unique_name)
        for _, v in ipairs(self.items) do
            if v:sub(1, #unique_name + 1) == unique_name .. "/" then
                return true
            end
        end

        return false
    end

    -- The items owning a well-known name, unless their application is
    -- already registered using its unique name.
    local function adopt_named()
        local name_prefix = self.prefix .. ".StatusNotifierItem-"

        call_bus("ListNames", nil, function(names)
            for _, name in ipairs(names) do
                if name:sub(1, #name_prefix) == name_prefix then
                    call_bus("GetNameOwner", GLib.Variant("(s)", { name }),
                        function(owner)
                            if not has_owner(owner) then
                                adopt(name, "/StatusNotifierItem")
                            end
                        end)
                end
            end
        end)
    end

    -- Adopt the items which already exist on the bus.
    local function adopt_existing()
        -- The items of the previous awesome instance come first: they have
        -- the right object path and keep their order.
        local saved = {}
        local file = state_file(self.prefix)
        local f = io.open(file, "r")

        if f then
            for line in f:lines() do
                if line ~= "" then table.insert(saved, line) end
            end

            f:close()
            os.remove(file)
        end

        -- Adopt them one at a time, to keep the order.
        local function next_saved(i)
            if not saved[i] then return adopt_named() end

            local service, path = split_item(saved[i])
            adopt(service, path, function() next_saved(i + 1) end)
        end

        next_saved(1)
    end

    self.adopt_existing = adopt_existing

    -- Save the items for the next instance when restarting.
    capi.awesome.connect_signal("exit", function(restart)
        local file = state_file(self.prefix)

        if not (restart and self.owned and #self.items > 0) then
            os.remove(file)
            return
        end

        local f = io.open(file, "w")

        if f then
            f:write(table.concat(self.items, "\n"), "\n")
            f:close()
        end
    end)

    local methods = {}

    function methods.RegisterStatusNotifierItem(sender, params, invocation)
        local arg_ = params.value[1] or ""
        local service, path

        -- The argument is either a bus name or an object path. In the latter
        -- case (used by libappindicator), the sender is the bus name.
        if arg_:sub(1, 1) == "/" then
            service, path = sender, arg_
        elseif arg_ ~= "" then
            service, path = arg_, "/StatusNotifierItem"
        else
            service, path = sender, "/StatusNotifierItem"
        end

        invocation:return_value(GLib.Variant("()"))

        add_item(service, path)
    end

    function methods.RegisterStatusNotifierHost(sender, params, invocation)
        local service = params.value[1]

        if (not service) or service == "" or service:sub(1, 1) == "/" then
            service = sender
        end

        invocation:return_value(GLib.Variant("()"))

        if self.hosts[service] then return end

        local had_host = next(self.hosts) ~= nil
        self.hosts[service] = true

        watch_name(service, function()
            self.hosts[service] = nil
            emit("StatusNotifierHostUnregistered", nil)
            emit_properties_changed { "IsStatusNotifierHostRegistered" }
        end)

        emit("StatusNotifierHostRegistered", nil)

        if not had_host then
            emit_properties_changed { "IsStatusNotifierHostRegistered" }
        end
    end

    local function method_call(_, sender, _, _, method, params, invocation)
        if not methods[method] then
            invocation:return_dbus_error(
                "org.freedesktop.DBus.Error.UnknownMethod",
                "Unknown method " .. tostring(method)
            )
            return
        end

        protected_call(methods[method], sender, params, invocation)
    end

    local function get_property(_, _, _, _, name)
        if name == "RegisteredStatusNotifierItems" then
            return GLib.Variant("as", self.items)
        elseif name == "IsStatusNotifierHostRegistered" then
            return GLib.Variant("b", next(self.hosts) ~= nil)
        elseif name == "ProtocolVersion" then
            return GLib.Variant("i", 0)
        end
    end

    -- Export the object before requesting the name, so that it is ready as
    -- soon as the name is visible on the bus. The object stays exported on
    -- our unique name even when the well-known name is owned by someone
    -- else, which is harmless.
    self.registration_id = connection:register_object(
        OBJECT_PATH,
        make_interface_info(prefix),
        GObject.Closure(method_call),
        GObject.Closure(get_property),
        nil
    )

    -- When another watcher owns the name, we wait in the queue.
    self.own_id = Gio.bus_own_name_on_connection(
        connection,
        self.interface,
        Gio.BusNameOwnerFlags.NONE,
        GObject.Closure(function()
            self.owned = true
            protected_call(adopt_existing)
        end),
        GObject.Closure(function()
            if self.owned then
                gdebug.print_warning(
                    "statusnotifier: Lost the " .. self.interface .. " name"
                )
            end
            self.owned = false

            -- Forget about everything. Items will register with the new
            -- owner on their own.
            for _, entry in pairs(self.name_watch) do
                Gio.bus_unwatch_name(entry.id)
            end

            self.items, self.hosts, self.name_watch = {}, {}, {}
        end)
    )

    return self
end

--- Start the watcher for a protocol prefix (`org.kde` or `org.freedesktop`).
-- @tparam string prefix The protocol prefix.
-- @tparam Gio.DBusConnection connection The session bus connection.
-- @noreturn
function module.start(prefix, connection)
    if watchers[prefix] then return end

    watchers[prefix] = new_watcher(prefix, connection)
end

-- Adopt the existing items again (for the tests).
function module._adopt_existing(prefix)
    local w = watchers[prefix]
    if w and w.owned then w.adopt_existing() end
end

-- The file where the items are saved across restarts (for the tests).
module._state_file = state_file

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
