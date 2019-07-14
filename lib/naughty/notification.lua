---------------------------------------------------------------------------
--- A notification object.
--
-- This class creates individual notification objects that can be manipulated
-- to extend the default behavior.
--
-- This class doesn't define the actual widget, but is rather intended as a data
-- object to hold the properties. All examples assume the default widgets, but
-- the whole implementation can be replaced.
--
--@DOC_naughty_actions_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee
-- @copyright 2008 koniu
-- @copyright 2017 Emmanuel Lepage Vallee
-- @coreclassmod naughty.notification
---------------------------------------------------------------------------
local gobject  = require("gears.object")
local gtable   = require("gears.table")
local gsurface = require("gears.surface")
local timer    = require("gears.timer")
local cst      = require("naughty.constants")
local naughty  = require("naughty.core")
local gdebug   = require("gears.debug")

local notification = {}

--- Notifications font.
-- @beautiful beautiful.notification_font
-- @tparam string|lgi.Pango.FontDescription notification_font

--- Notifications background color.
-- @beautiful beautiful.notification_bg
-- @tparam color notification_bg

--- Notifications foreground color.
-- @beautiful beautiful.notification_fg
-- @tparam color notification_fg

--- Notifications border width.
-- @beautiful beautiful.notification_border_width
-- @tparam int notification_border_width

--- Notifications border color.
-- @beautiful beautiful.notification_border_color
-- @tparam color notification_border_color

--- Notifications shape.
-- @beautiful beautiful.notification_shape
-- @tparam[opt] gears.shape notification_shape
-- @see gears.shape

--- Notifications opacity.
-- @beautiful beautiful.notification_opacity
-- @tparam[opt] int notification_opacity

--- The margins inside of the notification widget (or popup).
-- @beautiful beautiful.notification_margin
-- @tparam int notification_margin

--- Notifications width.
-- @beautiful beautiful.notification_width
-- @tparam int notification_width

--- Notifications height.
-- @beautiful beautiful.notification_height
-- @tparam int notification_height

--- The spacing between the notifications.
-- @beautiful beautiful.notification_spacing
-- @param[opt=2] number
-- @see gears.surface

-- Unique identifier of the notification.
-- This is the equivalent to a PID as allows external applications to select
-- notifications.
-- @property id
-- @param number

--- Text of the notification.
--
-- This exists only for the pre-AwesomeWM v4.4 new notification implementation.
-- Please always use `title`.
--
-- @deprecatedproperty text
-- @param string
-- @see title

--- Title of the notification.
--@DOC_naughty_helloworld_EXAMPLE@
-- @property title
-- @param string

--- Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @property timeout
-- @param number

--- The notification urgency level.
--
-- The default urgency levels are:
--
-- * low
-- * normal
-- * critical
--
-- @property urgency
-- @param string

--- The notification category.
--
-- The category should be named using the `x-vendor.class.name` naming scheme or
-- use one of the default categories:
--
-- <table class='widget_list' border=1>
--  <tr style='font-weight: bold;'>
--   <th align='center'>Name</th>
--   <th align='center'>Description</th>
--  </tr>
-- <tr><td><b>device</b></td><td>A generic device-related notification that
--  doesn't fit into any other category.</td></tr>
-- <tr><td><b>device.added</b></td><td>A device, such as a USB device, was added to the system.</td></tr>
-- <tr><td><b>device.error</b></td><td>A device had some kind of error.</td></tr>
-- <tr><td><b>device.removed</b></td><td>A device, such as a USB device, was removed from the system.</td></tr>
-- <tr><td><b>email</b></td><td>A generic e-mail-related notification that doesn't fit into
--  any other category.</td></tr>
-- <tr><td><b>email.arrived</b></td><td>A new e-mail notification.</td></tr>
-- <tr><td><b>email.bounced</b></td><td>A notification stating that an e-mail has bounced.</td></tr>
-- <tr><td><b>im</b></td><td>A generic instant message-related notification that doesn't fit into
--  any other category.</td></tr>
-- <tr><td><b>im.error</b></td><td>An instant message error notification.</td></tr>
-- <tr><td><b>im.received</b></td><td>A received instant message notification.</td></tr>
-- <tr><td><b>network</b></td><td>A generic network notification that doesn't fit into any other
--  category.</td></tr>
-- <tr><td><b>network.connected</b></td><td>A network connection notification, such as successful
--  sign-on to a network service. <br />
--  This should not be confused with device.added for new network devices.</td></tr>
-- <tr><td><b>network.disconnected</b></td><td>A network disconnected notification. This should not
--  be confused with <br />
--  device.removed for disconnected network devices.</td></tr>
-- <tr><td><b>network.error</b></td><td>A network-related or connection-related error.</td></tr>
-- <tr><td><b>presence</b></td><td>A generic presence change notification that doesn't fit into any
--  other category, <br />
--  such as going away or idle.</td></tr>
-- <tr><td><b>presence.offline</b></td><td>An offline presence change notification.</td></tr>
-- <tr><td><b>presence.online</b></td><td>An online presence change notification.</td></tr>
-- <tr><td><b>transfer</b></td><td>A generic file transfer or download notification that doesn't
--  fit into any other category.</td></tr>
-- <tr><td><b>transfer.complete</b></td><td>A file transfer or download complete notification.</td></tr>
-- <tr><td><b>transfer.error</b></td><td>A file transfer or download error.</td></tr>
-- </table>
--
-- @property category
-- @tparam string|nil category

--- True if the notification should be kept when an action is pressed.
--
-- By default, invoking an action will destroy the notification. Some actions,
-- like the "Snooze" action of alarm clock, will cause the notification to
-- be updated with a date further in the future.
--
-- @property resident
-- @param[opt=false] boolean

--- Delay in seconds after which hovered popup disappears.
-- @property hover_timeout
-- @param number

--- Target screen for the notification.
-- @property screen
-- @param screen

--- Corner of the workarea displaying the popups.
--
-- The possible values are:
--
-- * *top_right*
-- * *top_left*
-- * *bottom_left*
-- * *bottom_right*
-- * *top_middle*
-- * *bottom_middle*
-- * *middle*
--
--@DOC_awful_notification_box_corner_EXAMPLE@
--
-- @property position
-- @param string
-- @see awful.placement.next_to

--- Boolean forcing popups to display on top.
-- @property ontop
-- @param boolean

--- Popup height.
--
--@DOC_awful_notification_geometry_EXAMPLE@
--
-- @property height
-- @param number
-- @see width

--- Popup width.
-- @property width
-- @param number
-- @see height

--- Notification font.
--@DOC_naughty_colors_EXAMPLE@
-- @property font
-- @param string

--- "All in one" way to access the default image or icon.
--
-- A notification can provide a combination of an icon, a static image, or if
-- enabled, a looping animation. Add to that the ability to import the icon
-- information from the client or from a `.desktop` file, there is multiple
-- conflicting sources of "icons".
--
-- On the other hand, the vast majority of notifications don't multiple or
-- ambiguous sources of icons. This property will pick the first of the
-- following.
--
-- * The `image`.
-- * The `app_icon`.
-- * The `icon` from a client with `normal` type.
-- * The `icon` of a client with `dialog` type.
--
-- @property icon
-- @tparam string|surface icon
-- @see app_icon
-- @see image

--- Desired icon size in px.
-- @property icon_size
-- @param number

--- The icon provided in the `app_icon` field of the DBus notification.
--
-- This should always be either the URI (path) to an icon or a valid XDG
-- icon name to be fetched from the theme.
--
-- @property app_icon
-- @param string

--- The notification image.
--
-- This is usually provided as a `gears.surface` object. The image is used
-- instead of the `app_icon` by notification assets which are auto-generated
-- or stored elsewhere than the filesystem (databases, web, Android phones, etc).
--
-- @property image
-- @tparam string|surface image

--- The notification (animated) images.
--
-- Note that calling this without first setting
-- `naughty.image_animations_enabled` to true will throw an exception.
--
-- Also note that there is *zero* support for this anywhere else in `naughty`
-- and very, very few applications support this.
--
-- @property images
-- @tparam nil|table images

--- Foreground color.
--
--@DOC_awful_notification_fg_EXAMPLE@
--
-- @property fg
-- @tparam string|color|pattern fg
-- @see title
-- @see gears.color

--- Background color.
--
--@DOC_awful_notification_bg_EXAMPLE@
--
-- @property bg
-- @tparam string|color|pattern bg
-- @see title
-- @see gears.color

--- Border width.
-- @property border_width
-- @param number
-- @see title

--- Border color.
--
--@DOC_awful_notification_border_color_EXAMPLE@
--
-- @property border_color
-- @param string
-- @see title
-- @see gears.color

--- Widget shape.
--
-- Note that when using a custom `request::display` handler or `naughty.rules`,
-- choosing between multiple shapes depending on the content can be done using
-- expressions like:
--
--    -- The notification object is called `n`
--    shape = #n.actions > 0 and
--        gears.shape.rounded_rect or gears.shape.rounded_bar,
--
--@DOC_awful_notification_shape_EXAMPLE@
--
--@DOC_naughty_shape_EXAMPLE@
--
-- @property shape
-- @tparam gears.shape shape

--- Widget opacity.
-- @property opacity
-- @tparam number opacity Between 0 to 1.

--- Widget margin.
--
--@DOC_awful_notification_margin_EXAMPLE@
--
-- @property margin
-- @tparam number|table margin
-- @see shape

--- Function to run on left click.
--
-- Use the signals rather than this.
--
-- @deprecatedproperty run
-- @param function
-- @see destroyed

--- Function to run when notification is destroyed.
--
-- Use the signals rather than this.
--
-- @deprecatedproperty destroy
-- @param function
-- @see destroyed

--- Table with any of the above parameters.
-- args will override ones defined
--   in the preset.
-- @property preset
-- @param table

--- Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @property callback
-- @param function

--- A table containing strings that represents actions to buttons.
--
-- The table key (a number) is used by DBus to set map the action.
--
-- @property actions
-- @param table

--- Ignore this notification, do not display.
--
-- Note that this property has to be set in a `preset` or in a `request::preset`
-- handler.
--
-- @property ignore
-- @param boolean

--- Tell if the notification is currently suspended (read only).
--
-- This is always equal to `naughty.suspended`
--@property suspended
--@param boolean

--- If the notification is expired.
-- @property is_expired
-- @param boolean
-- @see naughty.expiration_paused

--- If the timeout needs to be reset when a property changes.
--
-- By default it fallsback to `naughty.auto_reset_timeout`, which itself is
-- true by default.
--
-- @property auto_reset_timeout
-- @tparam[opt=true] boolean auto_reset_timeout

--- Emitted when the notification is destroyed.
-- @signal destroyed
-- @tparam number reason Why it was destroyed
-- @tparam boolean keep_visible If it was kept visible.
-- @see naughty.notification_closed_reason

-- . --FIXME needs a description
-- @property ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.

--- A list of clients associated with this notification.
--
-- When used with DBus notifications, this returns all clients sharing the PID
-- of the notification sender. Note that this is highly unreliable.
-- Applications that use a different process to send the notification or
-- applications (and scripts) calling the `notify-send` command wont have any
-- client.
--
-- @property clients
-- @param table

--FIXME remove the screen attribute, let the handlers decide
-- document all handler extra properties

--- Destroy notification by notification object.
--
-- @method destroy
-- @tparam string reason One of the reasons from `notification_closed_reason`
-- @tparam[opt=false] boolean keep_visible If true, keep the notification visible
-- @return True if the popup was successfully destroyed, false otherwise
function notification:destroy(reason, keep_visible)
    if self._private.is_destroyed then
          gdebug.print_warning("Trying to destroy the same notification twice. It"..
            " was destroyed because: "..self._private.destroy_reason)
          return false
    end

    reason = reason or cst.notification_closed_reason.dismissed_by_user

    self:emit_signal("destroyed", reason, keep_visible)

    self._private.is_destroyed = true
    self._private.destroy_reason = reason

    return true
end

--- Set new notification timeout.
-- @method reset_timeout
-- @tparam number new_timeout Time in seconds after which notification disappears.
function notification:reset_timeout(new_timeout)
    if self.timer then self.timer:stop() end

    self.timeout = new_timeout or self.timeout

    if self.timer and not self.timer.started then
        self.timer:start()
    end
end

function notification:set_id(new_id)
    assert(self._private.id == nil, "Notification identifier can only be set once")
    self._private.id = new_id
    self:emit_signal("property::id", new_id)
end

function notification:set_timeout(timeout)
    local die = function (reason)
        if reason == cst.notification_closed_reason.expired then
            self.is_expired = true
            if naughty.expiration_paused then
                table.insert(naughty.notifications._expired[1], self)
                return
            end
        end

        self:destroy(reason)
    end

    if self.timer and self._private.timeout == timeout then return end

    -- 0 == never
    if timeout > 0 then
        local timer_die = timer { timeout = timeout }

        timer_die:connect_signal("timeout", function()
            pcall(die, cst.notification_closed_reason.expired)

            -- Prevent infinite timers events on errors.
            if timer_die.started then
                timer_die:stop()
            end
        end)

        --FIXME there's still a dependency loop to fix before it works
        if not self.suspended then
            timer_die:start()
        end

        -- Prevent a memory leak and the accumulation of active timers
        if self.timer and self.timer.started then
            self.timer:stop()
        end

        self.timer = timer_die
    end
    self.die = die
    self._private.timeout = timeout
end

function notification:set_text(txt)
    gdebug.deprecate(
        "The `text` attribute is deprecated, use `message`",
        {deprecated_in=5}
    )
    self:set_message(txt)
end

function notification:get_text()
    gdebug.deprecate(
        "The `text` attribute is deprecated, use `message`",
        {deprecated_in=5}
    )
    return self:get_message()
end

local properties = {
    "message" , "title"   , "timeout" , "hover_timeout" ,
    "screen"  , "position", "ontop"   , "border_width"  ,
    "width"   , "font"    , "icon"    , "icon_size"     ,
    "fg"      , "bg"      , "height"  , "border_color"  ,
    "shape"   , "opacity" , "margin"  , "ignore_suspend",
    "destroy" , "preset"  , "callback", "actions"       ,
    "run"     , "id"      , "ignore"  , "auto_reset_timeout",
    "urgency" , "image"   , "images"  ,
}

for _, prop in ipairs(properties) do
    notification["get_"..prop] = notification["get_"..prop] or function(self)
        -- It's possible this could be called from the `request::preset` handler.
        -- `rawget()` is necessary to avoid a stack overflow.
        local preset = rawget(self, "preset")

        return self._private[prop]
            or (preset and preset[prop])
            or cst.config.defaults[prop]
    end

    notification["set_"..prop] = notification["set_"..prop] or function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop, value)

        -- When a notification is updated over dbus or by setting a property,
        -- it is usually convenient to reset the timeout.
        local reset = ((not self.suspended)
            and self.auto_reset_timeout ~= false
            and naughty.auto_reset_timeout)

        if reset then
            self:reset_timeout()
        end
    end

end

local hints_default = {
    urgency  = "normal",
    resident = false,
}

for _, prop in ipairs { "category", "resident" } do
    notification["get_"..prop] = notification["get_"..prop] or function(self)
        return self._private[prop] or (
            self._private.freedesktop_hints and self._private.freedesktop_hints[prop]
        ) or hints_default[prop]
    end

    notification["set_"..prop] = notification["set_"..prop] or function(self, value)
        self._private[prop] = value
        self:emit_signal("property::"..prop, value)
    end
end

function notification.get_icon(self)
    if self._private.icon then
        return self._private.icon == "" and nil or self._private.icon
    elseif self.image and self.image ~= "" then
        return self.image
    elseif self._private.app_icon and self._private.app_icon ~= "" then
        return self._private.app_icon
    end

    local clients = notification.get_clients(self)

    for _, c in ipairs(clients) do
        if c.type == "normal" then
            self._private.icon = gsurface(c.icon)
            return self._private.icon
        end
    end

    for _, c in ipairs(clients) do
        if c.type == "dialog" then
            self._private.icon = gsurface(c.icon)
            return self._private.icon
        end
    end

    return nil
end

function notification.get_clients(self)
    -- Clients from the future don't send notification, it's useless to reload
    -- the list over and over.
    if self._private.clients then return self._private.clients end

    if not self._private._unique_sender then return {} end

    self._private.clients = require("naughty.dbus").get_clients(self)

    return self._private.clients
end

--TODO v6: remove this
local function convert_actions(actions)
    gdebug.deprecate(
        "The notification actions should now be of type `naughty.action`, "..
        "not strings or callback functions",
        {deprecated_in=5}
    )

    local naction = require("naughty.action")

    local new_actions = {}

    -- Does not attempt to handle when there is a mix of strings and objects
    for idx, name in pairs(actions) do
        local cb, old_idx = nil, idx

        if type(name) == "function" then
            cb = name
        end

        if type(idx) == "string" then
            name, idx = idx, #actions+1
        end

        local a = naction {
            position = idx,
            name     = name,
        }

        if cb then
            a:connect_signal("invoked", cb)
        end

        new_actions[old_idx] = a
    end

    -- Yes, it modifies `args`, this is legacy code, cloning the args
    -- just for this isn't worth it.
    for old_idx, a in pairs(new_actions) do
        actions[a.position] = a
        actions[ old_idx  ] = nil
    end
end

--- Create a notification.
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
-- @string[opt=`beautiful.notification_border_color` or
--   `beautiful.border_focus` or `'#535d6c'`] args.border_color Border color.
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
-- @tparam[opt] func args.callback Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @tparam[opt] table args.actions A list of `naughty.action`s.
-- @bool[opt=false] args.ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.
-- @usage naughty.notify({ title = "Achtung!", message = "You're idling", timeout = 0 })
-- @treturn ?table The notification object, or nil in case a notification was
--   not displayed.
-- @constructorfct naughty.notification
local function create(args)
    if cst.config.notify_callback then
        args = cst.config.notify_callback(args)
        if not args then return end
    end

    assert(not args.id, "Identifiers cannot be specified externally")

    args = args or {}

    -- Old actions usually have callbacks and names. But this isn't non
    -- compliant with the spec. The spec has explicit ordering and optional
    -- icons. The old format doesn't allow these metadata to be stored.
    local is_old_action = args.actions and (
        (args.actions[1] and type(args.actions[1]) == "string") or
        (type(next(args.actions)) == "string")
    )

    local n = gobject {
        enable_properties = true,
    }

    if args.text then
        gdebug.deprecate(
            "The `text` attribute is deprecated, use `message`",
            {deprecated_in=5}
        )
        args.message = args.text
    end

    assert(naughty.emit_signal)
    -- Make sure all signals bubble up
    n:_connect_everything(naughty.emit_signal)

    -- Avoid modifying the original table
    local private = {}

    -- gather variables together
    rawset(n, "preset", gtable.join(
        cst.config.defaults or {},
        args.preset or cst.config.presets.normal or {},
        rawget(n, "preset") or {}
    ))

    if is_old_action then
        convert_actions(args.actions)
    end

    for k, v in pairs(n.preset) do
        private[k] = v
    end

    for k, v in pairs(args) do
        private[k] = v
    end

    -- notif.actions should not be nil to allow cheching if there is actions
    -- using the shorthand `if #notif.actions > 0 then`
    private.actions = private.actions or {}

    -- Make sure the action are for this notification. Sharing actions with
    -- multiple notification is not supported.
    for _, a in ipairs(private.actions) do
        a.notification = n
    end

    -- It's an automatic property
    n.is_expired = false

    rawset(n, "_private", private)

    gtable.crush(n, notification, true)

    n.id = n.id or notification._gen_next_id()

    -- Allow extensions to create override the preset with custom data
    naughty.emit_signal("request::preset", n, args)

    -- Register the notification before requesting a widget
    n:emit_signal("new", args)

    -- Let all listeners handle the actual visual aspects
    if (not n.ignore) and (not n.preset.ignore) then
        naughty.emit_signal("request::display" , n, args)
        naughty.emit_signal("request::fallback", n, args)
    end

    -- Because otherwise the setter logic would not be executed
    if n._private.timeout then
        n:set_timeout(n._private.timeout or n.preset.timeout)
    end

    return n
end

-- This allows notification to be updated later.
local counter = 1

-- Identifier support.
function notification._gen_next_id()
    counter = counter+1
    return counter
end

--@DOC_object_COMMON@

return setmetatable(notification, {__call = function(_, ...) return create(...) end})
