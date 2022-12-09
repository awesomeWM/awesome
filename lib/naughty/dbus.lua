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
local gsurface = require("gears.surface")
local gdebug  = require("gears.debug")
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

local capabilities = {
    "body", "body-markup", "icon-static", "actions", "action-icons"
}

--- Notification library, dbus bindings
local dbus = { config = {} }

-- This is either nil or a Gio.DBusConnection for emitting signals
local bus_connection

-- DBUS Notification constants
-- https://developer-old.gnome.org/notification-spec/#urgency-levels
local urgency = {
    low      = "\0",
    normal   = "\1",
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
dbus.config.mapping = cst.config.mapping

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
    local appname, replaces_id, app_icon, title, text, actions, hints, expire =
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
        args.appname  = appname --TODO v6 Remove this.
        args.app_name = appname
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
                    id       = action_id,
                    position = (i - 1)/2 + 1,
                }

                -- Right now `gears` doesn't have a great icon implementation
                -- and `naughty` doesn't depend on `menubar`, so delegate the
                -- icon "somewhere" using a request.
                if hints["action-icons"] and action_id ~= "" then
                    naughty.emit_signal("request::action_icon", a, "dbus", {id = action_id})
                end

                a:connect_signal("invoked", function()
                    sendActionInvoked(notification.id, action_id)

                    if not notification.resident then
                        notification:destroy(cst.notification_closed_reason.dismissed_by_user)
                    end
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
        preset.callback(legacy_data, appname, replaces_id, app_icon, title, text, actions, hints, expire)) then


        if app_icon ~= "" then
            args.app_icon = app_icon
        end

        if hints.icon_data or hints.image_data or hints["image-data"] then
            -- Icon data is a bit complex and hence needs special care:
            -- .value breaks with the array of bytes (ay) that we get here.
            -- So, bypass it and look up the needed value differently
            local icon_condidates = {}
            for k, v in parameters:get_child_value(7 - 1):pairs() do
                if k == "image-data" then
                    icon_condidates[1] = v -- not deprecated
                    break
                elseif k == "image_data" then -- deprecated
                    icon_condidates[2] = v
                elseif k == "icon_data" then -- deprecated
                    icon_condidates[3] = v
                end
            end

            -- The order is mandated by the spec.
            local icon_data = icon_condidates[1]
                or icon_condidates[2]
                or icon_condidates[3]

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
            args.image = convert_icon(icon_data[1], icon_data[2], icon_data[3], icon_data[6], data)

            -- Convert all animation frames.
            if naughty.image_animations_enabled then
                args.images = {args.image}

                if #icon_data > 7 then
                    for frame=8, #icon_data do
                        data = tostring(icon_data:get_child_value(frame-1).data)

                        table.insert(
                            args.images,
                            convert_icon(
                                icon_data[1],
                                icon_data[2],
                                icon_data[3],
                                icon_data[6],
                                data
                            )
                        )
                    end
                end
            end
        end

        -- Alternate ways to set the icon. The specs recommends to allow both
        -- the icon and image to co-exist since they serve different purpose.
        -- However in case the icon isn't specified, use the image.
        args.image = args.image
            or hints["image-path"] -- not deprecated
            or hints["image_path"] -- deprecated

        if naughty.image_animations_enabled then
            args.images = args.images or {}
        end

        if replaces_id and replaces_id ~= "" and replaces_id ~= 0 then
            args.replaces_id = replaces_id
        end
        if expire and expire > -1 then
            args.timeout = expire / 1000
        end

        args.freedesktop_hints = hints

        -- Not very pretty, but given the current format is documented in the
        -- public API... well, whatever...
        if hints and hints.urgency then
            for name, key in pairs(urgency) do
                local b = string.char(hints.urgency)
                if key == b then
                    args.urgency = name
                end
            end
        end

        args.urgency = args.urgency or "normal"

        -- Try to update existing objects when possible
        notification = naughty.get_by_id(replaces_id)

        if notification then
            if not notification._private._unique_sender then
                -- If this happens, the notification is either trying to
                -- highjack content created within AwesomeWM or it is garbage
                -- to begin with.
                gdebug.print_warning(
                    "A notification has been received, but tried to update "..
                    "the content of a notification it does not own."
                )
            elseif notification._private._unique_sender ~= sender then
                -- Nothing says you cannot and some scripts may do it
                -- accidentally, but this is rather unexpected.
                gdebug.print_warning(
                    "Notification "..notification.title.." is being updated"..
                    "by a different DBus connection ("..sender.."), this is "..
                    "suspicious. The original connection was "..
                    notification._private._unique_sender
                )
            end

            for k, v in pairs(args) do
                if k == "destroy" then k = "destroy_cb" end
                notification[k] = v
            end

            -- Update the icon if necessary.
            if app_icon ~= notification._private.app_icon then
                notification._private.app_icon = app_icon

                naughty._emit_signal_if(
                    "request::icon", function()
                        if notification._private.icon then return true end
                    end, notification, "dbus_clear", {}
                )
            end

            -- Even if no property changed, restart the timeout.
            notification:reset_timeout()
        else
            -- Only set the sender for new notifications.
            args._unique_sender = sender

            notification = nnotif(args)

            notification:connect_signal("destroyed", function(_, r) args.destroy(r) end)
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
        "naughty", "awesome", capi.awesome.version, "1.2"
    }))
end

function notif_methods.GetCapabilities(_, _, _, _, _, invocation)
    -- We actually do display the body of the message, we support <b>, <i>
    -- and <u> in the body and we handle static (non-animated) icons.
    invocation:return_value(GLib.Variant("(as)", {capabilities}))
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

local bus_proxy, pid_for_unique_name = nil, {}

Gio.DBusProxy.new_for_bus(
    Gio.BusType.SESSION,
    Gio.DBusProxyFlags.DO_NOT_LOAD_PROPERTIES,
    nil,
    "org.freedesktop.DBus",
    "/org/freedesktop/DBus",
    "org.freedesktop.DBus",
    nil,
    function(proxy)
        bus_proxy =  proxy
    end,
    nil
)

--- Get the clients associated with a notification.
--
-- Note that is based on the process PID. PIDs roll over, so don't use this
-- with very old notifications.
--
-- Also note that some multi-process application can use a different process
-- for the clients and the service used to send the notifications.
--
-- Since this is based on PIDs, it is impossible to know which client sent the
-- notification if the process has multiple clients (windows). Using the
-- `client.type` can be used to further filter this list into more probable
-- candidates (tooltips, menus and dialogs are unlikely to send notifications).
--
-- @tparam naughty.notification notif A notification object.
-- @treturn table A table with all associated clients.
function dbus.get_clients(notif)
    -- First, the trivial case, but I never found an app that implements it.
    -- It isn't standardized, but mentioned as possible.
    local win_id = notif.freedesktop_hints and (notif.freedesktop_hints.window_ID
        or notif.freedesktop_hints["window-id"]
        or notif.freedesktop_hints.windowID
        or notif.freedesktop_hints.windowid)

    if win_id then
        for _, c in ipairs(client.get()) do
            if c.window_id == win_id then
                return {win_id}
            end
        end
    end

    -- Less trivial, but mentioned in the spec. Note that this isn't
    -- recommended by the spec, let alone mandatory. It is mentioned it can
    -- exist. This wont work with Flatpak or Snaps.
    local pid = notif.freedesktop_hints and (
        notif.freedesktop_hints.PID or notif.freedesktop_hints.pid
    )

    if ((not bus_proxy) or not notif._private._unique_sender) and not pid then
        return {}
    end

    if (not pid) and (not pid_for_unique_name[notif._private._unique_sender]) then
        local owner = GLib.Variant("(s)", {notif._private._unique_sender})

        -- It is sync, but this isn't done very often and since it is DBus
        -- daemon itself, it is very responsive. Doing this using the async
        -- variant would cause the clients to be unavailable in the notification
        -- rules.
        pid = bus_proxy:call_sync("GetConnectionUnixProcessID",
            owner,
            Gio.DBusCallFlags.NONE,
            -1
        )

        if (not pid) or (not pid.value) then return {} end

        pid = pid.value and pid.value[1]

        if not pid then return {} end

        pid_for_unique_name[notif._private._unique_sender] = pid
    end

    pid = pid or pid_for_unique_name[notif._private._unique_sender]

    if not pid then return {} end

    local ret = {}

    for _, c in ipairs(client.get()) do
        if c.pid == pid then
            table.insert(ret, c)
        end
    end

    return ret
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

local function remove_capability(cap)
    for k, v in ipairs(capabilities) do
        if v == cap then
            table.remove(capabilities, k)
            break
        end
    end
end

-- Update the capabilities.
naughty.connect_signal("property::persistence_enabled", function()
    remove_capability("persistence")

    if naughty.persistence_enabled then
        table.insert(capabilities, "persistence")
    end
end)
naughty.connect_signal("property::image_animations_enabled", function()
    remove_capability("icon-multi")
    remove_capability("icon-static")

    table.insert(capabilities, naughty.persistence_enabled
        and "icon-multi" or "icon-static"
    )
end)

-- For the tests.
dbus._capabilities = capabilities

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
