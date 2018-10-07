---------------------------------------------------------------------------
-- DBUS/Notification support
-- Notify
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @copyright 2008 koniu
-- @module naughty.dbus
---------------------------------------------------------------------------

assert(dbus)

-- Package environment
local pairs = pairs
local type = type
local string = string
local capi = { awesome = awesome,
               dbus = dbus }
local gtable = require("gears.table")
local gsurface = require("gears.surface")
local cairo = require("lgi").cairo

local schar = string.char
local sbyte = string.byte
local tcat = table.concat
local tins = table.insert
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local naughty = require("naughty.core")

--- Notification library, dbus bindings
local dbus = { config = {} }

-- DBUS Notification constants
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
    {{urgency = urgency.low}, naughty.config.presets.low},
    {{urgency = urgency.normal}, naughty.config.presets.normal},
    {{urgency = urgency.critical}, naughty.config.presets.critical}
}

local function sendActionInvoked(notificationId, action)
    if capi.dbus then
        capi.dbus.emit_signal("session", "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications", "ActionInvoked",
        "u", notificationId,
        "s", action)
    end
end

local function sendNotificationClosed(notificationId, reason)
    if capi.dbus then
        capi.dbus.emit_signal("session", "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications", "NotificationClosed",
        "u", notificationId,
        "u", reason)
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

capi.dbus.connect_signal("org.freedesktop.Notifications",
    function (data, appname, replaces_id, icon, title, text, actions, hints, expire)
        local args = { }
        if data.member == "Notify" then
            if text ~= "" then
                args.text = text
                if title ~= "" then
                    args.title = title
                end
            else
                if title ~= "" then
                    args.text = title
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
            local preset = args.preset or naughty.config.defaults
            local notification
            if actions then
                args.actions = {}

                for i = 1,#actions,2 do
                    local action_id = actions[i]
                    local action_text = actions[i + 1]

                    if action_id == "default" then
                        args.run = function()
                            sendActionInvoked(notification.id, "default")
                            naughty.destroy(notification, naughty.notificationClosedReason.dismissedByUser)
                        end
                    elseif action_id ~= nil and action_text ~= nil then
                        args.actions[action_text] = function()
                            sendActionInvoked(notification.id, action_id)
                            naughty.destroy(notification, naughty.notificationClosedReason.dismissedByUser)
                        end
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
                notification = naughty.notify(args)
                if notification ~= nil then
                    return "u", notification.id
                end
            end
            return "u", naughty.get_next_notification_id()
        elseif data.member == "CloseNotification" then
            local obj = naughty.getById(appname)
            if obj then
            naughty.destroy(obj, naughty.notificationClosedReason.dismissedByCommand)
            end
        elseif data.member == "GetServerInfo" or data.member == "GetServerInformation" then
            -- name of notification app, name of vender, version, specification version
            return "s", "naughty", "s", "awesome", "s", capi.awesome.version, "s", "1.0"
        elseif data.member == "GetCapabilities" then
            -- We actually do display the body of the message, we support <b>, <i>
            -- and <u> in the body and we handle static (non-animated) icons.
            return "as", { "s", "body", "s", "body-markup", "s", "icon-static", "s", "actions" }
        end
end)

capi.dbus.connect_signal("org.freedesktop.DBus.Introspectable", function (data)
    if data.member == "Introspect" then
        local xml = [=[<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object
    Introspection 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
    <node>
      <interface name="org.freedesktop.DBus.Introspectable">
        <method name="Introspect">
          <arg name="data" direction="out" type="s"/>
        </method>
      </interface>
      <interface name="org.freedesktop.Notifications">
        <method name="GetCapabilities">
          <arg name="caps" type="as" direction="out"/>
        </method>
        <method name="CloseNotification">
          <arg name="id" type="u" direction="in"/>
        </method>
        <method name="Notify">
          <arg name="app_name" type="s" direction="in"/>
          <arg name="id" type="u" direction="in"/>
          <arg name="icon" type="s" direction="in"/>
          <arg name="summary" type="s" direction="in"/>
          <arg name="body" type="s" direction="in"/>
          <arg name="actions" type="as" direction="in"/>
          <arg name="hints" type="a{sv}" direction="in"/>
          <arg name="timeout" type="i" direction="in"/>
          <arg name="return_id" type="u" direction="out"/>
        </method>
        <method name="GetServerInformation">
          <arg name="return_name" type="s" direction="out"/>
          <arg name="return_vendor" type="s" direction="out"/>
          <arg name="return_version" type="s" direction="out"/>
          <arg name="return_spec_version" type="s" direction="out"/>
        </method>
        <method name="GetServerInfo">
          <arg name="return_name" type="s" direction="out"/>
          <arg name="return_vendor" type="s" direction="out"/>
          <arg name="return_version" type="s" direction="out"/>
       </method>
       <signal name="NotificationClosed">
          <arg name="id" type="u" direction="out"/>
          <arg name="reason" type="u" direction="out"/>
       </signal>
       <signal name="ActionInvoked">
          <arg name="id" type="u" direction="out"/>
          <arg name="action_key" type="s" direction="out"/>
       </signal>
      </interface>
    </node>]=]
        return "s", xml
    end
end)

-- listen for dbus notification requests
capi.dbus.request_name("session", "org.freedesktop.Notifications")

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
