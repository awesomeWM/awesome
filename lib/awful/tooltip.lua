-------------------------------------------------------------------------
--- Tooltip module for awesome objects.
--
-- A tooltip is a small hint displayed when the mouse cursor
-- hovers a specific item.
-- In awesome, a tooltip can be linked with almost any
-- object having a `:connect_signal()` method and receiving
-- `mouse::enter` and `mouse::leave` signals.
--
-- How to create a tooltip?
-- ---
--
--     myclock = awful.widget.textclock({}, "%T", 1)
--     myclock_t = awful.tooltip({
--         objects = { myclock },
--         timer_function = function()
--                 return os.date("Today is %A %B %d %Y\nThe time is %T")
--             end,
--         })
--
-- How to add the same tooltip to several objects?
-- ---
--
--     myclock_t:add_to_object(obj1)
--     myclock_t:add_to_object(obj2)
--
-- Now the same tooltip is attached to `myclock`, `obj1`, `obj2`.
--
-- How to remove tooltip from many objects?
-- ---
--
--     myclock_t:remove_from_object(obj1)
--     myclock_t:remove_from_object(obj2)
--
-- Now the same tooltip is only attached to `myclock`.
--
-- @author Sébastien Gross &lt;seb•ɱɩɲʋʃ•awesome•ɑƬ•chezwam•ɖɵʈ•org&gt;
-- @copyright 2009 Sébastien Gross
-- @release @AWESOME_VERSION@
-- @module awful.tooltip
-------------------------------------------------------------------------

local mouse = mouse
local screen = screen
local timer = require("gears.timer")
local wibox = require("wibox")
local a_placement = require("awful.placement")
local beautiful = require("beautiful")
local textbox = require("wibox.widget.textbox")
local background = require("wibox.widget.background")
local setmetatable = setmetatable
local ipairs = ipairs

--- Tooltip object definition.
-- @table awful.tooltip
-- @field wibox The wibox displaying the tooltip.
-- @field visible True if tooltip is visible.
local tooltip = { mt = {}  }

--- Tooltip private data.
-- @local
-- @table awful.tooltip.data
-- @field fg tooltip foreground color.
-- @field font Tooltip font.
-- @field hide The hide() function.
-- @field show The show() function.
-- @field timer The text update timer.
-- @field timer_function The text update timer function.
local data = setmetatable({}, { __mode = 'k' })

-- Place the tooltip on the screen.
-- @param self A tooltip object.
local function place(self)
    a_placement.under_mouse(self.wibox)
    a_placement.no_offscreen(self.wibox)
end

-- Place the tooltip under the mouse.
--
-- @param self A tooltip object.
local function set_geometry(self)
    local my_geo = self.wibox:geometry()
    -- calculate width / height
    local n_w, n_h = self.textbox:fit(-1, -1)
    if my_geo.width ~= n_w or my_geo.height ~= n_h then
        self.wibox:geometry({ width = n_w, height = n_h })
    end
end

-- Show a tooltip.
--
-- @param self The tooltip to show.
local function show(self)
    -- do nothing if the tooltip is already shown
    if self.visible then return end
    if data[self].timer then
        if not data[self].timer.started then
            data[self].timer_function()
            data[self].timer:start()
        end
    end
    set_geometry(self)
    place(self)
    self.wibox.visible = true
    self.visible = true
end

-- Hide a tooltip.
--
-- @param self The tooltip to hide.
local function hide(self)
    -- do nothing if the tooltip is already hidden
    if not self.visible then return end
    if data[self].timer then
        if data[self].timer.started then
            data[self].timer:stop()
        end
    end
    self.visible = false
    self.wibox.visible = false
end

--- Change displayed text.
--
-- @param self The tooltip object.
-- @param text New tooltip text.
tooltip.set_text = function(self, text)
    self.textbox:set_text(text)
    set_geometry(self)
end

--- Change displayed text.
--
-- @param self The tooltip object.
-- @param text New tooltip text, including pango markup.
tooltip.set_markup = function(self, text)
    self.textbox:set_markup(text)
    set_geometry(self)
end

--- Change the tooltip's update interval.
--
-- @param self A tooltip object.
-- @param timeout The timeout value.
-- @function set_timeout
tooltip.set_timeout = function(self, timeout)
    if data[self].timer then
        data[self].timer.timeout = timeout
    end
end

--- Add tooltip to an object.
--
-- @param self The tooltip.
-- @param object An object.
tooltip.add_to_object = function(self, object)
    object:connect_signal("mouse::enter", data[self].show)
    object:connect_signal("mouse::leave", data[self].hide)
end

--- Remove tooltip from an object.
--
-- @param self The tooltip.
-- @param object An object.
-- @function remove_from_object
tooltip.remove_from_object = function(self, object)
    object:disconnect_signal("mouse::enter", data[self].show)
    object:disconnect_signal("mouse::leave", data[self].hide)
end


--- Create a new tooltip and link it to a widget.
-- @tparam table    args Arguments for tooltip creation.
-- @tparam number   args.timeout The timeout value for update_func.
-- @tparam function args.timer_function A function to dynamically change the
--                                      tooltip text.
-- @tparam table    args.objects A list of objects linked to the tooltip.
-- @tparam number   args.delay_show Delay showing the tooltip by this many
--                                  seconds.
-- @return The created tooltip.
-- @see add_to_object
-- @see set_timeout
-- @see set_text
-- @see set_markup
tooltip.new = function(args)
    local self = {
        wibox =  wibox({ }),
        visible = false,
    }

    -- private data
    local delay_timeout
    if args.delay_show then
        delay_timeout = timer { timeout = args.delay_show }
        delay_timeout:connect_signal("timeout", function () show(self) end)
    end
    data[self] = {
        show = function()
            if delay_timeout then delay_timeout:start()
            else show(self) end
        end,
        hide = function()
            if delay_timeout then delay_timeout:stop() end
            hide(self)
        end,
    }

    -- export functions
    self.set_text = tooltip.set_text
    self.set_markup = tooltip.set_markup
    self.set_timeout = tooltip.set_timeout
    self.add_to_object = tooltip.add_to_object
    self.remove_from_object = tooltip.remove_from_object

    -- setup the timer action only if needed
    if args.timer_function then
        data[self].timer = timer { timeout = args.timeout and args.timeout or 1 }
        data[self].timer_function = function()
                self:set_markup(args.timer_function())
            end
        data[self].timer:connect_signal("timeout", data[self].timer_function)
    end

    -- Set default properties
    self.wibox.border_width = beautiful.tooltip_border_width or beautiful.border_width or 1
    self.wibox.border_color = beautiful.tooltip_border_color or beautiful.border_normal or "#ffcb60"
    self.wibox.opacity = beautiful.tooltip_opacity or 1
    self.wibox:set_bg(beautiful.tooltip_bg_color or beautiful.bg_focus or "#ffcb60")
    local fg = beautiful.tooltip_fg_color or beautiful.fg_focus or "#000000"
    local font = beautiful.tooltip_font or beautiful.font or "terminus 6"

    -- set tooltip properties
    self.wibox.visible = false
    -- Who wants a non ontop tooltip ?
    self.wibox.ontop = true
    self.textbox = textbox()
    self.textbox:set_font(font)
    self.background = background(self.textbox)
    self.background:set_fg(fg)
    self.wibox:set_widget(self.background)

    -- add some signals on both the tooltip and widget
    self.wibox:connect_signal("mouse::enter", data[self].hide)

    -- Add tooltip to objects
    if args.objects then
        for _, object in ipairs(args.objects) do
            self:add_to_object(object)
        end
    end

    return self
end

function tooltip.mt:__call(...)
    return tooltip.new(...)
end

return setmetatable(tooltip, tooltip.mt)

-- vim: ft=lua:et:sw=4:ts=4:sts=4:tw=78
