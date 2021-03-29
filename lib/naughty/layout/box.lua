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
-- @supermodule awful.popup
----------------------------------------------------------------------------

local capi       = {screen=screen}
local beautiful  = require("beautiful")
local gtimer     = require("gears.timer")
local gtable     = require("gears.table")
local wibox      = require("wibox")
local popup      = require("awful.popup")
local awcommon   = require("awful.widget.common")
local placement  = require("awful.placement")
local abutton    = require("awful.button")
local ascreen    = require("awful.screen")
local gpcall     = require("gears.protected_call")
local dpi        = require("beautiful").xresources.apply_dpi

local default_widget = require("naughty.widget._default")

local box, by_position = {}, {}

-- Init the weak tables for each positions. It is done ahead of time rather
-- than when notifications are added to simplify the code.

local function init_screen(s)
    if not s.valid then return end

    if by_position[s] then return by_position[s] end

    by_position[s] = setmetatable({},{__mode = "k"})

    for _, pos in ipairs { "top_left"   , "top_middle"   , "top_right",
                           "bottom_left", "bottom_middle", "bottom_right" } do
        by_position[s][pos] = setmetatable({},{__mode = "v"})
    end

    return by_position[s]
end

ascreen.connect_for_each_screen(init_screen)

-- Manually cleanup to help the GC.
capi.screen.connect_signal("removed", function(scr)
    -- By that time, all direct events should have been handled. Cleanup the
    -- leftover. Being a weak table doesn't help Lua 5.1.
    gtimer.delayed_call(function()
        by_position[scr] = nil
    end)
end)

local function get_spacing()
    local margin = beautiful.notification_spacing or dpi(2)
    return {top = margin, bottom = margin}
end

local function get_offset(position, preset)
    preset = preset or {}
    local margin = preset.padding or beautiful.notification_spacing or dpi(4)
    if position:match('_right') then
        return {x = -margin}
    elseif position:match('_left') then
        return {x = margin}
    end
    return {}
end

-- Leverage `awful.placement` to create the stacks.
local function update_position(position, preset)
    local pref  = position:match("top_") and "bottom" or "top"
    local align = position:match("_(.*)")
        :gsub("left", "front"):gsub("right", "back")

    for _, pos in pairs(by_position) do
        for k, wdg in ipairs(pos[position]) do
            local args = {
                geometry            = pos[position][k-1],
                preferred_positions = {pref },
                preferred_anchors   = {align},
                margins             = get_spacing(),
                honor_workarea      = true,
            }
            if k == 1 then
                args.offset              = get_offset(position, preset)
            end

            -- The first entry is aligned to the workarea, then the following to the
            -- previous widget.
            placement[k==1 and position:gsub("_middle", "") or "next_to"](wdg, args)

            wdg.visible = true
        end
    end
end

local function finish(self)
    self.visible = false
    assert(init_screen(self.screen)[self.position])

    for k, v in ipairs(init_screen(self.screen)[self.position]) do
        if v == self then
            table.remove(init_screen(self.screen)[self.position], k)
            break
        end
    end

    local preset
    if self.private and self.private.args and self.private.args.notification then
        preset = self.private.args.notification.preset
    end

    update_position(self.position, preset)
end

-- It isn't a good idea to use the `attach` `awful.placement` property. If the
-- screen is resized or the notification is moved, it causes side effects.
-- Better listen to geometry changes and reflow.
capi.screen.connect_signal("property::geometry", function(s)
    for pos, notifs in pairs(by_position[s]) do
        if #notifs > 0 then
            update_position(pos, notifs[1].preset)
        end
    end
end)

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
--
-- @property notification
-- @tparam naughty.notification notification
-- @propemits true false

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
-- @usebeautiful beautiful.notification_max_width The maximum width for the
--  resulting widget.

local function generate_widget(args, n)
    local w = gpcall(wibox.widget.base.make_widget_from_value,
        args.widget_template or (n and n.widget_template) or default_widget
    )

    -- This will happen if the user-provided widget_template is invalid and/or
    -- got unexpected notifications.
    if not w then
        w = gpcall(wibox.widget.base.make_widget_from_value, default_widget)

        -- In case this happens in an error message itself, make sure the
        -- private error popup code knowns it and can revert to the fallback
        -- popup.
        if not w then
            n._private.widget_template_failed = true
        end

        return nil
    end

    if w.set_width then
        w:set_width(n.max_width or beautiful.notification_max_width or dpi(500))
    end

    -- Call `:set_notification` on all children
    awcommon._set_common_property(w, "notification", n or args.notification)

    return w
end

local function init(self, notification)
    local args = self._private.args

    local preset = notification.preset or {}

    local position = args.position or notification.position or
        preset.position or beautiful.notification_position or "top_right"

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

    local s = notification.screen
    assert(s)

    -- Add the notification to the active list
    assert(init_screen(s)[position], "Invalid position "..position)

    self:_apply_size_now()

    table.insert(init_screen(s)[position], self)

    local function update() update_position(position, preset) end

    self:connect_signal("property::geometry", update)
    notification:connect_signal("property::margin", update)
    notification:connect_signal("destroyed", self._private.destroy_callback)

    update_position(position, preset)

end

function box:set_notification(notif)
    if self._private.notification == notif then return end

    if self._private.notification then
        self._private.notification:disconnect_signal("destroyed",
            self._private.destroy_callback)
    end

    init(self, notif)

    self._private.notification = notif

    self:emit_signal("property::notification", notif)
end

function box:get_position()
    if self._private.notification then
        return self._private.notification:get_position()
    end

    return "top_right"
end

--- Create a notification popup box.
--
-- @constructorfct naughty.layout.box
-- @tparam[opt=nil] table args
-- @tparam table args.widget_template A widget definition template which will
--  be instantiated for each box.
-- @tparam naughty.notification args.notification The notification object.
-- @tparam string args.position The position. See `naughty.notification.position`.
--@DOC_wibox_constructor_COMMON@
-- @usebeautiful beautiful.notification_position If `position` is not defined
-- in the notification object (or in this constructor).

local function new(args)
    args = args or {}

    -- Set the default wibox values
    local new_args = {
        ontop        = true,
        visible      = false,
        bg           = args.bg or beautiful.notification_bg,
        fg           = args.fg or beautiful.notification_fg,
        shape        = args.shape or beautiful.notification_shape,
        border_width = args.border_width or beautiful.notification_border_width or 1,
        border_color = args.border_color or beautiful.notification_border_color,
    }

    -- The C code needs `pairs` to work, so a full copy is required.
    gtable.crush(new_args, args, true)

    -- Add a weak-table layer for the screen.
    local weak_args = setmetatable({
        screen = args.notification and args.notification.screen or nil
    }, {__mode="v"})

    setmetatable(new_args, {__index = weak_args})

    -- Generate the box before the popup is created to avoid the size changing
    new_args.widget = generate_widget(new_args, new_args.notification)

    -- It failed, request::fallback will be used, there is nothing left to do.
    if not new_args.widget then return nil end

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

    gtable.crush(ret, box, false)

    return ret
end

--@DOC_wibox_COMMON@

return setmetatable(box, {__call = function(_, args) return new(args) end})
