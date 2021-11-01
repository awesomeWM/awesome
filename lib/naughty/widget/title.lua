----------------------------------------------------------------------------
--- A notification title.
--
-- This widget is a specialized `wibox.widget.textbox` with the following extra
-- features:
--
-- * Honor the `beautiful` notification variables.
-- * React to the `naughty.notification` object title changes.
--
--@DOC_wibox_nwidget_title_simple_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @widgetmod naughty.widget.title
-- @see wibox.widget.textbox
----------------------------------------------------------------------------
local textbox = require("wibox.widget.textbox")
local gtable  = require("gears.table")
local beautiful = require("beautiful")
local markup  = require("naughty.widget._markup").set_markup

local title = {}

--- The attached notification.
-- @property notification
-- @tparam naughty.notification notification
-- @propemits true false

function title:set_notification(notif)
    local old = self._private.notification[1]

    if old == notif then return end

    if old then
        old:disconnect_signal("property::message",
            self._private.title_changed_callback)
        old:disconnect_signal("property::fg",
            self._private.title_changed_callback)
    end

    markup(self, notif.title, notif.fg, notif.font)

    self._private.notification = setmetatable({notif}, {__mode="v"})
    self._private.title_changed_callback()

    notif:connect_signal("property::title", self._private.title_changed_callback)
    notif:connect_signal("property::fg"   , self._private.title_changed_callback)
    self:emit_signal("property::notification", notif)
end

--- Create a new naughty.widget.title.
-- @tparam table args
-- @tparam naughty.notification args.notification The notification.
-- @constructorfct naughty.widget.title
-- @usebeautiful beautiful.notification_fg
-- @usebeautiful beautiful.notification_font

local function new(args)
    args = args or {}
    local tb = textbox()
    tb:set_wrap("word")
    tb:set_font(beautiful.notification_font)
    tb._private.notification = {}

    gtable.crush(tb, title, true)

    function tb._private.title_changed_callback()
        local n = tb._private.notification[1]

        if n then
            markup(
                tb,
                n.title,
                n.fg,
                n.font
            )
        else
            markup("", nil, nil)
        end
    end

    if args.notification then
        tb:set_notification(args.notification)
    end

    return tb
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(title, {__call = function(_, ...) return new(...) end})
