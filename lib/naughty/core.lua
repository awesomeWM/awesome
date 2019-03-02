----------------------------------------------------------------------------
--- Notification library.
--
-- For more details on how to create notifications, see `naughty.notification`.
--
-- To send notifications from the terminal, use `notify-send`.
--
-- @author koniu &lt;gkusnierz@gmail.com&gt;
-- @copyright 2008 koniu
-- @module naughty
----------------------------------------------------------------------------

--luacheck: no max line length

-- Package environment
local capi = { screen = screen }
local gdebug = require("gears.debug")
local screen = require("awful.screen")
local gtable = require("gears.table")

local naughty = {}

--- Naughty configuration - a table containing common popup settings.
--
-- @table naughty.config
-- @tfield[opt=apply_dpi(4)] int padding Space between popups and edge of the
--   workarea.
-- @tfield[opt=apply_dpi(1)] int spacing Spacing between popups.
-- @tfield[opt={"/usr/share/pixmaps/"}] table icon_dirs List of directories
--   that will be checked by `getIcon()`.
-- @tfield[opt={ "png", "gif" }] table icon_formats List of formats that will be
--   checked by `getIcon()`.
-- @tfield[opt] function notify_callback Callback used to modify or reject
-- notifications, e.g.
--     naughty.config.notify_callback = function(args)
--         args.text = 'prefix: ' .. args.text
--         return args
--     end
--   To reject a notification return `nil` from the callback.
--   If the notification is a freedesktop notification received via DBUS, you can
--   access the freedesktop hints via `args.freedesktop_hints` if any where
--   specified.
--
-- @tfield table presets Notification presets.  See `config.presets`.
--
-- @tfield table defaults Default values for the params to `notify()`.  These can
--   optionally be overridden by specifying a preset.  See `config.defaults`.

-- It's done that way to preserve compatibility with Awesome 4.0 while allowing
-- the naughty submodules to use the contants without creating a circular
-- dependency.
gtable.crush(naughty, require("naughty.constants"))

--- Notification presets for `naughty.notify`.
-- This holds presets for different purposes.  A preset is a table of any
-- parameters for `notify()`, overriding the default values
-- (`naughty.config.defaults`).
--
-- You have to pass a reference of a preset in your `notify()` as the `preset`
-- argument.
--
-- The presets `"low"`, `"normal"` and `"critical"` are used for notifications
-- over DBUS.
--
-- @table config.presets
-- @tfield table low The preset for notifications with low urgency level.
-- @tfield[opt=5] int low.timeout
-- @tfield[opt=empty] table normal The default preset for every notification without a
--   preset that will also be used for normal urgency level.
-- @tfield table critical The preset for notifications with a critical urgency
--   level.
-- @tfield[opt="#ff0000"] string critical.bg
-- @tfield[opt="#ffffff"] string critical.fg
-- @tfield[opt=0] string critical.timeout

--- Defaults for `naughty.notify`.
--
-- @table config.defaults
-- @tfield[opt=5] int timeout
-- @tfield[opt=""] string text
-- @tfield[opt] int screen Defaults to `awful.screen.focused`.
-- @tfield[opt=true] boolean ontop
-- @tfield[opt=apply_dpi(5)] int margin
-- @tfield[opt=apply_dpi(1)] int border_width
-- @tfield[opt="top_right"] string position

--- The reason why a notification is to be closed.
-- See [the specification](https://developer.gnome.org/notification-spec/#signals)
-- for more details.
-- @tfield number silent
-- @tfield number expired
-- @tfield number dismissed_by_user
-- @tfield number dismissed_by_command
-- @tfield number undefined
-- @table notification_closed_reason

--- The global suspension state.
--
-- When suspended, no notification widget should interrupt the user. This is
-- useful when watching movies or doing presentations.
--
-- @property suspended
-- @param boolean

--- Do not allow notifications to auto-expire.
--
-- When navigating the notifications, for example on mouse over or when
-- keyboard navigation is enabled, it is very annoying when notifications
-- just vanish.
--
-- @property expiration_paused
-- @param[opt=false] boolean

local properties = {
    suspended         = false,
    expiration_paused = false
}

--TODO v5 Deprecate the public `naughty.notifications` (to make it private)

--- Index of notifications per screen and position.
-- See config table for valid 'position' values.
-- Each element is a table consisting of:
--
-- @field box Wibox object containing the popup
-- @field height Popup height
-- @field width Popup width
-- @field die Function to be executed on timeout
-- @field id Unique notification id based on a counter
-- @table notifications
naughty.notifications = { suspended = { }, _expired = {{}} }
screen.connect_for_each_screen(function(s)
    naughty.notifications[s] = {
        top_left = {},
        top_middle = {},
        top_right = {},
        bottom_left = {},
        bottom_middle = {},
        bottom_right = {},
    }
end)

capi.screen.connect_signal("removed", function(scr)
    -- Destroy all notifications on this screen
    naughty.destroy_all_notifications({scr})
    naughty.notifications[scr] = nil
end)

--- Notification state.
--
-- This function is deprecated, use `naughty.suspended`.
--
-- @deprecated naughty.is_suspended
function naughty.is_suspended()
    gdebug.deprecate("Use naughty.suspended", {deprecated_in=5})
    return properties.suspended
end

--- Suspend notifications.
--
-- This function is deprecated, use `naughty.suspended = true`.
--
-- @deprecated naughty.suspend
function naughty.suspend()
    gdebug.deprecate("Use naughty.suspended = true", {deprecated_in=5})
    properties.suspended = true
end

local conns = {}

--- Connect a global signal on the notification engine.
--
-- Functions connected to this signal source will be executed when any
-- notification object emits the signal.
--
-- It is also used for some generic notification signals such as
-- `request::display`.
--
-- @tparam string name The name of the signal
-- @tparam function func The function to attach
-- @usage naughty.connect_signal("added", function(notif)
--    -- do something
-- end)
function naughty.connect_signal(name, func)
    assert(name)
    conns[name] = conns[name] or {}
    table.insert(conns[name], func)
end

--- Emit a notification signal.
-- @tparam string name The signal name.
-- @param ... The signal callback arguments
function naughty.emit_signal(name, ...)
    assert(name)
    for _, func in pairs(conns[name] or {}) do
        func(...)
    end
end

--- Disconnect a signal from a source.
-- @tparam string name The name of the signal
-- @tparam function func The attached function
-- @treturn boolean If the disconnection was successful
function naughty.disconnect_signal(name, func)
    for k, v in ipairs(conns[name] or {}) do
        if v == func then
            table.remove(conns[name], k)
            return true
        end
    end
    return false
end

local function resume()
    properties.suspended = false
    for _, v in pairs(naughty.notifications.suspended) do
        local args = v._private.args
        assert(args)
        v._private.args = nil

        naughty.emit_signal("added", v, args)
        naughty.emit_signal("request::display", v, args)
        if v.timer then v.timer:start() end
    end
    naughty.notifications.suspended = { }
end

--- Resume notifications.
--
-- This function is deprecated, use `naughty.suspended = false`.
--
-- @deprecated naughty.resume
function naughty.resume()
    gdebug.deprecate("Use naughty.suspended = false", {deprecated_in=5})
    resume()
end

--- Toggle notification state.
--
-- This function is deprecated, use `naughty.suspended = not naughty.suspended`.
--
-- @deprecated naughty.toggle
function naughty.toggle()
    gdebug.deprecate("Use naughty.suspended = not naughty.suspended", {deprecated_in=5})
    if properties.suspended then
        naughty.resume()
    else
        naughty.suspend()
    end
end

--- Destroy notification by notification object
--
-- This function is deprecated in favor of
-- `notification:destroy(reason, keep_visible)`.
--
-- @param notification Notification object to be destroyed
-- @param reason One of the reasons from `notification_closed_reason`
-- @param[opt=false] keep_visible If true, keep the notification visible
-- @return True if the popup was successfully destroyed, nil otherwise
-- @deprecated naughty.destroy
function naughty.destroy(notification, reason, keep_visible)
    gdebug.deprecate("Use notification:destroy(reason, keep_visible)", {deprecated_in=5})

    if not notification then return end

    return notification:destroy(reason, keep_visible)
end

--- Destroy all notifications on given screens.
--
-- @tparam table screens Table of screens on which notifications should be
-- destroyed. If nil, destroy notifications on all screens.
-- @tparam naughty.notification_closed_reason reason Reason for closing
-- notifications.
-- @treturn true|nil True if all notifications were successfully destroyed, nil
-- otherwise.
-- @see notification_closed_reason
function naughty.destroy_all_notifications(screens, reason)
    if not screens then
        screens = {}
        for key, _ in pairs(naughty.notifications) do
            table.insert(screens, key)
        end
    end
    local ret = true
    for _, scr in pairs(screens) do
        for _, list in pairs(naughty.notifications[scr]) do
            while #list > 0 do
                ret = ret and list[1]:destroy(reason)
            end
        end
    end
    return ret
end

--- Get notification by ID
--
-- @param id ID of the notification
-- @return notification object if it was found, nil otherwise
-- @deprecated naughty.getById
function naughty.getById(id)
    gdebug.deprecate("Use naughty.get_by_id", {deprecated_in=5})
    return naughty.get_by_id(id)
end

--- Get notification by ID
--
-- @param id ID of the notification
-- @return notification object if it was found, nil otherwise
function naughty.get_by_id(id)
    -- iterate the notifications to get the notfications with the correct ID
    for s in pairs(naughty.notifications) do
        for p in pairs(naughty.notifications[s]) do
            for _, notification in pairs(naughty.notifications[s][p]) do
                if notification.id == id then
                    return notification
                 end
            end
        end
    end
end

--- Set new notification timeout.
--
-- This function is deprecated, use `notification:reset_timeout(new_timeout)`.
--
-- @tparam notification notification Notification object, which timer is to be reset.
-- @tparam number new_timeout Time in seconds after which notification disappears.
-- @deprecated naughty.reset_timeout
function naughty.reset_timeout(notification, new_timeout)
    gdebug.deprecate("Use notification:reset_timeout(new_timeout)", {deprecated_in=5})

    if not notification then return end

    notification:reset_timeout(new_timeout)
end

--- Replace title and text of an existing notification.
--
-- This function is deprecated, use `notification.message = new_text` and
-- `notification.title = new_title`
--
-- @tparam notification notification Notification object, which contents are to be replaced.
-- @tparam string new_title New title of notification. If not specified, old title remains unchanged.
-- @tparam string new_text New text of notification. If not specified, old text remains unchanged.
-- @return None.
-- @deprecated naughty.replace_text
function naughty.replace_text(notification, new_title, new_text)
    gdebug.deprecate(
        "Use notification.text = new_text; notification.title = new_title",
        {deprecated_in=5}
    )

    if not notification then return end

    notification.title = new_title or notification.title
    notification.text  = new_text  or notification.text
end

-- Remove the notification from the internal list(s)
local function cleanup(self, reason)
    assert(reason, "Use n:destroy() instead of emitting the signal directly")

    if properties.suspended then
        for k, v in pairs(naughty.notifications.suspended) do
            if v == self then
                table.remove(naughty.notifications.suspended, k)
                break
            end
        end
    end
    local scr = self.screen

    assert(naughty.notifications[scr][self.position][self.idx] == self)
    table.remove(naughty.notifications[scr][self.position], self.idx)

    -- Update all indices
    for k, n in ipairs(naughty.notifications[scr][self.position]) do
        n.idx = k
    end

    -- `self.timer.started` will be false if the expiration was paused.
    if self.timer and self.timer.started then
        self.timer:stop()
    end

    if self.destroy_cb and reason ~= naughty.notification_closed_reason.silent then
        self.destroy_cb(reason or naughty.notification_closed_reason.undefined)
    end
end

naughty.connect_signal("destroyed", cleanup)

local function get_screen(s)
    return s and capi.screen[s]
end

-- Proxy the global suspension state on all notification objects
local function get_suspended(self)
    return properties.suspended and not self.ignore_suspend
end

function naughty.set_expiration_paused(p)
    properties.expiration_paused = p

    if not p then
         for _, n in ipairs(naughty.notifications._expired[1]) do
            n:destroy(naughty.notification_closed_reason.expired)
         end
    end
end

--- Emitted when a notification is created.
-- @signal added
-- @tparam naughty.notification notification The notification object

--- Emitted when a notification is destroyed.
-- @signal destroyed
-- @tparam naughty.notification notification The notification object

--- Emitted when a notification has to be displayed.
--
-- To add an handler, use:
--
--    naughty.connect_signal("request::display", function(notification, args)
--        -- do something
--    end)
--
-- @tparam table notification The `naughty.notification` object.
-- @tparam table args Any arguments passed to the `naughty.notify` function,
--  including, but not limited to, all `naughty.notification` properties.
-- @signal request::display

--- Emitted when a notification needs pre-display configuration.
--
-- @tparam table notification The `naughty.notification` object.
-- @tparam table args Any arguments passed to the `naughty.notify` function,
--  including, but not limited to, all `naughty.notification` properties.
-- @signal request::preset



-- Register a new notification object.
local function register(notification, args)

    -- Add the some more properties
    rawset(notification, "get_suspended", get_suspended)

    --TODO v5 uncouple the notifications and the screen
    local s = get_screen(args.screen or notification.preset.screen or screen.focused())

    -- insert the notification to the table
    table.insert(naughty.notifications[s][notification.position], notification)
    notification.idx    = #naughty.notifications[s][notification.position]
    notification.screen = s

    if properties.suspended and not args.ignore_suspend then
        notification._private.args = args
        table.insert(naughty.notifications.suspended, notification)
    else
        naughty.emit_signal("added", notification, args)
    end

    assert(rawget(notification, "preset"))

    -- return the notification
    return notification
end

naughty.connect_signal("new", register)

local function index_miss(_, key)
    if rawget(naughty, "get_"..key) then
         return rawget(naughty, "get_"..key)()
    elseif properties[key] ~= nil then
        return properties[key]
    else
        return nil
    end
end

local function set_index_miss(_, key, value)
    if rawget(naughty, "set_"..key) then
         return rawget(naughty, "set_"..key)(value)
    elseif properties[key] ~= nil then
        assert(type(value) == "boolean")
        properties[key] = value
        if not value then
            resume()
        end
    else
        rawset(naughty, key, value)
    end
end

--- Create a notification.
--
-- This function is deprecated, create notification objects instead:
--
--    local notif = naughty.notification(args)
--
-- @tab args The argument table containing any of the arguments below.
-- @string[opt=""] args.text Text of the notification.
-- @string[opt] args.title Title of the notification.
-- @int[opt=5] args.timeout Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @int[opt] args.hover_timeout Delay in seconds after which hovered popup disappears.
-- @tparam[opt=focused] integer|screen args.screen Target screen for the notification.
-- @string[opt="top_right"] args.position Corner of the workarea displaying the popups.
--   Values: `"top_right"`, `"top_left"`, `"bottom_left"`,
--   `"bottom_right"`, `"top_middle"`, `"bottom_middle"`.
-- @bool[opt=true] args.ontop Boolean forcing popups to display on top.
-- @int[opt=`beautiful.notification_height` or auto] args.height Popup height.
-- @int[opt=`beautiful.notification_width` or auto] args.width Popup width.
-- @string[opt=`beautiful.notification_font` or `beautiful.font` or `awesome.font`] args.font Notification font.
-- @string[opt] args.icon Path to icon.
-- @int[opt] args.icon_size Desired icon size in px.
-- @string[opt=`beautiful.notification_fg` or `beautiful.fg_focus` or `'#ffffff'`] args.fg Foreground color.
-- @string[opt=`beautiful.notification_fg` or `beautiful.bg_focus` or `'#535d6c'`] args.bg Background color.
-- @int[opt=`beautiful.notification_border_width` or 1] args.border_width Border width.
-- @string[opt=`beautiful.notification_border_color` or `beautiful.border_focus` or `'#535d6c'`] args.border_color Border color.
-- @tparam[opt=`beautiful.notification_shape`] gears.shape args.shape Widget shape.
-- @tparam[opt=`beautiful.notification_opacity`] gears.opacity args.opacity Widget opacity.
-- @tparam[opt=`beautiful.notification_margin`] gears.margin args.margin Widget margin.
-- @tparam[opt] func args.run Function to run on left click.  The notification
--   object will be passed to it as an argument.
--   You need to call e.g.
--   `notification.die(naughty.notification_closed_reason.dismissedByUser)` from
--   there to dismiss the notification yourself.
-- @tparam[opt] func args.destroy Function to run when notification is destroyed.
-- @tparam[opt] table args.preset Table with any of the above parameters.
--   Note: Any parameters specified directly in args will override ones defined
--   in the preset.
-- @tparam[opt] int args.replaces_id Replace the notification with the given ID.
-- @tparam[opt] func args.callback Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @tparam[opt] table args.actions A list of `naughty.action`s.
-- @bool[opt=false] args.ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.
-- @usage naughty.notify({ title = "Achtung!", message = "You're idling", timeout = 0 })
-- @treturn ?table The notification object, or nil in case a notification was
--   not displayed.
-- @deprecated naughty.notify

local nnotif = nil

function naughty.notify(args)
    gdebug.deprecate(
        "Use local notif = naughty.notification(args)",
        {deprecated_in=5}
    )

    --TODO v6 remove this hack
    nnotif = nnotif or require("naughty.notification")

    -- The existing notification object, if any.
    local n = args and args.replaces_id and
        naughty.get_by_id(args.replaces_id) or nil

    -- It was possible to update the notification content using `replaces_id`.
    -- This is a concept that come from the dbus API and leaked into the public
    -- API. It has all kind of issues and brokenness, but it being used.
    if n then
        return gtable.crush(n, args)
    end

    return nnotif(args)
end

return setmetatable(naughty, {__index = index_miss, __newindex = set_index_miss})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
