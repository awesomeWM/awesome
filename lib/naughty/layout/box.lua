----------------------------------------------------------------------------
--- A notification popup widget.
--
-- By default, the box is composed of many other widgets:
--
--@DOC_wibox_nwidget_default_EXAMPLE@
--
-- @author Emmanuel Lepage Vallee &lt;elv1313@gmail.com&gt;
-- @copyright 2017 Emmanuel Lepage Vallee
-- @popupmod naughty.layout.box
----------------------------------------------------------------------------

local beautiful  = require("beautiful")
local gtable     = require("gears.table")
local wibox      = require("wibox")
local popup      = require("awful.popup")
local awcommon   = require("awful.widget.common")
local placement  = require("awful.placement")
local abutton    = require("awful.button")

local default_widget = require("naughty.widget._default")

local box, by_position = {}, {}

-- Init the weak tables for each positions. It is done ahead of time rather
-- than when notifications are added to simplify the code.
for _, pos in ipairs { "top_left"   , "top_middle"   , "top_right",
                       "bottom_left", "bottom_middle", "bottom_right" } do
    by_position[pos] = setmetatable({},{__mode = "v"})
end

local function get_spacing()
    local margin = beautiful.notification_spacing or 2
    return {top = margin, bottom = margin}
end

-- Leverage `awful.placement` to create the stacks.
local function update_position(position)
    local pref  = position:match("top_") and "bottom" or "top"
    local align = position:match("_(.*)")
        :gsub("left", "front"):gsub("right", "back")

    for k, wdg in ipairs(by_position[position]) do
        local args = {
            geometry            = by_position[position][k-1],
            preferred_positions = {pref },
            preferred_anchors   = {align},
            margins             = get_spacing(),
            honor_workarea      = true,
        }

        -- The first entry is aligned to the workarea, then the following to the
        -- previous widget.
        placement[k==1 and position:gsub("_middle", "") or "next_to"](wdg, args)

        wdg.visible = true
    end
end

local function finish(self)
    self.visible = false
    assert(by_position[self.position])

    for k, v in ipairs(by_position[self.position]) do
        if v == self then
            table.remove(by_position[self.position], k)
            break
        end
    end

    update_position(self.position)
end

--- The maximum notification width.
-- @beautiful beautiful.notification_max_width
-- @tparam[opt=500] number notification_max_width

--- The maximum notification position.
--
-- Valid values are:
--
-- * top_left
-- * top_middle
-- * top_right
-- * bottom_left
-- * bottom_middle
-- * bottom_right
--
-- @beautiful beautiful.notification_position
-- @tparam[opt="top_right"] string notification_position

--- The widget notification object.
-- @property notification
-- @param naughty.notification

--- The widget template to construct the box content.
--
--@DOC_wibox_nwidget_default_EXAMPLE@
--
-- The default template is (less or more):
--
--    {
--        {
--            {
--                {
--                    {
--                        naughty.widget.icon,
--                        {
--                            naughty.widget.title,
--                            naughty.widget.message,
--                            spacing = 4,
--                            layout  = wibox.layout.fixed.vertical,
--                        },
--                        fill_space = true,
--                        spacing    = 4,
--                        layout     = wibox.layout.fixed.horizontal,
--                    },
--                    naughty.list.actions,
--                    spacing = 10,
--                    layout  = wibox.layout.fixed.vertical,
--                },
--                margins = beautiful.notification_margin,
--                widget  = wibox.container.margin,
--            },
--            id     = "background_role",
--            widget = naughty.container.background,
--        },
--        strategy = "max",
--        width    = width(beautiful.notification_max_width
--            or beautiful.xresources.apply_dpi(500)),
--        widget   = wibox.container.constraint,
--    }
--
-- @property widget_template
-- @param widget

local function generate_widget(args, n)
    local w = wibox.widget.base.make_widget_from_value(
        args.widget_template or default_widget
    )

    -- Call `:set_notification` on all children
    awcommon._set_common_property(w, "notification", n or args.notification)

    return w
end

local function init(self, notification)
    local args = self._private.args

    local preset = notification.preset or {}

    local position = args.position or notification.position or
        beautiful.notification_position or preset.position or "top_right"

    if not self.widget then
        self.widget = generate_widget(self._private.args, notification)
    end

    local bg = self._private.widget:get_children_by_id( "background_role" )[1]

    -- Make sure the border isn't set twice, favor the widget one since it is
    -- shared by the notification list and the notification box.
    if bg then
        if bg.set_notification then
            bg:set_notification(notification)
            self.border_width = 0
        else
            bg:set_bg(notification.bg)
            self.border_width = notification.border_width
        end
    end

    -- Add the notification to the active list
    assert(by_position[position])

    self:_apply_size_now()

    table.insert(by_position[position], self)

    local function update() update_position(position) end

    self:connect_signal("property::geometry", update)
    notification:connect_signal("property::margin", update)
    notification:connect_signal("destroyed", self._private.destroy_callback)

    update_position(position)

end

function box:set_notification(notif)
    if self._private.notification == notif then return end

    if self._private.notification then
        self._private.notification:disconnect_signal("destroyed",
            self._private.destroy_callback)
    end

    init(self, notif)

    self._private.notification = notif
end

function box:get_position()
    if self._private.notification then
        return self._private.notification:get_position()
    end

    return "top_right"
end

local function new(args)
    -- Set the default wibox values
    local new_args = {
        ontop        = true,
        visible      = false,
        bg           = args and args.bg or beautiful.notification_bg,
        fg           = args and args.fg or beautiful.notification_fg,
        shape        = args and args.shape or beautiful.notification_shape,
        border_width = args and args.border_width or beautiful.notification_border_width or 1,
        border_color = args and args.border_color or beautiful.notification_border_color,
    }

    new_args = args and setmetatable(new_args, {__index = args}) or new_args

    -- Generate the box before the popup is created to avoid the size changing
    new_args.widget = generate_widget(new_args)

    local ret = popup(new_args)
    ret._private.args = new_args

    gtable.crush(ret, box, true)

    function ret._private.destroy_callback()
        finish(ret)
    end

    if new_args.notification then
        ret:set_notification(new_args.notification)
    end

    --TODO remove
    local function hide()
        if ret._private.notification then
            ret._private.notification:destroy()
        end
    end

    --FIXME there's another pull request for this
    ret:buttons(gtable.join(
        abutton({ }, 1, hide),
        abutton({ }, 3, hide)
    ))

    return ret
end

--@DOC_wibox_COMMON@

return setmetatable(box, {__call = function(_, args) return new(args) end})
