----------------------------------------------------------------------------
--- A notification background.
--
-- This widget holds the boilerplate code associated with the notification
-- background. This includes the color and potentially some other styling
-- elements such as the shape and border.
--
-- * Honor the `beautiful` notification variables.
-- * React to the `naughty.notification` changes.
--
--@DOC_wibox_nwidget_background_simple_EXAMPLE@
--
-- Note that this widget is based on the `wibox.container.background`. This is
-- an implementation detail and may change in the future without prior notice.
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2019 Emmanuel Lepage Vallee
-- @containermod naughty.widget.background
-- @see wibox.container.background
----------------------------------------------------------------------------
local wbg       = require("wibox.container.background")
local gtable    = require("gears.table")
local beautiful = require("beautiful")
local gshape    = require("gears.shape")

local background = {}

local function update_background(notif, wdg)
    local bg    = notif.bg           or beautiful.notification_bg
    local bw    = notif.border_width or beautiful.notification_border_width
    local bc    = notif.border_color or beautiful.notification_border_color

    -- Always fallback to the rectangle to make sure the border works
    local shape = notif.shape or
        beautiful.notification_shape or gshape.rectangle

    wdg:set_bg(bg)
    wdg:set_shape(shape) -- otherwise there's no borders
    wdg:set_border_width(bw)
    wdg:set_border_color(bc)
end

--- The attached notification.
-- @property notification
-- @tparam naughty.notification notification
-- @propemits true false

function background:set_notification(notif)
    if self._private.notification == notif then return end

    if self._private.notification then
        self._private.notification:disconnect_signal("property::bg",
            self._private.background_changed_callback)
        self._private.notification:disconnect_signal("property::border_width",
            self._private.background_changed_callback)
        self._private.notification:disconnect_signal("property::border_color",
            self._private.background_changed_callback)
        self._private.notification:disconnect_signal("property::shape",
            self._private.background_changed_callback)
    end

    update_background(notif, self)

    self._private.notification = notif

    notif:connect_signal("property::bg"          , self._private.background_changed_callback)
    notif:connect_signal("property::border_width", self._private.background_changed_callback)
    notif:connect_signal("property::border_color", self._private.background_changed_callback)
    notif:connect_signal("property::shape"       , self._private.background_changed_callback)
    self:emit_signal("property::notification", notif)
end

--- Create a new naughty.container.background.
-- @tparam table args
-- @tparam naughty.notification args.notification The notification.
-- @constructorfct naughty.container.background
-- @usebeautiful beautiful.notification_border_width Fallback when the `border_width` property isn't set.
-- @usebeautiful beautiful.notification_border_color Fallback when the `border_color` property isn't set.
-- @usebeautiful beautiful.notification_shape Fallback when the `shape` property isn't set.

local function new(args)
    args = args or {}

    local bg = wbg()
    bg:set_border_strategy("inner")

    gtable.crush(bg, background, true)

    function bg._private.background_changed_callback()
        update_background(bg._private.notification, bg)
    end

    if args.notification then
        bg:set_notification(args.notification)
    end

    return bg
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(background, {__call = function(_, ...) return new(...) end})
