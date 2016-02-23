---------------------------------------------------------------------------
-- @author Grigory Mishchenko &lt;grishkokot@gmail.com&gt;
-- @copyright 2015 Grigory Mishchenko
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.slider
---------------------------------------------------------------------------

local setmetatable = setmetatable
local type = type
local max = math.max
local pi = math.pi
local widget = require("wibox.widget.base")
local color = require("gears.color")
local util = require("awful.util")
local surface = require("gears.surface")
local cairo = require('lgi').cairo
local beautiful = require("beautiful")
local capi = {
    mouse = mouse,
    mousegrabber = mousegrabber
}

--- Slider widget.
-- awful.widget.slider
local slider = { mt = {} }
local properties = { "min", "max", "step", "snap", "mode", "vertical",
                     "bar_width", "bar_color", "bar_color_filled",
                     "bar_color_disabled", "bar_color_filled_disabled",
                     "pointer_radius", "pointer_radius_active", "pointer_radius_disabled",
                     "pointer_color", "pointer_color_active", "pointer_color_disabled",
                     "with_pointer", "draggable", "draggable_button", "disabled",
                     "cursor", "before_fn", "move_fn", "after_fn", "change_fn" }

local function cap(value, min, max)
    return (value < min) and min or (value > max) and max or value
end

local function get_new_value(position, data, props)
    local position = data.vertical and props.position.max + props.position.min - position or position
    local slider_width = props.position.max - props.position.min
    local percentage = (position - props.position.min) / ((slider_width == 0) and 1 or slider_width)
    local value = data.step * util.round(percentage * (data.max - data.min) / data.step) + data.min
    return cap(value, data.min, data.max)
end

local function get_new_position(value, data, props, position)
    if data.snap or not position then
        local value_diff = data.max - data.min
        position = (value - data.min) / ((value_diff == 0) and 1 or value_diff)
        if data.vertical then
            position = 1 - position
        end
    else
        local slider_width = props.position.max - props.position.min
        position = (position - props.position.min) / ((slider_width == 0) and 1 or slider_width)
    end
    return cap(position, 0, 1)
end


function slider:_get_draw_properties(width, height)
    local orient = self.data.vertical and height or width
    local center = (self.data.vertical and width or height) / 2
    local pointer = self.data.disabled and self.data.pointer_disabled or self._is_dragging and self.data.pointer_active or self.data.pointer
    local pw, ph
    if (self.data.mode == 'over_margin') then
        pw, ph = self:get_pointers_size(true)
    else
        pw, ph = surface.get_size(pointer)
    end
    local ps = self.data.vertical and ph / 2 or pw / 2
    local position_min, bar_min = 0, 0
    local position_max, bar_max = orient, orient
    local pointer_min = ps
    local pointer_max = orient - ps
    local bar_center = center

    -- fix blurry bar line
    if (self.data.vertical and width or height) % 2 ~= self.data.bar_width % 2 then
        bar_center = center + 0.5
    end

    if self.data.mode == 'stop_position' then
        position_min = ps
        position_max = orient - ps
    elseif self.data.mode == 'over' then
        pointer_min = 0
        pointer_max = orient
    elseif self.data.mode == 'over_margin' then
        position_min, bar_min = ps, ps
        position_max, bar_max = orient - ps, orient - ps
    end

    return { position={min=position_min, max=position_max},
             bar={min=bar_min, max=bar_max, center=bar_center},
             pointer={min=pointer_min, max=pointer_max, center=center, current=pointer} }
end


--- Draw a slider with the given cairo context in the given geometry.
function slider:draw(wibox, cr, width, height)
    local props = self:_get_draw_properties(width, height)
    local position = self._position or get_new_position(self._value, self.data, props)
    position = position * (props.position.max - props.position.min) + props.position.min
    local pointer_position = cap(position, props.pointer.min, props.pointer.max)
    local pw, ph = surface.get_size(props.pointer.current)
    local px, py

    cr:set_line_width(self.data.bar_width)
    if not self.data.vertical then
        px, py = pointer_position, props.pointer.center
        cr:move_to(props.bar.min, props.bar.center)
        cr:line_to(position, props.bar.center)
        cr:set_source(color(self.data.disabled and self.data.bar_color_filled_disabled or self.data.bar_color_filled))
        cr:stroke()
        cr:move_to(position, props.bar.center)
        cr:line_to(props.bar.max, props.bar.center)
        cr:set_source(color(self.data.disabled and self.data.bar_color_disabled or self.data.bar_color))
        cr:stroke()
    else
        px, py = props.pointer.center, pointer_position
        cr:move_to(props.bar.center, props.bar.min)
        cr:line_to(props.bar.center, position)
        cr:set_source(color(self.data.disabled and self.data.bar_color_disabled or self.data.bar_color))
        cr:stroke()
        cr:move_to(props.bar.center, position)
        cr:line_to(props.bar.center, props.bar.max)
        cr:set_source(color(self.data.disabled and self.data.bar_color_filled_disabled or self.data.bar_color_filled))
        cr:stroke()
    end

    if self.data.with_pointer then
        cr:set_source_surface(props.pointer.current, px - pw/2, py - ph/2)
        cr:paint()
    end

    if self._value ~= self._cached_value then
        if self.data.change_fn then
            self.data.change_fn(self._value)
        end
        self._cached_value = self._value
        self:emit_signal("slider::value_changed")
    end
end

--- Fit a slider widget into the given space
function slider:fit(width, height)
    local max_size = max(self.data.bar_width, self:get_pointers_size())
    return max(width, max_size), max(height, max_size)
end

--- Get sizes of default, active and disabled pointers.
-- @param max_size A boolean value. If true return only max width and max heihgt of all pointers
-- @return width, height, width_active, height_active, width_disabled, height_disabled
function slider:get_pointers_size(max_size)
    local w, h = surface.get_size(self.data.pointer)
    local wa, ha = surface.get_size(self.data.pointer_active)
    local wd, hd = surface.get_size(self.data.pointer_disabled)
    if max_size then
        return max(w, wa, wd), max(h, ha, hd)
    end
    return w, h, wa, ha, wd, hd
end

--- Set the slider pointer. Pointer will be placed at the center of slider context.
-- @param value Can be cairo instance or file path.
-- @param typeof Could be nil, active or disabled. Use if you know what are you doing.
-- @param property Property name for emit property change signal. Use if you know what are you doing.
-- @param need_reposition Reset current position. Use if you know what are you doing.
function slider:set_pointer(value, typeof, property, need_reposition)
    local typeof = typeof and '_'..typeof or ''
    local property = property or 'pointer'
    local need_reposition = (need_reposition == nil) and true or need_reposition
    if not value and (self.data['pointer_color'..typeof] or self.data['pointer_radius'..typeof]) then
        local pointer_radius = self.data['pointer_radius'..typeof] or self.data.pointer_radius or 3
        value = cairo.ImageSurface(cairo.Format.ARGB32, pointer_radius*2, pointer_radius*2)
        local cr = cairo.Context(value)
        cr:set_source(color(self.data['pointer_color'..typeof] or self.data.pointer_color))
        cr:arc(pointer_radius, pointer_radius, pointer_radius, 0, 2*pi)
        cr:fill()
    end
    local new_value = surface.load(value)
    if self.data['pointer'..typeof] == new_value then return end
    self.data['pointer'..typeof] = new_value
    if need_reposition then
        self._position = nil
    end
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::"..property)
end

--- Set the slider active pointer. Pointer will be placed at the center of slider context.
-- It's a shortcut for slider:set_pointer(value, 'active', 'pointer_active').
-- @param value Can be cairo instance or file path.
function slider:set_pointer_active(value)
    self:set_pointer(value, 'active', 'pointer_active')
end

--- Set the slider disabled pointer. Pointer will be placed at the center of slider context.
-- It's a shortcut for slider:set_pointer(value, 'disabled', 'pointer_disabled').
-- @param value Can be cairo instance or file path.
function slider:set_pointer_disabled(value)
    self:set_pointer(value, 'disabled', 'pointer_disabled')
end

--- Set the radius of the default pointer.
-- @param value Radius of the pointer.
function slider:set_pointer_radius(value)
    if self.data.pointer_radius == value then return end
    self.data.pointer_radius = value or theme.slider_pointer_radius or 5
    self:set_pointer(nil, nil, 'pointer_radius')
end

--- Set the radius of the active pointer.
-- @param value Radius of the pointer.
function slider:set_pointer_radius_active(value)
    if self.data.pointer_radius_active == value then return end
    self.data.pointer_radius_active = value or theme.pointer_radius_active
    self:set_pointer(nil, 'active', 'pointer_radius_active')
end

--- Set the radius of the disabled pointer.
-- @param value Radius of the pointer.
function slider:set_pointer_radius_disabled(value)
    if self.data.pointer_radius_disabled == value then return end
    self.data.pointer_radius_disabled = value or theme.pointer_radius_disabled
    self:set_pointer(nil, 'disabled', 'pointer_radius_disabled')
end

--- Set color of the default pointer.
-- @param value Color of the pointer.
function slider:set_pointer_color(value)
    if self.data.pointer_color == value then return end
    self.data.pointer_color = value or theme.slider_pointer_color or theme.fg_normal
    self:set_pointer(nil, nil, 'pointer_color', false)
end

--- Set color of the active pointer.
-- @param value Color of the pointer.
function slider:set_pointer_color_active(value)
    if self.data.pointer_color_active == value then return end
    self.data.pointer_color_active = value or theme.slider_pointer_color_active
    self:set_pointer(nil, 'active', 'pointer_color_active', false)
end

--- Set color of the disabled pointer.
-- @param value Color of the pointer.
function slider:set_pointer_color_disabled(value)
    if self.data.pointer_color_disabled == value then return end
    self.data.pointer_color_disabled = value or theme.slider_pointer_color_disabled
    self:set_pointer(nil, 'disabled', 'pointer_color_disabled', false)
end

--- Set slider current value.
-- @param value New value.
-- @param silent Do not emmit change_fn and "slider::value_changed" signal.
function slider:set_value(value, silent)
    value = cap(value, self.data.min, self.data.max)
    if value == self._value or self._is_dragging or self.data.disabled then return end
    self._value = value
    if silent then
        self._cached_value = self._value
    end
    self._position = nil
    self:emit_signal("widget::redraw_needed")
end

--- Set the slider minimum value.
--
-- @function set_min
-- @param value The value.

--- Set the slider maximum value.
--
-- @function set_max
-- @param value The value.

--- Set the slider step value.
--
-- @function set_step
-- @param value The value.

--- Set the slider snap value.
--
-- @function set_snap
-- @param value A boolean value.

--- Set the slider to draw vertically. Default is false.
--
-- @function set_vertical
-- @param vertical A boolean value.

--- Set mode of drawing slider widget.
--
-- @function set_mode
-- @param mode Could be 'stop', 'stop_position', 'over', 'over_margin'.

--- Enable or disable slider.
--
-- @function set_disabled
-- @param value A boolean value.

--- Set the slider bar color.
--
-- @function set_bar_color
-- @param value The color.

--- Set the slider filled bar color.
--
-- @function set_bar_color_filled
-- @param value The color.

--- Set the slider disabled bar color.
--
-- @function set_bar_color_disabled
-- @param value The color.

--- Set the slider filled disabled bar color.
--
-- @function set_bar_color_filled_disabled
-- @param value The color.

--- Set the slider widget draw pointer.
--
-- @function set_with_pointer
-- @param value A boolean value.

--- Set the slider widget to drag pointer.
--
-- @function set_draggable
-- @param value A boolean value.

--- Set the mouse button which can drag pointer.
--
-- @function set_draggable_button
-- @param value The value.

--- Set mouse cursor which will be used when dragging.
--
-- @function set_cursor
-- @param value The value.

--- Set function which emited before moving pointer.
--
-- @function set_before_fn
-- @param value Function.

--- Set function which emited when pointer moved.
--
-- @function set_move_fn
-- @param value Function.

--- Set function which emited when value is changed.
--
-- @function set_change_fn
-- @param value Function.

--- Set function which emited after moving pointer.
--
-- @function set_after_fn
-- @param value Function.

--- Set bar line width.
--
-- @function set_bar_width
-- @param value Integer value.

--- Get current value
-- @return Current value
function slider:get_value()
    return self._value
end

---
-- @signal slider::value_changed

---
-- @signal slider::before_move

---
-- @signal slider::after_move

---
-- @signal slider::moved

---
-- @signal property::pointer

---
-- @signal property::pointer_active

---
-- @signal property::pointer_disabled

---
-- @signal property::min

---
-- @signal property::max

---
-- @signal property::step

---
-- @signal property::snap

---
-- @signal property::mode

---
-- @signal property::vertical

---
-- @signal property::bar_width

---
-- @signal property::bar_color

---
-- @signal property::bar_color_filled

---
-- @signal property::bar_color_disabled

---
-- @signal property::bar_color_filled_disabled

---
-- @signal property::pointer_radius

---
-- @signal property::pointer_radius_active

---
-- @signal property::pointer_radius_disabled

---
-- @signal property::pointer_color

---
-- @signal property::pointer_color_active

---
-- @signal property::pointer_color_disabled

---
-- @signal property::with_pointer

---
-- @signal property::draggable

---
-- @signal property::draggable_button

---
-- @signal property::disabled

---
-- @signal property::cursor

---
-- @signal property::before_fn

---
-- @signal property::move_fn

---
-- @signal property::after_fn

---
-- @signal property::change_fn

--- Create a slider widget.
-- @tparam[opt] table args Parameters for slider widget.
-- @param args.min Minimum value.
-- @param args.max Maximum value.
-- @param args.initial Initial slider value.
-- @param args.step Change value by step.
-- @param args.snap Makes the slider snap to each step when dragging.
-- @param args.mode Set how to draw slider. Could be 'stop', 'stop_position', 'over', 'over_margin'
-- @param args.vertical Set the slider to draw vertically.
-- @param args.bar_width Set the width of slider bar.
-- @param args.bar_color Set the color of not filled bar.
-- @param args.bar_color_filled Set the color of filled bar.
-- @param args.bar_color_disabled Set the color of not filled disabled bar.
-- @param args.bar_color_filled_disabled Set the color of filled disabled bar.
-- @param args.pointer Set the custom normal pointer. Could be cairo instance or file path.
-- @param args.pointer_active Set the custom active pointer. Could be cairo instance or file path.
-- @param args.pointer_disabled Set the custom disabled pointer. Could be cairo instance or file path.
-- @param args.pointer_color Set the normal default pointer color.
-- @param args.pointer_color_active Set the active default pointer color.
-- @param args.pointer_color_disabled Set the disabled default pointer color.
-- @param args.pointer_radius Set the normal default pointer radius.
-- @param args.pointer_radius_active Set the active default pointer radius.
-- @param args.pointer_radius_disabled Set the disabled default pointer radius.
-- @param args.with_pointer Set the slider to draw pointer.
-- @param args.draggable Set the slider to drag pointer.
-- @param args.draggable_button Set the mouse button which can drag pointer.
-- @param args.disabled Set the slider disabled.
-- @param args.cursor Set mouse cursor which will be used when dragging.
-- @param args.before_fn Function which emited when dragging is start.
-- @param args.move_fn Function which emited when pointer is moving.
-- @param args.after_fn Function which emited when dragging is stop.
-- @param args.change_fn Function which emited when value is changed.
function slider.new(args)
    local ret = widget.make_widget()
    local args = args or {}
    local theme = beautiful.get()

    ret.data = { min=args.min or 0,
                 max=args.max or 100,
                 step=(args.step == 0 or args.step == nil) and 1 or args.step,
                 snap=args.snap or false,
                 mode=args.mode or 'stop',
                 vertical=args.vertical,
                 bar_width=args.bar_width or theme.slider_bar_width or 2,
                 bar_color=args.bar_color or theme.slider_bar_color or theme.fg_normal,
                 bar_color_filled=args.bar_color_filled or theme.slider_bar_color_filled or theme.fg_normal,
                 bar_color_disabled=args.bar_color_disabled or theme.slider_bar_color_disabled or theme.fg_normal,
                 bar_color_filled_disabled=args.bar_color_filled_disabled or theme.slider_bar_color_filled_disabled or theme.fg_normal,
                 pointer_color=args.pointer_color or theme.slider_pointer_color or theme.fg_normal,
                 pointer_color_active=args.pointer_color_active or theme.slider_pointer_color_active,
                 pointer_color_disabled=args.pointer_color_disabled or theme.slider_pointer_color_disabled,
                 pointer_radius=args.pointer_radius or theme.slider_pointer_radius or 5,
                 pointer_radius_active=args.pointer_radius_active or theme.pointer_radius_active,
                 pointer_radius_disabled=args.pointer_radius_disabled or theme.pointer_radius_disabled,
                 with_pointer=(args.with_pointer == nil) and true or args.with_pointer,
                 draggable=(args.draggable == nil) and true or args.draggable,
                 draggable_button=args.draggable_button or 1,
                 disabled=args.disabled or false,
                 cursor=args.cursor or 'fleur',
                 before_fn=type(args.before_fn) == 'function' and args.before_fn,
                 move_fn=type(args.move_fn) == 'function' and args.move_fn,
                 change_fn=type(args.change_fn) == 'function' and args.change_fn,
                 after_fn=type(args.after_fn) == 'function' and args.after_fn }

    ret:add_signal('slider::value_changed')
    ret:add_signal('slider::before_move')
    ret:add_signal('slider::after_move')
    ret:add_signal('slider::moved')
    ret:add_signal("property::pointer")
    ret:add_signal("property::pointer_active")
    ret:add_signal("property::pointer_disabled")

    for k, v in pairs(slider) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    local need_reposition = {min=true, max=true, mode=true, vertical=true}

    for _, prop in ipairs(properties) do
        ret:add_signal("property::"..prop)
        if not ret['set_' .. prop] then
            ret['set_' .. prop] = function(self, value)
                if self.data[prop] == value then return end
                self.data[prop] = value
                if need_reposition[prop] then
                    self._position = nil
                end
                self:emit_signal("property::"..prop)
                self:emit_signal("widget::redraw_needed")
            end
        end
    end

    ret:set_pointer(args.pointer)
    if args.pointer_active or args.pointer_color_active or args.pointer_radius_active then
        ret:set_pointer_active(args.pointer_active)
    end
    if args.pointer_disabled or args.pointer_color_disabled or args.pointer_radius_disabled then
        ret:set_pointer_disabled(args.pointer_disabled)
    end

    ret._value = cap(args.initial or ret.data.min, ret.data.min, ret.data.max)
    ret._cached_value = ret._value

    ret:connect_signal("button::press", function (v, x, y, button, mods, obj)
        if button ~= ret.data.draggable_button or ret.data.disabled then return end
        if ret.data.before_fn then ret.data.before_fn(ret._value) end
        ret:emit_signal("slider::before_move")

        local width = obj.width
        local height = obj.height
        local props = ret:_get_draw_properties(width, height)
        local position = ret._position

        ret._value = get_new_value(ret.data.vertical and y or x, ret.data, props)
        ret._position = get_new_position(ret._value, ret.data, props, ret.data.vertical and y or x)

        if position ~= ret._position then
            if ret.data.move_fn then ret.data.move_fn(ret._value) end
            ret:emit_signal("slider::moved")
        end
        ret:emit_signal("widget::redraw_needed")

        if ret.data.draggable then
            ret._is_dragging = true
            local mouse_coords = capi.mouse.coords()
            local minx = mouse_coords.x - x
            local miny = mouse_coords.y - y
            capi.mousegrabber.run(function (_mouse)
                if not _mouse.buttons[ret.data.draggable_button] then
                    if ret.data.after_fn then ret.data.after_fn(ret._value) end
                    ret._is_dragging = false
                    ret:emit_signal("slider::after_move")
                    ret:emit_signal("widget::redraw_needed")
                    return false
                end
                position = ret._position
                local new_position = ret.data.vertical and _mouse.y - miny or _mouse.x - minx
                ret._value = get_new_value(new_position, ret.data, props)
                ret._position = get_new_position(ret._value, ret.data, props, new_position)
                if position ~= ret._position then
                    if ret.data.move_fn then ret.data.move_fn(ret._value) end
                    ret:emit_signal("slider::moved")
                end
                ret:emit_signal("widget::redraw_needed")
                return true
            end, ret.data.cursor or "fleur")
        end
    end)

    return ret
end

function slider.mt:__call(...)
    return slider.new(...)
end

return setmetatable(slider, slider.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
