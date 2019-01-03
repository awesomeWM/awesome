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
-- @classmod naughty.notification
---------------------------------------------------------------------------
local gobject = require("gears.object")
local gtable  = require("gears.table")
local timer   = require("gears.timer")
local cst     = require("naughty.constants")
local naughty = require("naughty.core")

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

--- Notifications margin.
-- @beautiful beautiful.notification_margin
-- @tparam int notification_margin

--- Notifications width.
-- @beautiful beautiful.notification_width
-- @tparam int notification_width

--- Notifications height.
-- @beautiful beautiful.notification_height
-- @tparam int notification_height

--- Unique identifier of the notification.
-- This is the equivalent to a PID as allows external applications to select
-- notifications.
-- @property text
-- @param string
-- @see title

--- Text of the notification.
-- @property text
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
--
--@DOC_awful_notification_corner_EXAMPLE@
--
-- @property position
-- @param string

--- Boolean forcing popups to display on top.
-- @property ontop
-- @param boolean

--- Popup height.
-- @property height
-- @param number

--- Popup width.
-- @property width
-- @param number

--- Notification font.
--@DOC_naughty_colors_EXAMPLE@
-- @property font
-- @param string

--- Path to icon.
-- @property icon
-- @tparam string|surface icon

--- Desired icon size in px.
-- @property icon_size
-- @param number

--- Foreground color.
-- @property fg
-- @tparam string|color|pattern fg
-- @see title
-- @see gears.color

--- Background color.
-- @property bg
-- @tparam string|color|pattern bg
-- @see title
-- @see gears.color

--- Border width.
-- @property border_width
-- @param number
-- @see title

--- Border color.
-- @property border_color
-- @param string
-- @see title
-- @see gears.color

--- Widget shape.
--@DOC_naughty_shape_EXAMPLE@
-- @property shape

--- Widget opacity.
-- @property opacity
-- @param number From 0 to 1

--- Widget margin.
-- @property margin
-- @tparam number|table margin
-- @see shape

--- Function to run on left click.
-- @property run
-- @param function

--- Function to run when notification is destroyed.
-- @property destroy
-- @param function

--- Table with any of the above parameters.
-- args will override ones defined
--   in the preset.
-- @property preset
-- @param table

--- Replace the notification with the given ID.
-- @property replaces_id
-- @param number

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

--- Emitted when the notification is destroyed.
-- @signal destroyed
-- @tparam number reason Why it was destroyed
-- @tparam boolean keep_visible If it was kept visible.
-- @see naughty.notification_closed_reason

-- . --FIXME needs a description
-- @property ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.

--FIXME remove the screen attribute, let the handlers decide
-- document all handler extra properties

--FIXME add methods such as persist

--- Destroy notification by notification object
--
-- @tparam string reason One of the reasons from `notification_closed_reason`
-- @tparam[opt=false] boolean keep_visible If true, keep the notification visible
-- @return True if the popup was successfully destroyed, nil otherwise
function notification:destroy(reason, keep_visible)
    self:emit_signal("destroyed", reason, keep_visible)

    return true
end

--- Set new notification timeout.
-- @tparam number new_timeout Time in seconds after which notification disappears.
function notification:reset_timeout(new_timeout)
    if self.timer then self.timer:stop() end

    self.timeout = new_timeout or self.timeout

    if not self.timer.started then
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

local properties = {
    "text"    , "title"   , "timeout" , "hover_timeout" ,
    "screen"  , "position", "ontop"   , "border_width"  ,
    "width"   , "font"    , "icon"    , "icon_size"     ,
    "fg"      , "bg"      , "height"  , "border_color"  ,
    "shape"   , "opacity" , "margin"  , "ignore_suspend",
    "destroy" , "preset"  , "callback", "replaces_id"   ,
    "actions" , "run"     , "id"      , "ignore"        ,
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
        return
    end

end

--TODO v6: remove this
local function convert_actions(actions)
    gdebug.deprecate(
        "The notification actions should now be of type `naughty.action`, "..
        "not strings or callback functions",
        {deprecated_in=5}
    )

    local naction = require("naughty.action")

    -- Does not attempt to handle when there is a mix of strings and objects
    for idx, name in pairs(actions) do
        local cb = nil

        if type(name) == "function" then
            cb = name
        end

        if type(idx) == "string" then
            name, idx = idx, nil
        end

        local a = naction {
            position = idx,
            name     = name,
        }

        if cb then
            a:connect_signal("invoked", cb)
        end

        -- Yes, it modifies `args`, this is legacy code, cloning the args
        -- just for this isn't worth it.
        actions[idx] = a
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
-- @tparam[opt] int args.replaces_id Replace the notification with the given ID.
-- @tparam[opt] func args.callback Function that will be called with all arguments.
--   The notification will only be displayed if the function returns true.
--   Note: this function is only relevant to notifications sent via dbus.
-- @tparam[opt] table args.actions A list of `naughty.action`s.
-- @bool[opt=false] args.ignore_suspend If set to true this notification
--   will be shown even if notifications are suspended via `naughty.suspend`.
-- @usage naughty.notify({ title = "Achtung!", text = "You're idling", timeout = 0 })
-- @treturn ?table The notification object, or nil in case a notification was
--   not displayed.
-- @function naughty.notification
local function create(args)
    if cst.config.notify_callback then
        args = cst.config.notify_callback(args)
        if not args then return end
    end

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

    if is_old_action  then
        convert_actions(args.actions)
    end

    for k, v in pairs(n.preset) do
        private[k] = v
    end

    for k, v in pairs(args) do
        private[k] = v
    end

    -- It's an automatic property
    n.is_expired = false

    rawset(n, "_private", private)

    gtable.crush(n, notification, true)

    -- Allow extensions to create override the preset with custom data
    naughty.emit_signal("request::preset", n, args)

    -- Register the notification before requesting a widget
    n:emit_signal("new", args)

    -- Let all listeners handle the actual visual aspects
    if (not n.ignore) and (not n.preset.ignore) then
        naughty.emit_signal("request::display", n, args)
    end

    -- Because otherwise the setter logic would not be executed
    if n._private.timeout then
        n:set_timeout(n._private.timeout or n.preset.timeout)
    end

    return n
end

return setmetatable(notification, {__call = function(_, ...) return create(...) end})
