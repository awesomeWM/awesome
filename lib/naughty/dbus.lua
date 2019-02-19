---------------------------------------------------------------------------
-- DBUS/Notification support
-- Notify
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @copyright 2008 koniu
-- @module naughty.dbus
---------------------------------------------------------------------------

-- Package environment
local pairs = pairs
local type = type
local string = string
local capi = { awesome = awesome }
local gtable = require("gears.table")
local gsurface = require("gears.surface")
local protected_call = require("gears.protected_call")
local lgi = require("lgi")
local cairo = lgi.cairo
local Gio = lgi.Gio
local GLib = lgi.GLib
local GObject = lgi.GObject

local schar = string.char
local sbyte = string.byte
local tcat = table.concat
local tins = table.insert
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local naughty = require("naughty.core")
local cst     = require("naughty.constants")
local nnotif = require("naughty.notification")
local naction = require("naughty.action")

--- Notification library, dbus bindings
local dbus = { config = {} }

-- DBUS Notification constants
-- https://developer.gnome.org/notification-spec/#urgency-levels
local urgency = {
    low = "\0",
    normal = "\1",
    critical = "\2"
}

--- DBUS notification to preset mapping.
-- The first element is an object containing the filter.
-- If the rules in the filter match, the associated preset will be applied.
-- The rules object can contain the following keys: urgency, category, appname.
-- The second element is the preset.
-- @tfield table 1 low urgency
-- @tfield table 2 normal urgency
-- @tfield table 3 critical urgency
-- @table config.mapping
dbus.config.mapping = {
    {{urgency = urgency.low}, cst.config.presets.low},
    {{urgency = urgency.normal}, cst.config.presets.normal},
    {{urgency = urgency.critical}, cst.config.presets.critical}
}

local bus_connection
local function sendActionInvoked(notificationId, action)
    if bus_connection then
        bus_connection:emit_signal(nil, "/org/freedesktop/Notifications",
            "org.freedesktop.Notifications", "ActionInvoked",
            GLib.Variant("(us)", { notificationId, action }))
    end
end

local function sendNotificationClosed(notificationId, reason)
    if bus_connection and reason >= 0 then
        bus_connection:emit_signal(nil, "/org/freedesktop/Notifications",
            "org.freedesktop.Notifications", "NotificationClosed",
            GLib.Variant("(uu)", { notificationId, reason }))
    end
end

-- This allow notification to be upadated later.
local counter = 1

local function convert_icon(w, h, rowstride, channels, data)
    -- Do the arguments look sane? (e.g. we have enough data)
    local expected_length = rowstride * (h - 1) + w * channels
    if w < 0 or h < 0 or rowstride < 0 or (channels ~= 3 and channels ~= 4) or
        string.len(data) < expected_length then
        w = 0
        h = 0
    end

    local format = cairo.Format[channels == 4 and 'ARGB32' or 'RGB24']

    -- Figure out some stride magic (cairo dictates rowstride)
    local stride = cairo.Format.stride_for_width(format, w)
    local append = schar(0):rep(stride - 4 * w)
    local offset = 0

    -- Now convert each row on its own
    local rows = {}

    for _ = 1, h do
        local this_row = {}

        for i = 1 + offset, w * channels + offset, channels do
            local R, G, B, A = sbyte(data, i, i + channels - 1)
            tins(this_row, schar(B, G, R, A or 255))
        end

        -- Handle rowstride, offset is stride for the input, append for output
        tins(this_row, append)
        tins(rows, tcat(this_row))

        offset = offset + rowstride
    end

    local pixels = tcat(rows)
    local surf = cairo.ImageSurface.create_for_data(pixels, format, w, h, stride)

    -- The surface refers to 'pixels', which can be freed by the GC. Thus,
    -- duplicate the surface to create a copy of the data owned by cairo.
    local res = gsurface.duplicate_surface(surf)
    surf:finish()
    return res
end

local function protected_method_call(_conn, _sender, _obj, _interface, method, parameters, invocation)
    if method == "Notify" then
        local appname, replaces_id, icon, title, text, actions, hints, expire =
            unpack(parameters.value)

        local args = { }
        if text ~= "" then
            args.message = text
            if title ~= "" then
                args.title = title
            end
        else
            if title ~= "" then
                args.message = title
            else
                return
            end
        end
        if appname ~= "" then
            args.appname = appname
        end
        for _, obj in pairs(dbus.config.mapping) do
            local filter, preset = obj[1], obj[2]
            if (not filter.urgency or filter.urgency == hints.urgency) and
            (not filter.category or filter.category == hints.category) and
            (not filter.appname or filter.appname == appname) then
                args.preset = gtable.join(args.preset, preset)
            end
        end
        local preset = args.preset or cst.config.defaults
        local notification
        if actions then
            args.actions = {}

            for i = 1,#actions,2 do
                local action_id = actions[i]
                local action_text = actions[i + 1]

                if action_id == "default" then
                    args.run = function()
                        sendActionInvoked(notification.id, "default")
                        notification:destroy(cst.notification_closed_reason.dismissed_by_user)
                    end
                elseif action_id ~= nil and action_text ~= nil then
                    local a = naction {
                        name     = action_text,
                        position = action_id,
                    }

                    a:connect_signal("invoked", function()
                        sendActionInvoked(notification.id, action_id)
                        notification:destroy(cst.notification_closed_reason.dismissed_by_user)
                    end)

                    table.insert(args.actions, a)
                end
            end
        end
        args.destroy = function(reason)
            sendNotificationClosed(notification.id, reason)
        end
        if not preset.callback or (type(preset.callback) == "function" and
            preset.callback(data, appname, replaces_id, icon, title, text, actions, hints, expire)) then
            if icon ~= "" then
                args.icon = icon
            elseif hints.icon_data or hints.image_data then
                if hints.icon_data == nil then hints.icon_data = hints.image_data end

                -- icon_data is an array:
                -- 1 -> width
                -- 2 -> height
                -- 3 -> rowstride
                -- 4 -> has alpha
                -- 5 -> bits per sample
                -- 6 -> channels
                -- 7 -> data
                local w, h, rowstride, _, _, channels, icon_data = unpack(hints.icon_data)
                args.icon = convert_icon(w, h, rowstride, channels, icon_data)
            end
            if replaces_id and replaces_id ~= "" and replaces_id ~= 0 then
                args.replaces_id = replaces_id
            end
            if expire and expire > -1 then
                args.timeout = expire / 1000
            end
            args.freedesktop_hints = hints

            -- Try to update existing objects when possible
            notification = naughty.get_by_id(replaces_id)

            if notification then
                for k, v in pairs(args) do
                    notification[k] = v
                end
            else
                counter = counter+1
                args.id = counter
                notification = nnotif(args)
            end

            invocation:return_value(GLib.Variant("(u)", { notification.id }))
        end
        counter = counter+1
        invocation:return_value(GLib.Variant("(u)", { counter }))
    elseif method == "CloseNotification" then
        local obj = naughty.get_by_id(parameters.value[1])
        if obj then
            obj:destroy(cst.notification_closed_reason.dismissed_by_command)
        end
        invocation:return_value(GLib.Variant("()"))
    elseif method == "GetServerInformation" then
        -- name of notification app, name of vender, version, specification version
        invocation:return_value(GLib.Variant("(ssss)", {
            "naughty", "awesome", capi.awesome.version, "1.0"
        }))
    elseif method == "GetServerInfo" then
        -- name of notification app, name of vender, version
        invocation:return_value(GLib.Variant("(sss)", {
            "naughty", "awesome", capi.awesome.version
        }))
    elseif method == "GetCapabilities" then
        -- We actually do display the body of the message, we support <b>, <i>
        -- and <u> in the body and we handle static (non-animated) icons.
        invocation:return_value(GLib.Variant("(as)", {
            { "body", "body-markup", "icon-static", "actions" }
        }))
    end
end

local function new_arg(name, signature)
    local result = Gio.DBusArgInfo()
    --result.ref_count = 1
    result.name = name
    result.signature = signature
    return result
end

local function new_method(name, in_args, out_args)
    local result = Gio.DBusMethodInfo()
    --result.ref_count = 1
    result.name = name
    result.in_args = in_args
    result.out_args = out_args
    return result
end

local function new_signal(name, args)
    local result = Gio.DBusSignalInfo()
    --result.ref_count = 1
    result.name = name
    result.args = args
    return result
end

local get_capabilities_method =
    new_method("GetCapabilities", nil, { new_arg("caps", "as") })
local close_notification_method =
    new_method("CloseNotification", { new_arg("id", "u") }, nil)
local get_server_information_method = new_method("GetServerInformation", nil, {
    new_arg("return_name", "s"), new_arg("return_vendor", "s"),
    new_arg("return_version", "s"), new_arg("return_spec_version", "s")
})
local get_server_info_method = new_method("GetServerInfo", nil, {
    new_arg("return_name", "s"), new_arg("return_vendor", "s"),
    new_arg("return_version", "s")
})
local notify_method = new_method("Notify", {
    new_arg("app_name", "s"), new_arg("id", "u"), new_arg("icon", "s"),
    new_arg("summary", "s"), new_arg("body", "s"), new_arg("actions", "as"),
    new_arg("hints", "a{sv}"), new_arg("timeout", "i") },
    { new_arg("return_id", "u") })
local notification_closed_signal = new_signal("NotificationClosed",
    { new_arg("id", "u"), new_arg("reason", "u") })
local action_invoked_signal = new_signal("ActionInvoked",
    { new_arg("id", "u"), new_arg("action_key", "s") })

local interface_info = Gio.DBusInterfaceInfo()
--interface_info.ref_count = 1
interface_info.name = "org.freedesktop.Notifications"
interface_info.methods = { get_capabilities_method, close_notification_method,
    notify_method, get_server_information_method, get_server_info_method }
interface_info.signals = { notification_closed_signal, action_invoked_signal }

local function method_call(...)
    protected_call(protected_method_call, ...)
end

local function on_bus_acquire(conn, _name)
    bus_connection = conn
    conn:register_object("/org/freedesktop/Notifications", interface_info, GObject.Closure(method_call))
end

-- listen for dbus notification requests
Gio.bus_own_name(Gio.BusType.SESSION, "org.freedesktop.Notifications",
    Gio.BusNameOwnerFlags.NONE, GObject.Closure(on_bus_acquire), nil, nil)

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
