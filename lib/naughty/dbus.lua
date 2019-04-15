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
local cairo, Gio, GLib, GObject = lgi.cairo, lgi.Gio, lgi.GLib, lgi.GObject

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

-- This is either nil or a Gio.DBusConnection for emitting signals
local bus_connection

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

local function sendActionInvoked(notificationId, action)
    if bus_connection then
        bus_connection:emit_signal(nil, "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications", "ActionInvoked",
        GLib.Variant("(us)", { notificationId, action }))
    end
end

local function sendNotificationClosed(notificationId, reason)
    if reason <= 0 then
        reason = cst.notification_closed_reason.undefined
    end
    if bus_connection then
        bus_connection:emit_signal(nil, "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications", "NotificationClosed",
        GLib.Variant("(uu)", { notificationId, reason }))
    end
end

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

local notif_methods = {}

function notif_methods.Notify(sender, object_path, interface, method, parameters, invocation)
    local appname, replaces_id, icon, title, text, actions, hints, expire =
        unpack(parameters.value)

    local args = {}
    if text ~= "" then
        args.message = text
        if title ~= "" then
            args.title = title
        end
    else
        if title ~= "" then
            args.message = title
        else
            -- FIXME: We have to reply *something* to the DBus invocation.
            -- Right now this leads to a memory leak, I think.
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
    local legacy_data = { -- This data used to be generated by AwesomeWM's C code
        type = "method_call", interface = interface, path = object_path,
        member = method, sender = sender, bus = "session"
    }
    if not preset.callback or (type(preset.callback) == "function" and
        preset.callback(legacy_data, appname, replaces_id, icon, title, text, actions, hints, expire)) then
        if icon ~= "" then
            args.icon = icon
        elseif hints.icon_data or hints.image_data then
            -- Icon data is a bit complex and hence needs special care:
            -- .value breaks with the array of bytes (ay) that we get here.
            -- So, bypass it and look up the needed value differently
            local icon_data
            for k, v in parameters:get_child_value(7 - 1):pairs() do
                if k == "icon_data" then
                    icon_data = v
                elseif k == "image_data" and icon_data == nil then
                    icon_data = v
                end
            end

            -- icon_data is an array:
            -- 1 -> width
            -- 2 -> height
            -- 3 -> rowstride
            -- 4 -> has alpha
            -- 5 -> bits per sample
            -- 6 -> channels
            -- 7 -> data

            -- Get the value as a GVariant and then use LGI's special
            -- GVariant.data to get that as an LGI byte buffer. That one can
            -- then by converted to a string via its __tostring metamethod.
            local data = tostring(icon_data:get_child_value(7 - 1).data)
            args.icon = convert_icon(icon_data[1], icon_data[2], icon_data[3], icon_data[6], data)
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
                if k == "destroy" then k = "destroy_cb" end
                notification[k] = v
            end
        else
            notification = nnotif(args)
        end

        invocation:return_value(GLib.Variant("(u)", { notification.id }))
        return
    end

    invocation:return_value(GLib.Variant("(u)", { nnotif._gen_next_id() }))
end

function notif_methods.CloseNotification(_, _, _, _, parameters, invocation)
    local obj = naughty.get_by_id(parameters.value[1])
    if obj then
        obj:destroy(cst.notification_closed_reason.dismissed_by_command)
    end
    invocation:return_value(GLib.Variant("()"))
end

function notif_methods.GetServerInformation(_, _, _, _, _, invocation)
    -- name of notification app, name of vender, version, specification version
    invocation:return_value(GLib.Variant("(ssss)", {
        "naughty", "awesome", capi.awesome.version, "1.0"
    }))
end

function notif_methods.GetCapabilities(_, _, _, _, _, invocation)
    -- We actually do display the body of the message, we support <b>, <i>
    -- and <u> in the body and we handle static (non-animated) icons.
    invocation:return_value(GLib.Variant("(as)", {
        { "body", "body-markup", "icon-static", "actions" }
    }))
end

local function method_call(_, sender, object_path, interface, method, parameters, invocation)
    if not notif_methods[method] then return end

    protected_call(
        notif_methods[method],
        sender,
        object_path,
        interface,
        method,
        parameters,
        invocation
    )
end

local function on_bus_acquire(conn, _)
    local function arg(name, signature)
        return Gio.DBusArgInfo{ name = name, signature = signature }
    end
    local method = Gio.DBusMethodInfo
    local signal = Gio.DBusSignalInfo

    local interface_info = Gio.DBusInterfaceInfo {
        name = "org.freedesktop.Notifications",
        methods = {
            method{ name = "GetCapabilities",
                out_args = { arg("caps", "as") }
            },
            method{ name = "CloseNotification",
                in_args = { arg("id", "u") }
            },
            method{ name = "GetServerInformation",
                out_args = { arg("return_name", "s"), arg("return_vendor", "s"),
                    arg("return_version", "s"), arg("return_spec_version", "s")
                }
            },
            method{ name = "Notify",
                in_args = { arg("app_name", "s"), arg("id", "u"),
                    arg("icon", "s"), arg("summary", "s"), arg("body", "s"),
                    arg("actions", "as"), arg("hints", "a{sv}"),
                    arg("timeout", "i")
                },
                out_args = { arg("return_id", "u") }
            }
        },
        signals = {
            signal{ name = "NotificationClosed",
                args = { arg("id", "u"), arg("reason", "u") }
            },
            signal{ name = "ActionInvoked",
                args = { arg("id", "u"), arg("action_key", "s") }
            }
        }
    }
    conn:register_object("/org/freedesktop/Notifications", interface_info,
        GObject.Closure(method_call))
end

local function on_name_acquired(conn, _)
    bus_connection = conn
end

local function on_name_lost(_, _)
    bus_connection = nil
end

Gio.bus_own_name(Gio.BusType.SESSION, "org.freedesktop.Notifications",
    Gio.BusNameOwnerFlags.NONE, GObject.Closure(on_bus_acquire),
    GObject.Closure(on_name_acquired), GObject.Closure(on_name_lost))

-- For testing
dbus._notif_methods = notif_methods

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
