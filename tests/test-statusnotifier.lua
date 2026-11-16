-- Test the StatusNotifierItem support of `wibox.widget.systray`.
--
-- A fake application is exported on a separate D-Bus connection (with its own
-- unique name) from within awesome itself.

local runner = require("_runner")
local wibox = require("wibox")
local beautiful = require("beautiful")
local lgi = require("lgi")
local Gio, GLib, GObject = lgi.Gio, lgi.GLib, lgi.GObject
local gdebug = require("gears.debug")
local statusnotifier = require("wibox.widget.systray.statusnotifier")
local dbusmenu = require("wibox.widget.systray.dbusmenu")

-- Record the warnings (they are still printed).
local warnings = {}
local print_warning = gdebug.print_warning

function gdebug.print_warning(message)
    table.insert(warnings, message)
    print_warning(message)
end

local function count_warnings(pattern)
    local n = 0
    for _, msg in ipairs(warnings) do
        if msg:find(pattern, 1, true) then n = n + 1 end
    end
    return n
end

-- The fake application -------------------------------------------------------

local function arg(name, signature)
    return Gio.DBusArgInfo { name = name, signature = signature }
end

local readable = Gio.DBusPropertyInfoFlags.READABLE

local item_info = Gio.DBusInterfaceInfo {
    name = "org.kde.StatusNotifierItem",
    methods = {
        Gio.DBusMethodInfo { name = "Activate",
            in_args = { arg("x", "i"), arg("y", "i") } },
        Gio.DBusMethodInfo { name = "SecondaryActivate",
            in_args = { arg("x", "i"), arg("y", "i") } },
        Gio.DBusMethodInfo { name = "Scroll",
            in_args = { arg("delta", "i"), arg("orientation", "s") } },
        Gio.DBusMethodInfo { name = "ContextMenu",
            in_args = { arg("x", "i"), arg("y", "i") } },
    },
    signals = {
        Gio.DBusSignalInfo { name = "NewTitle" },
        Gio.DBusSignalInfo { name = "NewStatus", args = { arg("status", "s") } },
    },
    properties = {
        Gio.DBusPropertyInfo { name = "Id", signature = "s", flags = readable },
        Gio.DBusPropertyInfo { name = "Title", signature = "s", flags = readable },
        Gio.DBusPropertyInfo { name = "Status", signature = "s", flags = readable },
        Gio.DBusPropertyInfo { name = "Category", signature = "s", flags = readable },
        Gio.DBusPropertyInfo { name = "IconName", signature = "s", flags = readable },
        Gio.DBusPropertyInfo { name = "IconPixmap", signature = "a(iiay)", flags = readable },
        Gio.DBusPropertyInfo { name = "ToolTip", signature = "(sa(iiay)ss)", flags = readable },
        Gio.DBusPropertyInfo { name = "Menu", signature = "o", flags = readable },
        Gio.DBusPropertyInfo { name = "ItemIsMenu", signature = "b", flags = readable },
    },
}

local menu_info = Gio.DBusInterfaceInfo {
    name = "com.canonical.dbusmenu",
    methods = {
        Gio.DBusMethodInfo { name = "GetLayout",
            in_args = { arg("parentId", "i"), arg("recursionDepth", "i"),
                arg("propertyNames", "as") },
            out_args = { arg("revision", "u"), arg("layout", "(ia{sv}av)") } },
        Gio.DBusMethodInfo { name = "Event",
            in_args = { arg("id", "i"), arg("eventId", "s"), arg("data", "v"),
                arg("timestamp", "u") } },
        Gio.DBusMethodInfo { name = "AboutToShow",
            in_args = { arg("id", "i") }, out_args = { arg("needUpdate", "b") } },
    },
}

local app = {
    title       = "Fake app",
    status      = "Active",
    calls       = {},
    events      = {},
    about_to_show = {},
}

-- A 2x1 pixmap: an opaque red pixel and a half transparent white one. The
-- data contains NUL bytes on purpose.
local pixmap_text = "[(2, 1, [byte 0xff, 0xff, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff])]"

local function item_property(name)
    if name == "Id" then return GLib.Variant("s", "fake-app")
    elseif name == "Title" then return GLib.Variant("s", app.title)
    elseif name == "Status" then return GLib.Variant("s", app.status)
    elseif name == "Category" then return GLib.Variant("s", "ApplicationStatus")
    elseif name == "IconName" then return GLib.Variant("s", "")
    elseif name == "IconPixmap" then
        return GLib.Variant.parse(GLib.VariantType("a(iiay)"), pixmap_text, nil, nil)
    elseif name == "ToolTip" then
        return GLib.Variant("(sa(iiay)ss)", { "", {}, "Tip title", "Some <b>text</b><br/>here" })
    elseif name == "Menu" then return GLib.Variant("o", "/MenuBar")
    elseif name == "ItemIsMenu" then return GLib.Variant("b", false)
    end
end

local function menu_node(id, props, children)
    local p = {}
    for k, v in pairs(props) do p[k] = v end
    return GLib.Variant("(ia{sv}av)", { id, p, children or {} })
end

local function menu_layout()
    return GLib.Variant("(u(ia{sv}av))", { 1, {
        0, {}, {
            -- A leading separator, which must be skipped.
            menu_node(1, { type = GLib.Variant("s", "separator") }),
            menu_node(2, { label = GLib.Variant("s", "_Open") }),
            menu_node(3, {
                label = GLib.Variant("s", "Toggle"),
                ["toggle-type"] = GLib.Variant("s", "checkmark"),
                ["toggle-state"] = GLib.Variant("i", 1),
            }),
            menu_node(4, { label = GLib.Variant("s", "Hidden"),
                visible = GLib.Variant("b", false) }),
            menu_node(5, { label = GLib.Variant("s", "Disabled"),
                enabled = GLib.Variant("b", false) }),
            menu_node(6, { type = GLib.Variant("s", "separator") }),
            menu_node(7, { label = GLib.Variant("s", "More"),
                ["children-display"] = GLib.Variant("s", "submenu") }, {
                menu_node(8, { label = GLib.Variant("s", "Sub item") }),
            }),
            menu_node(9, { label = GLib.Variant("s", "_Quit") }),
        }
    }})
end

local app_conn

local function start_app()
    local address = Gio.dbus_address_get_for_bus_sync(Gio.BusType.SESSION)

    app_conn = Gio.DBusConnection.new_for_address_sync(address,
        Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT +
        Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION, nil, nil)

    assert(app_conn, "could not create the application connection")

    app_conn:register_object("/StatusNotifierItem", item_info,
        GObject.Closure(function(_, _, _, _, method, params, invocation)
            table.insert(app.calls, { method = method, params = params.value })
            invocation:return_value(GLib.Variant("()"))
        end),
        GObject.Closure(function(_, _, _, _, name)
            return item_property(name)
        end),
        nil)

    app_conn:register_object("/MenuBar", menu_info,
        GObject.Closure(function(_, _, _, _, method, params, invocation)
            if method == "GetLayout" then
                invocation:return_value(menu_layout())
            elseif method == "AboutToShow" then
                table.insert(app.about_to_show, params.value[1])
                invocation:return_value(GLib.Variant("(b)", { false }))
            elseif method == "Event" then
                table.insert(app.events, {
                    id = params.value[1], event = params.value[2]
                })
                invocation:return_value(GLib.Variant("()"))
            end
        end),
        nil, nil)
end

local function register_app(path)
    app_conn:call("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
        GLib.Variant("(s)", { path or "/StatusNotifierItem" }), nil,
        Gio.DBusCallFlags.NONE, -1, nil, function(c, res)
            local _, err = c:call_finish(res)
            assert(not err, tostring(err))
        end)
end

-- A second application, registered using a well-known name instead of an
-- object path (like KDE applications do).
local named = { title = "Named app" }

local function start_named_app()
    local address = Gio.dbus_address_get_for_bus_sync(Gio.BusType.SESSION)

    named.conn = Gio.DBusConnection.new_for_address_sync(address,
        Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT +
        Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION, nil, nil)

    named.conn:register_object("/StatusNotifierItem", item_info,
        GObject.Closure(function(_, _, _, _, _, _, invocation)
            invocation:return_value(GLib.Variant("()"))
        end),
        GObject.Closure(function(_, _, _, _, name)
            if name == "Id" then return GLib.Variant("s", "named-app") end
            if name == "Title" then return GLib.Variant("s", named.title) end
            if name == "Menu" then return GLib.Variant("o", "/") end
            if name == "IconPixmap" then return GLib.Variant("a(iiay)", {}) end
            if name == "ToolTip" then
                return GLib.Variant("(sa(iiay)ss)", { "", {}, "", "" })
            end
            if name == "ItemIsMenu" then return GLib.Variant("b", false) end
            return GLib.Variant("s", "")
        end),
        nil)

    named.name = "org.kde.StatusNotifierItem-4242-1"

    local ret = named.conn:call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "RequestName",
        GLib.Variant("(su)", { named.name, 0 }), nil, Gio.DBusCallFlags.NONE, -1)
    assert(ret and ret.value[1] == 1, "could not own the name")

    named.conn:call("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
        GLib.Variant("(s)", { named.name }), nil,
        Gio.DBusCallFlags.NONE, -1, nil, function(c, res)
            local _, err = c:call_finish(res)
            assert(not err, tostring(err))
        end)
end

local function watcher_property(name, callback)
    app_conn:call("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
        "org.freedesktop.DBus.Properties", "Get",
        GLib.Variant("(ss)", { "org.kde.StatusNotifierWatcher", name }), nil,
        Gio.DBusCallFlags.NONE, -1, nil, function(c, res)
            local ret = c:call_finish(res)
            callback(ret and ret:get_child_value(0):get_variant().value)
        end)
end

-- The test ---------------------------------------------------------------------

local tray = wibox.widget.systray()

local w = wibox {
    x = 0, y = 0, width = 200, height = 20, visible = true,
    widget = tray,
}

-- The number of displayed StatusNotifierItems.
local function count_icons()
    local ctx = { screen = screen.primary, wibox = w }
    local width = tray:fit(ctx, 200, 20)
    local placements = tray:layout(ctx, width, 20)

    -- Each icon uses the full height.
    for _, p in ipairs(placements) do
        assert(p._height == 20, p._height)
    end

    return #placements, width
end

local function click(button)
    -- Click on the first icon.
    mouse.coords { x = 10, y = 10 }
    root.fake_input("button_press", button)
    root.fake_input("button_release", button)
end

local function key(name)
    root.fake_input("key_press", name)
    root.fake_input("key_release", name)
end

local added, removed, item = 0, 0, nil

statusnotifier:connect_signal("item::added", function(_, i)
    added = added + 1
    item = i
end)

statusnotifier:connect_signal("item::removed", function(_, i)
    if i == item then
        removed = removed + 1
    end
end)

local host_registered, menu, bogus_registered_at, bogus_warned_at

local steps = {
    -- Wait for our watcher to be up, then start the application.
    function()
        local done = false
        local conn = Gio.bus_get_sync(Gio.BusType.SESSION)
        local owner = conn:call_sync("org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "NameHasOwner",
            GLib.Variant("(s)", { "org.kde.StatusNotifierWatcher" }), nil,
            Gio.DBusCallFlags.NONE, -1)

        if owner and owner.value[1] then
            start_app()
            register_app()
            done = true
        end

        return done or nil
    end,

    -- The item shows up.
    function()
        if added == 0 then return end

        assert(added == 1)
        assert(#statusnotifier.get_items() == 1)
        assert(item.id == "fake-app", tostring(item.id))
        assert(item.title == "Fake app")
        assert(item.status == "Active")
        assert(item.menu_path == "/MenuBar")
        assert(item.item_is_menu == false)
        assert(item.path == "/StatusNotifierItem")
        assert(item.service:sub(1, 1) == ":")
        assert(item.tooltip.title == "Tip title")

        -- The pixmap was correctly converted (and NUL bytes were kept).
        local ic = item:get_icon(16)
        assert(ic and ic.surface, "no icon")
        assert(ic.surface.width == 2 and ic.surface.height == 1)

        -- The widget displays it.
        local n, width = count_icons()
        assert(n == 1)
        assert(width == 20, width)

        watcher_property("IsStatusNotifierHostRegistered", function(v)
            host_registered = v
        end)

        return true
    end,

    -- The watcher sees the host.
    function()
        if host_registered == nil then return end
        assert(host_registered == true)
        return true
    end,

    -- An item whose object does not exist is never displayed. The initial
    -- load is retried a few times, then a single warning is printed.
    function(count)
        if count == 1 then
            bogus_registered_at = GLib.get_monotonic_time()
            register_app("/NoSuchItem")
        end

        local n = count_warnings("/NoSuchItem")

        if n == 0 then
            assert(#statusnotifier.get_items() == 1)
            return
        end

        if not bogus_warned_at then
            -- The load was retried (about one second apart) before giving up.
            local elapsed = (GLib.get_monotonic_time() - bogus_registered_at) / 1e6
            print(string.format("Bogus item ignored after %.2fs", elapsed))
            assert(elapsed > 1.5, elapsed)
            bogus_warned_at = count
        end

        -- Wait a bit more to check that nothing else happens.
        if count < bogus_warned_at + 15 then return end

        assert(n == 1, n)
        assert(added == 1, added)
        assert(#statusnotifier.get_items() == 1)
        assert(statusnotifier.get_items()[1] == item)
        assert(count_icons() == 1)

        return true
    end,

    -- Clicks are forwarded.
    function(count)
        if count == 1 then
            click(1)
            click(2)
            click(4)
        end

        if #app.calls < 3 then return end

        assert(app.calls[1].method == "Activate")
        assert(app.calls[1].params[1] == 10 and app.calls[1].params[2] == 10)
        assert(app.calls[2].method == "SecondaryActivate")
        assert(app.calls[3].method == "Scroll")
        assert(app.calls[3].params[1] == -1)
        assert(app.calls[3].params[2] == "vertical")

        return true
    end,

    -- Property updates are followed.
    function(count)
        if count == 1 then
            app.title = "New title"
            app_conn:emit_signal(nil, "/StatusNotifierItem",
                "org.kde.StatusNotifierItem", "NewTitle", nil)
        end

        if item.title ~= "New title" then return end

        return true
    end,

    -- Passive items can be hidden.
    function(count)
        if count == 1 then
            beautiful.systray_show_passive = false
            app.status = "Passive"
            app_conn:emit_signal(nil, "/StatusNotifierItem",
                "org.kde.StatusNotifierItem", "NewStatus",
                GLib.Variant("(s)", { "Passive" }))
        end

        if item.status ~= "Passive" then return end

        assert(count_icons() == 0)
        beautiful.systray_show_passive = nil
        wibox.widget.systray.refresh_icons()
        assert(count_icons() == 1)

        return true
    end,

    -- The menu is fetched and displayed when right clicking.
    function(count)
        if count == 1 then
            item:get_menu():connect_signal("shown", function(_, m) menu = m end)
            click(3)
        end

        if not dbusmenu.is_supported() then
            -- Without GTK, the application is asked to show its menu.
            local last = app.calls[#app.calls]
            if last.method ~= "ContextMenu" then return end
            return true
        end

        if not menu then return end

        assert(app.about_to_show[1] == 0)
        assert(menu:get_visible())

        -- Open, Toggle, Disabled, separator, More, Quit
        local children = menu:get_children()
        assert(#children == 6, #children)
        assert(children[1]:get_label() == "_Open")
        assert(children[1]:get_use_underline())
        assert(children[2]:get_active() == true) -- checkmark
        assert(not children[3]:get_sensitive()) -- disabled
        assert(children[4]:get_label() == nil or children[4]:get_label() == "")
        assert(children[5]:get_submenu()) -- submenu
        assert(children[6]:get_label() == "_Quit")

        return true
    end,

    -- The menu stays open after the button release and supports the
    -- keyboard. Select "Open".
    function(count)
        if not dbusmenu.is_supported() then return true end

        if count == 1 then
            key("Down")
            key("Return")
        end

        local clicked

        for _, e in ipairs(app.events) do
            if e.event == "clicked" then clicked = e end
        end

        if not clicked then return end

        assert(clicked.id == 2, tostring(clicked.id))
        assert(app.events[1].id == 0 and app.events[1].event == "opened")

        return true
    end,

    -- The menu is closed and destroyed.
    function()
        if not dbusmenu.is_supported() then return true end

        if item:get_menu():is_visible() then return end

        local closed = false

        for _, e in ipairs(app.events) do
            if e.id == 0 and e.event == "closed" then closed = true end
        end

        if not closed then return end

        return true
    end,

    -- The item goes away with the application.
    function(count)
        if count == 1 then
            app_conn:close_sync(nil)
        end

        if removed == 0 then return end

        assert(#statusnotifier.get_items() == 0)
        assert(count_icons() == 0)

        return true
    end,

    -- An item registered with a well-known name.
    function(count)
        if count == 1 then
            start_named_app()
        end

        named.item = statusnotifier.get_items()[1]

        if not named.item then return end

        assert(named.item.service == named.name, named.item.service)
        assert(named.item.title == "Named app")
        assert(named.item:get_menu() == nil)

        -- Without a usable icon, a fallback is used, so the item is visible.
        assert(count_icons() == 1)

        return true
    end,

    -- Signals are received even though they come from the unique name.
    function(count)
        if count == 1 then
            named.title = "Renamed"
            named.conn:emit_signal(nil, "/StatusNotifierItem",
                "org.kde.StatusNotifierItem", "NewTitle", nil)
        end

        if named.item.title ~= "Renamed" then return end

        return true
    end,

    -- Releasing the name removes the item.
    function(count)
        if count == 1 then
            named.conn:close_sync(nil)
        end

        if #statusnotifier.get_items() > 0 then return end

        assert(count_icons() == 0)

        return true
    end,
}

-- Leave enough time for the initial load retries of the bogus item.
runner.run_steps(steps, { wait_per_step = 5 })

-- Keep a reference to the wibox.
assert(w)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
