----------------------------------------------------------------------------
--- A notification square icon widget.
--
-- This widget is a specialized `wibox.widget.imagebox` with the following extra
-- features:
--
-- * Honor the `beautiful` notification variables.
-- * Restrict the size avoid huge notifications
-- * Provides some strategies to handle small icons
-- * React to the `naughty.notification` object icon changes.
--
--@DOC_wibox_nwidget_icon_simple_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @widgetmod naughty.widget.icon
-- @supermodule wibox.widget.imagebox
-- @see wibox.widget.imagebox
----------------------------------------------------------------------------
local imagebox = require("wibox.widget.imagebox")
local gtable  = require("gears.table")
local beautiful = require("beautiful")
local gsurface = require("gears.surface")
local dpi = require("beautiful.xresources").apply_dpi

local icon = {}

--- The default way to resize the icon.
-- @beautiful beautiful.notification_icon_resize_strategy
-- @param number

function icon:fit(_, width, height)
    -- Until someone complains, adding a "leave blank space" isn't supported
    if not self._private.image then return 0, 0 end

    local maximum  = math.min(width, height)
    local strategy = self._private.resize_strategy or "resize"
    local optimal  = math.min(
        (
            self._private.notification[1] and self._private.notification[1].icon_size
        ) or beautiful.notification_icon_size or dpi(48),
        maximum
    )

    local w = self._private.image:get_width()
    local h = self._private.image:get_height()

    if strategy == "resize" then
        return math.min(w, optimal, maximum), math.min(h, optimal, maximum)
    else
        return optimal, optimal
    end
end

function icon:draw(_, cr, width, height)
    if not self._private.image then return end
    if width == 0 or height == 0 then return end

    -- Let's scale the image so that it fits into (width, height)
    local strategy = self._private.resize_strategy or "resize"
    local w = self._private.image:get_width()
    local h = self._private.image:get_height()
    local aspect = width / w
    local aspect_h = height / h

    if aspect > aspect_h then aspect = aspect_h end

    if aspect < 1 or (strategy == "scale" and (w < width or h < height)) then
        cr:scale(aspect, aspect)
    end

    local x, y = 0, 0

    if (strategy == "center" and aspect < 1) or strategy == "resize" then
        x = math.floor((width  - w*aspect) / 2)
        y = math.floor((height - h*aspect) / 2)
    elseif strategy == "center" and aspect > 1 then
        x = math.floor((width  - w) / 2)
        y = math.floor((height - h) / 2)
    end

    cr:set_source_surface(self._private.image, x, y)
    cr:paint()
end

--- The attached notification.
-- @property notification
-- @tparam naughty.notification notification
-- @propertydefault This is usually set in the construtor.
-- @propemits true false

function icon:set_notification(notif)
    local old = (self._private.notification or {})[1]

    if old == notif then return end

    if old then
        old:disconnect_signal("destroyed",
            self._private.icon_changed_callback)
    end

    local icn = gsurface.load_silently(notif.icon)

    if icn then
        self:set_image(icn)
    end

    self._private.notification = setmetatable({notif}, {__mode="v"})

    notif:connect_signal("property::icon", self._private.icon_changed_callback)
    self:emit_signal("property::notification", notif)
end

local valid_strategies = {
    scale  = true,
    center = true,
    resize = true,
}

--- How small icons are handled.
--
-- Note that the size upper bound is defined by
-- `beautiful.notification_icon_size`.
--
--@DOC_wibox_nwidget_icon_strategy_EXAMPLE@
--
-- @property resize_strategy
-- @tparam string resize_strategy
-- @propemits true false
-- @propertyvalue "scale" Scale the icon up to the optimal size.
-- @propertyvalue "center" Keep the icon size and draw it in the center
-- @propertyvalue "resize" Change the size of the widget itself (*default*).
-- @usebeautiful beautiful.notification_icon_resize_strategy The fallback when
--  there is no specified strategy.
-- @usebeautiful beautiful.notification_icon_size  The size upper bound.


--- The default notification icon size.
-- @beautiful beautiful.notification_icon_size
-- @tparam number notification_icon_size The size (in pixels).

function icon:set_resize_strategy(strategy)
    assert(valid_strategies[strategy], "Invalid strategy")

    self._private.resize_strategy = strategy

    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::resize_strategy", strategy)
end


function icon:get_resize_strategy()
    return self._private.resize_strategy
        or beautiful.notification_icon_resize_strategy
        or "resize"
end

--- Create a new naughty.widget.icon.
-- @tparam table args
-- @tparam naughty.notification args.notification The notification.
-- @constructorfct naughty.widget.icon

local function new(args)
    args = args or {}
    local tb = imagebox()

    gtable.crush(tb, icon, true)
    tb._private.notification = {}

    function tb._private.icon_changed_callback()
        local n = tb._private.notification[1]

        if not n then return end

        local icn = gsurface.load_silently(n.icon)

        if icn then
            tb:set_image(icn)
        end
    end

    if args.notification then
        tb:set_notification(args.notification)
    end

    return tb
end

--@DOC_object_COMMON@

return setmetatable(icon, {__call = function(_, ...) return new(...) end})
