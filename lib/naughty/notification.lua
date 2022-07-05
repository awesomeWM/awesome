---------------------------------------------------------------------------
--- Notification manipulation class.
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
local capi     = { screen = screen }
local gobject  = require("gears.object")
local gtable   = require("gears.table")
local timer    = require("gears.timer")
local gfs      = require("gears.filesystem")
local cst      = require("naughty.constants")
local naughty  = require("naughty.core")
local gdebug   = require("gears.debug")
local pcommon = require("awful.permissions._common")

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
-- @tparam number id
-- @propemits true false

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
-- @tparam string title
-- @propemits true false

--- Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @property timeout
-- @tparam number timeout
-- @propemits true false

--- The notification urgency level.
--
-- The default urgency levels are:
--
-- * low
-- * normal
-- * critical
--
-- @property urgency
-- @tparam string urgency
-- @propemits true false

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
-- @propemits true false

--- True if the notification should be kept when an action is pressed.
--
-- By default, invoking an action will destroy the notification. Some actions,
-- like the "Snooze" action of alarm clock, will cause the notification to
-- be updated with a date further in the future.
--
-- @property resident
-- @tparam[opt=false] boolean resident
-- @propemits true false

--- Delay in seconds after which hovered popup disappears.
-- @property hover_timeout
-- @tparam[opt=nil] number|nil hover_timeout
-- @propemits true false

--- Target screen for the notification.
-- @property screen
-- @tparam screen screen
-- @propemits true false

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
-- @tparam[opt=beautiful.notification_position] string position
-- @propemits true false
-- @see awful.placement.next_to

--- Boolean forcing popups to display on top.
-- @property ontop
-- @tparam[opt=false] boolean ontop

--- Popup height.
--
--@DOC_awful_notification_geometry_EXAMPLE@
--
-- @property height
-- @tparam number height
-- @propemits true false
-- @see width

--- Popup width.
-- @property width
-- @tparam number width
-- @propemits true false
-- @see height

--- Notification font.
--@DOC_naughty_colors_EXAMPLE@
-- @property font
-- @tparam string font
-- @propemits true false
-- @see wibox.widget.textbox.font

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
-- @propemits true false
-- @see app_icon
-- @see image

--- Desired icon size in px.
-- @property icon_size
-- @tparam[opt=beautiful.notification_icon_size] number icon_size
-- @propemits true false

--- The icon provided in the `app_icon` field of the DBus notification.
--
-- This should always be either the URI (path) to an icon or a valid XDG
-- icon name to be fetched from the theme.
--
-- @property app_icon
-- @tparam string app_icon
-- @propemits true false

--- The notification image.
--
-- This is usually provided as a `gears.surface` object. The image is used
-- instead of the `app_icon` by notification assets which are auto-generated
-- or stored elsewhere than the filesystem (databases, web, Android phones, etc).
--
-- @property image
-- @tparam string|surface image
-- @propemits true false

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
-- @propemits true false

--- Foreground color.
--
--@DOC_awful_notification_fg_EXAMPLE@
--
-- @property fg
-- @tparam[beautiful.notification_fg] string|color|pattern fg
-- @propemits true false
-- @see title
-- @see gears.color

--- Background color.
--
--@DOC_awful_notification_bg_EXAMPLE@
--
-- @property bg
-- @tparam[opt=beautiful.notification_bg] string|color|pattern bg
-- @propemits true false
-- @see title
-- @see gears.color

--- Border width.
-- @property border_width
-- @tparam[opt=beautiful.notification_border_width or 0] number border_width
-- @propemits true false

--- Border color.
--
--@DOC_awful_notification_border_color_EXAMPLE@
--
-- @property border_color
-- @tparam[opt=beautiful.notification_border_color] string border_color
-- @propemits true false
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
-- @propemits true false

--- Widget opacity.
-- @property opacity
-- @tparam number opacity Between 0 to 1.
-- @propemits true false

--- Widget margin.
--
--@DOC_awful_notification_margin_EXAMPLE@
--
-- @property margin
-- @tparam number|table margin
-- @propemits true false
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
-- @tparam table preset
-- @propemits true false

--- Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @property callback
-- @tparam function callback
-- @propemits true false

--- A table containing strings that represents actions to buttons.
--
-- The table key (a number) is used by DBus to set map the action.
--
-- @property actions
-- @tparam table actions
-- @propemits true false

--- Ignore this notification, do not display.
--
-- Note that this property has to be set in a `preset` or in a `request::preset`
-- handler.
--
-- @property ignore
-- @tparam[opt=false] boolean ignore
-- @propemits true false

--- Tell if the notification is currently suspended (read only).
--
-- This is always equal to `naughty.suspended`
-- @property suspended
-- @tparam[opt=false] boolean suspended
-- @propemits true false
-- @see naughty.suspended

--- If the notification is expired.
-- @property is_expired
-- @tparam boolean is_expired
-- @propemits true false
-- @see naughty.expiration_paused

--- If the timeout needs to be reset when a property changes.
--
-- By default it fallsback to `naughty.auto_reset_timeout`, which itself is
-- true by default.
--
-- @property auto_reset_timeout
-- @tparam[opt=true] boolean auto_reset_timeout
-- @propemits true false

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
-- @tparam table clients

--- The maximum popup width.
--
-- Some notifications have overlong message, cap them to this width. Note that
-- this is ignored by `naughty.list.notifications` because it delegate this
-- decision to the layout.
--
-- @property max_width
-- @tparam[opt=500] number max_width
-- @propemits true false

--- The application name specified by the notification.
--
-- This can be anything. It is usually less relevant than the `clients`
-- property, but can sometime be specified for remote or headless notifications.
-- In these case, it helps to triage and detect the notification from the rules.
-- @property app_name
-- @tparam string app_name
-- @propemits true false

--- The widget template used to represent the notification.
--
-- Some notifications, such as chat messages or music applications are better
-- off with a specialized notification widget.
--
-- @property widget_template
-- @tparam table|nil widget_template
-- @propemits true false

--- Destroy notification by notification object.
--
-- @method destroy
-- @tparam string reason One of the reasons from `notification_closed_reason`
-- @tparam[opt=false] boolean keep_visible If true, keep the notification visible
-- @treturn boolean True if the popup was successfully destroyed, false otherwise.
-- @emits destroyed
-- @emitstparam destroyed integer reason The reason.
-- @emitstparam destroyed boolean keep_visible If the notification should be kept.
-- @see naughty.notification_closed_reason
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
-- @noreturn
function notification:reset_timeout(new_timeout)
    if self.timer then self.timer:stop() end

    -- Do not set `self.timeout` to `self.timeout` since that would create the
    -- timer before the constructor ends.
    if new_timeout and self.timer then
        self.timeout = new_timeout
    elseif new_timeout and new_timeout ~= self._private.timeout then
        local previous_timer = self._private.timeout
        timer.delayed_call(function()
            if self._private.timeout == previous_timer then
                self.timeout = new_timeout
            end
        end)
    end

    if self.timer and not self.timer.started then
        self.timer:start()
    end
end

function notification:set_id(new_id)
    assert(self._private.id == nil, "Notification identifier can only be set once")
    self._private.id = new_id
    self:emit_signal("property::id", new_id)
end

-- Return true if `self` is suspended.
local function get_suspended(self)
    return naughty.suspended and (not self._private.ignore_suspend)
end

function notification:set_timeout(timeout)
    timeout = timeout or 0

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
        if not get_suspended(self) then
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
    self:emit_signal("property::timeout", timeout)
end

function notification:get_message()
    -- This property was called "text" in older versions.
    -- Some modules like `lain` abused of the presets (they
    -- had little choice at the time) to set the message on
    -- an existing popup.
    local p = rawget(self, "preset") or {}
    local message = self._private.message or p.message or ""

    if message == "" and p.text and p.text ~= "" then
        gdebug.deprecate(
            "Using the preset configuration to set the notification "..
            "message is not supported. Please use `n.message = 'text'`.",
            {deprecated_in=5}
        )

        return p.text
    end

    return message
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
    "message"  , "title"   , "timeout" , "hover_timeout"     ,
    "app_name" , "position", "ontop"   , "border_width"      ,
    "width"    , "font"    , "icon"    , "icon_size"         ,
    "fg"       , "bg"      , "height"  , "border_color"      ,
    "shape"    , "opacity" , "margin"  , "ignore_suspend"    ,
    "destroy"  , "preset"  , "callback", "actions"           ,
    "run"      , "id"      , "ignore"  , "auto_reset_timeout",
    "urgency"  , "image"   , "images"  , "widget_template"   ,
    "max_width", "app_icon",
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
        local reset = ((not self.suspended) or self._private.ignore_suspend)
            and self.auto_reset_timeout ~= false
            and naughty.auto_reset_timeout

        if reset then
            self:reset_timeout()
        end
    end

end

-- Changing the image will change the icon, make sure property::icon is emitted.
for _, prop in ipairs {"image", "images" } do
    local cur = notification["set_"..prop]

    notification["set_"..prop] = function(self, value)
        cur(self, value)
        self._private.icon = nil
        self:emit_signal("property::icon")
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

-- Stop the request::icon when one is found.
local function request_filter(self, context, _)
    if not pcommon.check(self, "notification", "icon", context) then return true end
    if self._private.icon then return true end
end

-- Convert encoded local URI to Unix paths.
local function check_path(input)
    if type(input) ~= "string" then return nil end

    if input:sub(1,7) == "file://" then
        input = input:sub(8)
    end

    -- urldecode
    input = input:gsub("%%(%x%x)", function(x) return string.char(tonumber(x, 16)) end )

    return gfs.file_readable(input) and input or nil
end

function notification.get_icon(self)
    -- Honor all overrides.
    if self._private.icon then
        return self._private.icon == "" and nil or self._private.icon
    end

    -- First, check if the image is passed as a surface or a path.
    if self.image and self.image ~= "" then
        naughty._emit_signal_if("request::icon", request_filter, self, "image", {
            image = self.image
        })
    elseif self.images then
        naughty._emit_signal_if("request::icon", request_filter, self, "images", {
            images = self.images
        })
    end

    if self._private.icon then
        return self._private.icon == "" and nil or self._private.icon
    end

    -- Second level of fallback, icon paths.
    local path = check_path(self._private.app_icon)

    if path then
        naughty._emit_signal_if("request::icon", request_filter, self, "path", {
            path = path
        })
    end

    if self._private.icon then
        return self._private.icon == "" and nil or self._private.icon
    end

    -- Third level fallback is `app_icon`.
    if self._private.app_icon then
        naughty._emit_signal_if("request::icon", request_filter, self, "app_icon", {
            app_icon = self._private.app_icon
        })
    end

    -- Finally, the clients.
    naughty._emit_signal_if("request::icon", request_filter, self, "clients", {})

    return self._private.icon == "" and nil or self._private.icon
end

function notification.get_clients(self)
    -- Clients from the future don't send notification, it's useless to reload
    -- the list over and over.
    if self._private.clients then return self._private.clients end

    if not self._private._unique_sender then return {} end

    self._private.clients = require("naughty.dbus").get_clients(self)

    return self._private.clients
end

function notification.set_actions(self, new_actions)
    for _, a in ipairs(self._private.actions or {}) do
        a:disconnect_signal("_changed", self._private.action_cb )
        a:disconnect_signal("invoked" , self._private.invoked_cb)
    end

    -- Clone so `append_actions` doesn't add unwanted actions to other
    -- notifications.
    self._private.actions = gtable.clone(new_actions, false)

    for _, a in ipairs(self._private.actions or {}) do
        a:connect_signal("_changed", self._private.action_cb )
        a:connect_signal("invoked" , self._private.invoked_cb)
    end

    self:emit_signal("property::actions", new_actions)

    -- When a notification is updated over dbus or by setting a property,
    -- it is usually convenient to reset the timeout.
    local reset = (not get_suspended(self))
        and self.auto_reset_timeout ~= false
        and naughty.auto_reset_timeout

    if reset then
        self:reset_timeout()
    end
end

--- Add more actions to the notification.
-- @method append_actions
-- @tparam table new_actions
-- @noreturn

function notification:append_actions(new_actions)
    self._private.actions = self._private.actions or {}

    for _, a in ipairs(new_actions or {}) do
        a:connect_signal("_changed", self._private.action_cb )
        a:connect_signal("invoked" , self._private.invoked_cb)
        table.insert(self._private.actions, a)
    end

end

function notification:set_screen(s)
    assert(not self._private.screen)

    s = s and capi.screen[s] or nil

    -- Avoid an infinite loop in the management code.
    if s == self._private.weak_screen[1] then return end

    self._private.weak_screen = setmetatable({s}, {__mode="v"})

    self:emit_signal("property::screen", s)
end

function notification:get_screen()
    return self._private.weak_screen[1]
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

-- The old API used monkey-patched variable presets.
--
-- Monkey-patched anything is always an issue and prevent module from safely
-- doing anything without stepping on each other foot. In the background,
-- presets were selected with a rule-like API anyway.
local function select_legacy_preset(n, args)
    for _, obj in pairs(cst.config.mapping) do
        local filter, preset = obj[1], obj[2]
        if (not filter.urgency or filter.urgency == args.urgency) and
        (not filter.category or filter.category == args.category) and
        (not filter.appname or filter.appname == args.appname) then
            args.preset = gtable.join(args.preset or {}, preset)
        end
    end

    -- gather variables together
    rawset(n, "preset", gtable.join(
        cst.config.defaults or {},
        args.preset or cst.config.presets.normal or {},
        rawget(n, "preset") or {}
    ))

    for k, v in pairs(n.preset) do
        -- Don't keep a strong reference to the screen, Lua 5.1 GC wont be
        -- smart enough to unwind the mess of circular weak references.
        if k ~= "screen" then
            n._private[k] = v
        end
    end

    if n.preset.screen then
        n._private.weak_screen[1] = capi.screen[n.preset.screen]
    end
end

--- Create a notification.
--
-- @tparam[opt={}] table args The argument table containing any of the arguments below.
-- @tparam[opt=""] string args.text Text of the notification.
-- @tparam[opt] string args.title Title of the notification.
-- @tparam[opt=5] integer args.timeout Time in seconds after which popup expires.
--   Set 0 for no timeout.
-- @tparam[opt] number args.hover_timeout Delay in seconds after which hovered popup disappears.
-- @tparam[opt=focused] integer|screen args.screen Target screen for the notification.
-- @tparam[opt="top_right"] string args.position Corner of the workarea displaying the popups.
--   Values: `"top_right"`, `"top_left"`, `"bottom_left"`,
--   `"bottom_right"`, `"top_middle"`, `"bottom_middle"`, `"middle"`.
-- @tparam[opt=true] boolean args.ontop Boolean forcing popups to display on top.
-- @tparam[opt=`beautiful.notification_height` or auto] integer args.height Popup height.
-- @tparam[opt=`beautiful.notification_width` or auto] integer args.width Popup width.
-- @tparam[opt=`beautiful.notification_font` or `beautiful.font` or `awesome.font`] string|lgi.Pango.FontDescription args.font Notification font.
-- @tparam[opt] gears.surface args.icon Path to icon.
-- @tparam[opt] integer args.icon_size Desired icon size in px.
-- @tparam[opt=`beautiful.notification_fg` or `beautiful.fg_focus` or `'#ffffff'`] string args.fg Foreground color.
-- @tparam[opt=`beautiful.notification_fg` or `beautiful.bg_focus` or `'#535d6c'`] string args.bg Background color.
-- @tparam[opt=`beautiful.notification_border_width` or 1] integer args.border_width Border width.
-- @tparam[opt=`beautiful.notification_border_color` or `beautiful.border_color_active` or `'#535d6c'`] gears.color args.border_color Border color.
-- @tparam[opt=beautiful.notification_shape] gears.shape args.shape Widget shape.
-- @tparam[opt=beautiful.notification_opacity] gears.opacity args.opacity Widget opacity.
-- @tparam[opt=beautiful.notification_margin] gears.margin args.margin Widget margin.
-- @tparam[opt] function args.run Function to run on left click.  The notification
--   object will be passed to it as an argument.
--   You need to call e.g.
--   `notification.die(naughty.notification_closed_reason.dismissedByUser)` from
--   there to dismiss the notification yourself.
-- @tparam[opt] function args.destroy Function to run when notification is destroyed.
-- @tparam[opt] table args.preset Table with any of the above parameters.
--   Note: Any parameters specified directly in args will override ones defined
--   in the preset.
-- @tparam[opt] function args.callback Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @tparam[opt] table args.actions A list of `naughty.action`s.
-- @tparam[opt=false] boolean args.ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.
-- @treturn naughty.notification A new notification object.
-- @constructorfct naughty.notification
-- @usage naughty.notification {
--     title   = "Achtung!",
--     message = "You're idling", timeout = 0
-- }
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
    local private = {weak_screen = setmetatable({}, {__mode="v"})}
    rawset(n, "_private", private)

    -- Allow extensions to create override the preset with custom data
    if not naughty._has_preset_handler then
        select_legacy_preset(n, args)
    end

    if is_old_action then
        convert_actions(args.actions)
    end

    for k, v in pairs(args) do
        -- Don't keep a strong reference to the screen, Lua 5.1 GC wont be
        -- smart enough to unwind the mess of circular weak references.
        if k ~= "screen" then
            private[k] = v
        end
    end

    -- It's an automatic property
    n.is_expired = false

    gtable.crush(n, notification, true)

    -- Always emit property::actions when any of the action change to allow
    -- some widgets to be updated without over complicated built-in tracking
    -- of all options.
    function n._private.action_cb() n:emit_signal("property::actions") end

    -- Listen to action press and destroy non-resident notifications.
    function n._private.invoked_cb(a, notif)
        if (not notif) or notif == n then
            n:emit_signal("invoked", a)

            if not n.resident then
                n:destroy(cst.notification_closed_reason.dismissed_by_user)
            end
        end
    end

    -- notif.actions should not be nil to allow checking if there is actions
    -- using the shorthand `if #notif.actions > 0 then`
    private.actions = {}
    if args.actions then
        notification.set_actions(n, args.actions)
    end

    n.id = n.id or notification._gen_next_id()

    -- The rules are attached to this.
    if naughty._has_preset_handler then
        naughty.emit_signal("request::preset", n, "new", args)
    end

    -- Register the notification before requesting a widget
    n:emit_signal("new", args)

    -- Let all listeners handle the actual visual aspects
    if (not n.ignore) and ((not n.preset) or n.preset.ignore ~= true) and (not get_suspended(n)) then
        naughty.emit_signal("request::display" , n, "new", args)
        naughty.emit_signal("request::fallback", n, "new", args)
    end

    -- Because otherwise the setter logic would not be executed
    if n._private.timeout then
        n:set_timeout(n._private.timeout
            or (n.preset and n.preset.timeout)
            or cst.config.timeout
        )
    end

    return n
end

--- Grant a permission for a notification.
--
-- @method grant
-- @tparam string permission The permission name (just the name, no `request::`).
-- @tparam string context The reason why this permission is requested.
-- @noreturn
-- @see awful.permissions

--- Deny a permission for a notification
--
-- @method deny
-- @tparam string permission The permission name (just the name, no `request::`).
-- @tparam string context The reason why this permission is requested.
-- @noreturn
-- @see awful.permissions

pcommon.setup_grant(notification, "notification")

-- This allows notification to be updated later.
local counter = 1

-- Identifier support.
function notification._gen_next_id()
    counter = counter+1
    return counter
end

--@DOC_object_COMMON@

return setmetatable(notification, {__call = function(_, ...) return create(...) end})
