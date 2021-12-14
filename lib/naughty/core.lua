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
local gdebug  = require("gears.debug")
local screen  = require("awful.screen")
local gtable  = require("gears.table")
local gobject = require("gears.object")
local gsurface = require("gears.surface")

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
-- @tparam boolean suspended
-- @emits added
-- @propemits true false

--- Do not allow notifications to auto-expire.
--
-- When navigating the notifications, for example on mouse over or when
-- keyboard navigation is enabled, it is very annoying when notifications
-- just vanish.
--
-- @property expiration_paused
-- @tparam[opt=false] boolean expiration_paused
-- @propemits true false

--- A table with all active notifications.
--
-- Please note that this list is kept up-to-date even in suspended mode.
--
-- **Signal:**
--
-- * property::active
--
-- @property active
-- @tparam table active
-- @propemits false false

--- True when there is a handler connected to `request::display`.
-- @property has_display_handler
-- @tparam boolean has_display_handler

--- If the timeout needs to be reset when a property changes.
--
-- This is the global variant of the `naughty.notification` `auto_reset_timeout`
-- property.
--
-- @property auto_reset_timeout
-- @tparam[opt=true] boolean auto_reset_timeout
-- @propemits true false

--- Enable or disable naughty ability to claim to support animations.
--
-- When this is true, applications which query `naughty` feature support
-- will see that animations are supported. Note that there is *very little*
-- support for this and enabling it will cause bugs.
--
-- @property image_animations_enabled
-- @tparam[opt=false] boolean image_animations_enabled
-- @propemits true false

--- Enable or disable the persistent notifications.
--
-- This is very annoying when using `naughty.layout.box` popups, but tolerable
-- when using `naughty.list.notifications`.
--
-- Note that enabling this **does nothing** in `naughty` itself. The timeouts
-- are still honored and notifications still destroyed. It is the user
-- responsibility to disable the dismiss timer. However, this tells the
-- applications that notification persistence is supported so they might
-- stop using systray icons for the sake of displaying or other changes like
-- that.
--
-- @property persistence_enabled
-- @tparam[opt=false] boolean persistence_enabled
-- @propemits true false

local properties = {
    suspended                = false,
    expiration_paused        = false,
    auto_reset_timeout       = true,
    image_animations_enabled = false,
    persistence_enabled      = false,
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

naughty._active = {}

local function get_screen(s)
    return s and capi.screen[s]
end

local function init_screen(s)
    if naughty.notifications[s] then return end

    naughty.notifications[s] = {
        top_left = {},
        top_middle = {},
        top_right = {},
        bottom_left = {},
        bottom_middle = {},
        bottom_right = {},
        middle = {},
    }
end

screen.connect_for_each_screen(init_screen)

capi.screen.connect_signal("removed", function(scr)
    -- Allow the notifications to be moved to another screen.

    for _, list in pairs(naughty.notifications[scr]) do
        -- Clone the list to avoid having an iterator while mutating.
        list = gtable.clone(list, false)

        for _, n in ipairs(list) do
            naughty.emit_signal("request::screen", n, "removed", {})
        end
    end

    for _, n in ipairs(naughty._active) do
        if n._private.args and get_screen(n._private.args.screen) == scr then
            n._private.args.screen = nil
        end
    end

    -- Destroy all notifications on this screen
    naughty.destroy_all_notifications({scr})
    naughty.notifications[scr] = nil
end)

local function remove_from_index(n)
    for _, positions in pairs(naughty.notifications) do
        for _, ns in pairs(positions) do
            for k, n2 in ipairs(ns) do
                if n2 == n then
                    assert(ns[k+1] ~= n, "The notification index is corrupted")
                    table.remove(ns, k)
                    return
                end
            end
        end
    end
end

-- When id or screen are set after the object is created, update the indexing.
local function update_index(n)
    -- Do things in the right order.
    if not n._private.registered then return end

    assert(not n._private.is_destroyed, "The notification index is corrupted")

    -- Find the only index and remove it (there's an useless loop, but it's small).
    remove_from_index(n)

    -- Add to the index again
    local s = get_screen(n.screen
        or (n.preset and n.preset.screen)
        or screen.focused())
    naughty.notifications[s] = naughty.notifications[s] or {}
    table.insert(naughty.notifications[s][n.position], n)
end

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

local conns = gobject._setup_class_signals(
    naughty, {allow_chain_of_responsibility=true}
)

local function resume()
    properties.suspended = false
    for _, v in ipairs(naughty.notifications.suspended) do
        local args = v._private.args
        assert(args)
        v._private.args = nil

        v:emit_signal("property::suspended", false)

        naughty.emit_signal("added", v, args)
        naughty.emit_signal("request::display", v, "resume", args)
        if v.timer then v.timer:start() end

        if not v._private.args then
            v._private.args = args
        end
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
-- @tparam naughty.notification notification Notification object to be destroyed
-- @tparam string reason One of the reasons from `notification_closed_reason`
-- @tparam[opt=false] boolean keep_visible If true, keep the notification visible
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
-- @staticfct naughty.destroy_all_notifications
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
                -- Better cause an error than risk an infinite loop.
                assert(not list[1]._private.is_destroyed)

                ret = ret and list[1]:destroy(reason)
            end
        end
    end
    return ret
end

--- Get notification by ID
--
-- @tparam integer id ID of the notification
-- @treturn naughty.notification|nil notification object if it was found, nil otherwise
-- @deprecated naughty.getById
function naughty.getById(id)
    gdebug.deprecate("Use naughty.get_by_id", {deprecated_in=5})
    return naughty.get_by_id(id)
end

--- Get notification by ID
--
-- @tparam integer id ID of the notification
-- @treturn naughty.notification|nil notification object if it was found, nil otherwise
-- @staticfct naughty.get_by_id
function naughty.get_by_id(id)
    -- iterate the notifications to get the notfications with the correct ID
    for s in capi.screen do
        for p in pairs(naughty.notifications[s] or {}) do
            for _, notification in pairs(naughty.notifications[s][p]) do
                if notification.id == id then
                    return notification
                end
            end
        end
    end
end

-- Use an explicit getter to make it read only.
function naughty.get_active()
    return naughty._active
end

function naughty.get_has_display_handler()
    return conns["request::display"] and #conns["request::display"] > 0 or false
end

-- Presets are "deprecated" when notification rules are used.
function naughty.get__has_preset_handler()
    return conns["request::preset"] and #conns["request::preset"] > 0 or false
end

function naughty._reset_display_handlers()
    conns["request::display"] = nil
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
        for k, v in ipairs(naughty.notifications.suspended) do
            if v == self then
                table.remove(naughty.notifications.suspended, k)
                break
            end
        end
    end
    local scr = self.screen

    assert(naughty.notifications[scr][self.position][self.idx] == self)
    remove_from_index(self)

    -- Update all indices
    for k, n in ipairs(naughty.notifications[scr][self.position]) do
        n.idx = k
    end

    -- Remove from the global active list.
    for k, n in ipairs(naughty._active) do
         if n == self then
            table.remove(naughty._active, k)
            naughty.emit_signal("property::active")
         end
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

-- Proxy the global suspension state on all notification objects
local function get_suspended(self)
    return properties.suspended and not self.ignore_suspend
end

function naughty.set_suspended(value)
    if properties["suspended"] == value then return end

    properties["suspended"] = value

    if value then
        for _, n in pairs(naughty._active) do
            if not n.ignore_suspend then
                if n.timer and n.timer.started then
                    n.timer:stop()
                end

                n:emit_signal("property::suspended", true)
                table.insert(naughty.notifications.suspended, n)
            end
        end
    else
        resume()
    end
end

function naughty.set_expiration_paused(p)
    properties.expiration_paused = p

    if not p then
         for _, n in ipairs(naughty.notifications._expired[1]) do
            n:destroy(naughty.notification_closed_reason.expired)
         end
    end
end

--- The default handler for `request::screen`.
--
-- It selects `awful.screen.focused()`.
--
-- @signalhandler naughty.default_screen_handler

function naughty.default_screen_handler(n)
    if n.screen and n.screen.valid then return end

    n.screen = screen.focused()
end

naughty.connect_signal("request::screen", naughty.default_screen_handler)

--- Emitted when an error occurred and requires attention.
-- @signal request::display_error
-- @tparam string message The error message.
-- @tparam boolean startup If the error occurred during the initial loading of
--  rc.lua (and thus caused the fallback to kick in).

--- Emitted when a notification is created.
-- @signal added
-- @tparam naughty.notification notification The notification object

--- Emitted when a notification is destroyed.
-- @signal destroyed
-- @tparam naughty.notification notification The notification object

--- Emitted when a notification has to be displayed.
--
-- To add a handler, use:
--
--    naughty.connect_signal("request::display", function(notification, args)
--        -- do something
--    end)
--
-- @tparam table notification The `naughty.notification` object.
-- @tparam string context Why is the signal sent.
-- @tparam table args Any arguments passed to the `naughty.notify` function,
--  including, but not limited to, all `naughty.notification` properties.
-- @signal request::display

--- Emitted when a notification needs pre-display configuration.
--
-- @tparam table notification The `naughty.notification` object.
-- @tparam string context Why is the signal sent.
-- @tparam table args Any arguments passed to the `naughty.notify` function,
--  including, but not limited to, all `naughty.notification` properties.
-- @signal request::preset

--- Emitted when an action requires an icon it doesn't know.
--
-- The implementation should look in the icon theme for an action icon or
-- provide something natively.
--
-- If an icon is found, the handler must set the `icon` property on the `action`
-- object to a path or a `gears.surface`.
--
-- There is no implementation by default. To use the XDG-icon, the common
-- implementation will be:
--
--    naughty.connect_signal("request::action_icon", function(a, context, hints)
--         a.icon = menubar.utils.lookup_icon(hints.id)
--    end)
--
-- @signal request::action_icon
-- @tparam naughty.action action The action.
-- @tparam string context The context.
-- @tparam table hints
-- @tparam string args.id The action id. This will often by the (XDG) icon name.

--- Emitted when a notification icon could not be loaded.
--
-- When an icon is passed in some "encoded" formats, such as XDG icon names or
-- network URLs, AwesomeWM will not attempt to load it. If you wish to see the
-- icon displayed, you must provide an handler. It is highly recommended for
-- handler to only set `n.icon` when they *found* the icon. That way multiple
-- handlers can be attached for multiple protocols.
--
-- The `context` argument is the origin of the icon to decode. If an handler
-- only supports one if them, it should check the `context` and return if it
-- doesn't handle it. The currently valid contexts are:
--
-- * app_icon
-- * clients
-- * path
-- * image
-- * images
-- * dbus_clear
--
-- For example, an implementation which uses the `app_icon` to perform an XDG
-- icon lookup will look like:
--
--    naughty.connect_signal("request::icon", function(n, context, hints)
--        if context ~= "app_icon" then return end
--
--        local path = menubar.utils.lookup_icon(hints.app_icon) or
--            menubar.utils.lookup_icon(hints.app_icon:lower())
--
--        if path then
--            n.icon = path
--        end
--    end)
--
-- The `images` context has no handler. It is part of the specification to
-- handle animations. This is not supported by default.
--
-- @signal request::icon
-- @tparam notification n The notification.
-- @tparam string context The source of the icon to look for.
-- @tparam table hints The hints.
-- @tparam string hints.app_icon The name of the icon to look for.
-- @tparam string hints.path The path of the icon.
-- @tparam string hints.image The path or pixmap of the icon.
-- @see naughty.icon_path_handler
-- @see naughty.client_icon_handler

--- Emitted when the screen is not defined or being removed.
-- @signal request::screen
-- @tparam table notification The `naughty.notification` object. This is
--  currently either "new" or "removed".
-- @tparam string context Why is the signal sent.

-- Register a new notification object.
local function register(notification, args)
    assert(not notification._private.registered)

    -- Add the some more properties
    rawset(notification, "get_suspended", get_suspended)

    local s = get_screen(notification.screen or args.screen
        or (notification.preset and notification.preset.screen))

    if not s then
        naughty.emit_signal("request::screen", notification, "new", {})
        s = notification.screen
    end

    assert(s)

    if not naughty.notifications[s] then
        init_screen(get_screen(s))
    end

    -- insert the notification to the table
    table.insert(naughty._active, notification)
    table.insert(naughty.notifications[s][notification.position], notification)
    notification.idx    = #naughty.notifications[s][notification.position]
    notification.screen = s

    notification._private.registered = true

    notification._private.args = args

    if properties.suspended and not args.ignore_suspend then
        table.insert(naughty.notifications.suspended, notification)
    else
        naughty.emit_signal("added", notification, args)
    end

    assert(rawget(notification, "preset") or naughty._has_preset_handler)

    naughty.emit_signal("property::active")

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

        naughty.emit_signal("property::"..key, value)
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
-- @tparam table args The argument table containing any of the arguments below.
-- @string[opt=""] args.text Text of the notification.
-- @string[opt] args.title Title of the notification.
-- @int[opt=5] args.timeout Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @int[opt] args.hover_timeout Delay in seconds after which hovered popup disappears.
-- @tparam[opt=focused] integer|screen args.screen Target screen for the notification.
-- @string[opt="top_right"] args.position Corner of the workarea displaying the popups.
--   Values: `"top_right"`, `"top_left"`, `"bottom_left"`,
--   `"bottom_right"`, `"top_middle"`, `"bottom_middle"`, `"middle"`.
-- @bool[opt=true] args.ontop Boolean forcing popups to display on top.
-- @int[opt=`beautiful.notification_height` or auto] args.height Popup height.
-- @int[opt=`beautiful.notification_width` or auto] args.width Popup width.
-- @string[opt=`beautiful.notification_font` or `beautiful.font` or `awesome.font`] args.font Notification font.
-- @string[opt] args.icon Path to icon.
-- @int[opt] args.icon_size Desired icon size in px.
-- @string[opt=`beautiful.notification_fg` or `beautiful.fg_focus` or `'#ffffff'`] args.fg Foreground color.
-- @string[opt=`beautiful.notification_fg` or `beautiful.bg_focus` or `'#535d6c'`] args.bg Background color.
-- @int[opt=`beautiful.notification_border_width` or 1] args.border_width Border width.
-- @string[opt=`beautiful.notification_border_color` or `beautiful.border_color_active` or `'#535d6c'`] args.border_color Border color.
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

--- Request handler to get the icon using the clients icons.
-- @signalhandler naughty.client_icon_handler
function naughty.client_icon_handler(self, context)
    if context ~= "clients" then return end

    local clients = self:get_clients()

    for _, t in ipairs { "normal", "dialog" } do
        for _, c in ipairs(clients) do
            if c.type == t then
                self._private.icon = gsurface(c.icon) --TODO support other size
                return
            end
        end
    end
end

--- Request handler to get the icon using the image or path.
-- @signalhandler naughty.icon_path_handler
function naughty.icon_path_handler(self, context, hints)
    if context ~= "image" and context ~= "path" then return end

    self._private.icon = gsurface.load_uncached_silently(
        hints.path or hints.image
    )
end

--- Request handler for clearing the icon when asked by ie, DBus.
-- @signalhandler naughty.icon_clear_handler
function naughty.icon_clear_handler(self, context, hints) --luacheck: no unused args
    if context ~= "dbus_clear" then return end

    self._private.icon = nil
    self:emit_signal("property::icon")
end

naughty.connect_signal("property::screen"  , update_index)
naughty.connect_signal("property::position", update_index)

naughty.connect_signal("request::icon", naughty.client_icon_handler)
naughty.connect_signal("request::icon", naughty.icon_path_handler  )
naughty.connect_signal("request::icon", naughty.icon_clear_handler )

--@DOC_signals_COMMON@

return setmetatable(naughty, {__index = index_miss, __newindex = set_index_miss})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
