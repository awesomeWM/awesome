---------------------------------------------------------------------------
-- @author Grigory Mishchenko &lt;grishkokot@gmail.com&gt;
-- @copyright 2015 Grigory Mishchenko
-- @release @AWESOME_VERSION@
-- @classmod wibox.widget.slider
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

local function cap(val, min, max)
    return (val < min) and min or (val > max) and max or val
end

local function get_value_from_position(position, data, cache)
    local position = data.vertical and cache.position.max - position or position
    local pos_diff = cache.position.max - cache.position.min
    local percentage = (position - cache.position.min) / ((pos_diff == 0) and 1 or pos_diff)
    local value = data.step * util.round(percentage * (data.max - data.min) / data.step) + data.min
    return cap(value, data.min, data.max)
end

local function get_position_from_value(value, data, cache)
    local value_diff = data.max - data.min
    local percentage = (value - data.min) / ((value_diff == 0) and 1 or value_diff)
    local position = percentage * (cache.position.max - cache.position.min) + cache.position.min
    position = data.vertical and cache.position.max - position or position
    return cap(position, cache.position.min, cache.position.max)
end

local function get_default_pointer(radius, pointer_color)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, radius*2, radius*2)
    local cr = cairo.Context(img)
    cr:set_source(color(pointer_color))
    cr:arc(radius, radius, radius, 0, 2*pi)
    cr:fill()
    return img
end

function slider:_get_slider_cache(width, height)
    if self._slider_cache then return self._slider_cache end
    local orient = self.data.vertical and height or width
    local center = (self.data.vertical and width or height) / 2
    local pw, ph, pwa, pha, pwd, phd = self:get_pointers_size()
    local ps = self.data.vertical and ph / 2 or pw / 2
    local psa = self.data.vertical and pha / 2 or pwa / 2
    local psd = self.data.vertical and phd / 2 or pwd / 2
    ps = self.data.mode_respect_max_size and max(ps, psa, psd) or ps

    local min_pos, min_bar = 0, 0
    local max_pos, max_bar = orient, orient
    local pointer_min = ps
    local pointer_max = orient - ps
    local pointer_min_active = psa
    local pointer_max_active = orient - psa
    local pointer_min_disabled = psd
    local pointer_max_disabled = orient - psd

    -- fix blurry bar line
    if (self.data.vertical and width or height) % 2 ~= self.data.bar_line_width % 2 then
        center = center + 0.5
    end

    if self.data.mode == 'stop_position' then
        min_pos = ps
        max_pos = orient - ps
    elseif self.data.mode == 'over' then
        pointer_min, pointer_min_active, pointer_min_disabled = 0, 0, 0
        pointer_max, pointer_max_active, pointer_max_disabled = orient, orient, orient
    elseif self.data.mode == 'over_margin' then
        min_pos, min_bar = ps, ps
        max_pos, max_bar = orient - ps, orient - ps
    end

    self._slider_cache = {center=center,
                          position={min=min_pos, max=max_pos},
                          bar={min=min_bar, max=max_bar},
                          pointer={min=pointer_min, max=pointer_max},
                          pointer_active={min=pointer_min_active, max=pointer_max_active},
                          pointer_disabled={min=pointer_min_disabled, max=pointer_max_disabled}}
    return self._slider_cache
end

--- Draw a slider with the given cairo context in the given geometry.
function slider:draw(wibox, cr, width, height)
    local cache = self:_get_slider_cache(width, height)
    local pw, ph = self:get_pointers_size()
    local pointer_min = self.data.disabled and cache.pointer_disabled.min or self._is_dragging and cache.pointer_active.min or cache.pointer.min
    local pointer_max = self.data.disabled and cache.pointer_disabled.max or self._is_dragging and cache.pointer_active.max or cache.pointer.max
    local pointer

    if not self._position then
        self._position = get_position_from_value(self._value, self.data, cache)
    else
        self._position = cap(self._position, cache.position.min, cache.position.max)
        self._value = get_value_from_position(self._position, self.data, cache)
        if self.data.snap then
            self._position = get_position_from_value(self._value, self.data, cache)
        end
    end

    local pointer_pos = cap(self._position, pointer_min, pointer_max)

    cr:set_line_width(self.data.bar_line_width)
    if not self.data.vertical then
        pointer = {x=pointer_pos, y=cache.center}
        cr:move_to(cache.bar.min, cache.center)
        cr:line_to(self._position, cache.center)
        cr:set_source(color(self.data.disabled and self.data.bar_color_filled_disabled or self.data.bar_color_filled))
        cr:stroke()
        cr:move_to(self._position, cache.center)
        cr:line_to(cache.bar.max, cache.center)
        cr:set_source(color(self.data.disabled and self.data.bar_color_disabled or self.data.bar_color))
        cr:stroke()
    else
        pointer = {x=cache.center, y=pointer_pos}
        cr:move_to(cache.center, cache.bar.min)
        cr:line_to(cache.center, self._position)
        cr:set_source(color(self.data.disabled and self.data.bar_color_disabled or self.data.bar_color))
        cr:stroke()
        cr:move_to(cache.center, self._position)
        cr:line_to(cache.center, cache.bar.max)
        cr:set_source(color(self.data.disabled and self.data.bar_color_filled_disabled or self.data.bar_color_filled))
        cr:stroke()
    end

    if self.data.with_pointer then
        cr:set_source_surface(self._current_pointer, pointer.x - pw/2, pointer.y - ph/2)
        cr:paint()
    end

    if self._value ~= self._cached_value then
        if not self._silent then
            if self.data.move_fn then
                self.data.move_fn(ret._value)
            end
            self:emit_signal("slider::value_changed")
        end
        self._cached_value = self._value
        self._silent = false
    end
end

--- Fit a slider widget into the given space
function slider:fit(width, height)
    local max_size = max(self.data.bar_line_width, self:get_pointer_size())
    return max(width, max_size), max(height, max_size)
end

--- Get size of pointer, active pointer and disabled pointer.
-- @return width, height, width_active, height_active, width_disabled, height_disabled
function slider:get_pointers_size()
    local w, h = surface.get_size(self.data.pointer)
    local wa, ha = surface.get_size(self.data.pointer_active)
    local wd, wh = surface.get_size(self.data.pointer_disabled)
    return w, h, wa, ha, wd, wh
end

--- Set the slider to draw vertically. Default is false.
-- @param vertical A boolean value.
function slider:set_vertical(val)
    self.data.vertical = val
    self._position = nil
    self._slider_cache = nil
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("slider::data_changed")
end

--- Set the slider pointer. Pointer will be placed at the center of slider context.
-- @param val Can be cairo instance or file path.
-- @param keep_cache A boolean value. Don't clear cache after change pointer. Use if you know what you do.
-- @typeof Could be nil, active or disabled.
function slider:set_pointer(val, keep_cache, typeof)
    local typeof = typeof and '_'..typeof or ''
    if not val and (self.data['pointer_radius'..typeof] or self.data['pointer_color'..typeof]) then
        val = get_default_pointer(self.data['pointer_radius'..typeof], self.data['pointer_color'..typeof])
    end
    self.data['pointer'..typeof] = surface.load_uncached(val)
    if not keep_cache then
        self._slider_cache = nil
    end
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("slider::data_changed")
end

--- Set the slider active pointer. Pointer will be placed at the center of slider context.
-- It's a shortcut for slider:set_pointer(val, keep_cache, 'active').
-- @param val Can be cairo instance or file path.
-- @param keep_cache A boolean value. Don't clear cache after change pointer. Use if you know what you do.
function slider:set_pointer_active(val, keep_cache)
    self:set_pointer(val, keep_cache, 'active')
end

--- Set the slider disabled pointer. Pointer will be placed at the center of slider context.
-- It's a shortcut for slider:set_pointer(val, keep_cache, 'disabled').
-- @param val Can be cairo instance or file path.
-- @param keep_cache A boolean value. Don't clear cache after change pointer. Use if you know what you do.
function slider:set_pointer_disabled(val, keep_cache)
    self:set_pointer(val, keep_cache, 'disabled')
end

--- Set the radius of default pointer.
-- @param val Radius of the pointer.
-- @typeof Could be nil, active or disabled.
function slider:set_pointer_radius(val, typeof)
    local typeof = typeof and '_'..typeof or ''
    self.data['pointer_radius'..typeof] = val
    self:set_pointer()
end

--- Set the radius of default active pointer. It's a shortcut for
-- slider:set_pointer_radius(val, 'active').
-- @param val Radius of the pointer.
function slider:set_pointer_radius_active(val)
    self:set_pointer_radius(val, 'active')
end

--- Set the radius of default disabled pointer. It's a shortcut for
-- slider:set_pointer_radius(val, 'disabled').
-- @param val Radius of the pointer.
function slider:set_pointer_radius_disabled(val)
    self:set_pointer_radius(val, 'disabled')
end

--- Set color of the default pointer.
-- @param val Color of the pointer.
-- @typeof Could be nil, active or disabled.
function slider:set_pointer_color(val, typeof)
    local typeof = typeof and '_'..typeof or ''
    self.data['pointer_color'..typeof] = util.ensure_pango_color(val, 'green')
    self:set_pointer(nil, true)
end

--- Set color of the default active pointer. It's shortcut for slider:set_pointer_color(val, 'active')
-- @param val Color of the pointer.
function slider:set_pointer_color_active(val)
    self:set_pointer_color(val, 'active')
end

--- Set color of the default disabled pointer. It's shortcut for slider:set_pointer_color(val, 'disabled')
-- @param val Color of the pointer.
function slider:set_pointer_color_disabled(val)
    self:set_pointer_color(val, 'disabled')
end

--- Enable or disable slider.
-- @param val A boolean value.
function slider:set_disabled(val)
    self.data.disabled = val
    self._current_pointer = val and self.data.pointer_disabled or self.data.pointer
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("slider::data_changed")
end

--- Set slider current value.
-- @param val New value.
-- @param silent Do not emmit move_fn or value_changed signal.
function slider:set_value(val, silent)
    if val == self._value or self._is_dragging or self.data.disabled then return end
    self._value = val
    self._silent = silent
    self._position = nil
    self:emit_signal("widget::redraw_needed")
end

--- Set mode of drawing slider widget.
-- @param mode Could be 'stop', 'stop_position', 'over', 'over_margin'.
-- @param respect_max_size A boolean value. If true mode parameters will be
-- calculated by max size of all pointers, otherwise by default pointer.
function slider:set_mode(mode, respect_max_size)
    self.data.mode = mode
    self.data.mode_respect_max_size = respect_max_size
    self._position = nil
    self._slider_cache = nil
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("slider::data_changed")
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

--- Set the slider pointer color.
--
-- @function set_pointer_color
-- @param value The color.

--- Set the slider active pointer color.
--
-- @function set_pointer_color_active
-- @param value The color.

--- Set the slider disabled pointer color.
--
-- @function set_pointer_color_disabled
-- @param value The color.

--- Set the slider pointer radius.
--
-- @function set_pointer_radius
-- @param value The value.

--- Set the slider active pointer radius.
--
-- @function set_pointer_radius_active
-- @param value The value.

--- Set the slider disabled pointer radius.
--
-- @function set_pointer_radius_disabled
-- @param value The value.

--- Set the slider widget draw with pointer.
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

--- Set function which emited when dragging is start.
--
-- @function set_before_fn
-- @param value Function.

--- Set function which emited when value is changed.
--
-- @function set_move_fn
-- @param value Function.

--- Set function which emited when dragging is stop.
--
-- @function set_after_fn
-- @param value Function.

--- Set mode respect_max_size. It's a shortcut for slider:set_mode(current_mode, respect_max_size)
-- @param val A boolean value.
function slider:set_mode_respect_max_size(val)
    self:set_mode(self.data.mode, val)
end

--- Set bar line width.
-- @param val Integer value.
function slider:set_bar_line_width(val)
    self.data.bar_line_width = val
    self._slider_cache = nil
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("slider::data_changed")
end

--- Get current value
-- @return Integer value
function slider:get_value()
    return self._value
end

--- Create a slider widget. It's provide signals: 'slider::data_changed', 'slider::value_changed',
-- 'slider::dragging_started','slider::dragging_stoped'.
-- @tparam[opt] table args Parameters for slider widget.
-- @param args.min Minimum value.
-- @param args.max Maximum value.
-- @param args.initial Initial slider value.
-- @param args.step Change value by step.
-- @param args.snap Makes the slider snap to each step when dragging.
-- @param args.mode Set how to draw slider. Could be 'stop', 'stop_position', 'over', 'over_margin'
-- @param args.mode_respect_max_size If true mode parameters will be calculated by max size of all pointers, otherwise by default pointer.
-- @param args.vertical Set the slider to draw vertically.
-- @param args.bar_line_width Set the width of slider bar.
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
-- @param args.move_fn Function which emited when value is changed.
-- @param args.after_fn Function which emited when dragging is stop.
local function new(args)
    local ret = widget.make_widget()
    local args = args or {}
    local theme = beautiful.get()

    ret.data = {min=args.min or 0,
                max=args.max or 100,
                step=args.step or 1,
                snap=args.snap or false,
                mode=args.mode or 'stop',
                mode_respect_max_size=args.mode_respect_max_size,
                vertical=args.vertical or false,
                bar_line_width=args.bar_line_width or theme.slider_bar_line_width or 2,
                bar_color=util.ensure_pango_color(args.bar_color or theme.slider_bar_color or theme.fg_normal, "white"),
                bar_color_filled=util.ensure_pango_color(args.bar_color_filled or theme.slider_bar_color_filled or theme.fg_normal, "white"),
                bar_color_disabled=util.ensure_pango_color(args.bar_color_disabled or theme.slider_bar_color_disabled or theme.fg_normal, "white"),
                bar_color_filled_disabled=util.ensure_pango_color(args.bar_color_filled_disabled or theme.slider_bar_color_filled_disabled or theme.fg_normal, "white"),
                pointer_color=util.ensure_pango_color(args.pointer_color or theme.slider_pointer_color or theme.fg_normal, "white"),
                pointer_color_disabled=util.ensure_pango_color(args.pointer_color_disabled or theme.slider_pointer_color_disabled or theme.fg_normal, "white"),
                pointer_color_active=util.ensure_pango_color(args.pointer_color_active or theme.slider_pointer_color_active or theme.fg_normal, "white"),
                pointer_radius=args.pointer_radius or theme.slider_pointer_radius or 5,
                pointer_radius_active=args.pointer_radius_active or theme.pointer_radius_active or 5,
                pointer_radius_disabled=args.pointer_radius_disabled or theme.pointer_radius_disabled or 5,
                with_pointer=args.with_pointer == nil and true or args.with_pointer,
                draggable=args.draggable == nil and true or args.draggable,
                draggable_button=args.draggable_button or 1,
                disabled=args.disabled or false,
                cursor=args.cursor or 'fleur',
                before_fn=type(args.before_fn) == 'function' and args.before or nil,
                move_fn=type(args.move_fn) == 'function' and move or nil,
                after_fn=type(args.after_fn) == 'function' and args.after or nil}

    ret:add_signal('slider::data_changed')
    ret:add_signal('slider::value_changed')
    ret:add_signal('slider::dragging_started')
    ret:add_signal('slider::dragging_stoped')

    for k, v in pairs(slider) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    for k, v in pairs(ret.data) do
        if not ret['set_' .. k] then
            ret['set_' .. k] = function(self, val)
                self.data[k] = val
                self:emit_signal("widget::redraw_needed")
                self:emit_signal("slider::data_changed")
            end
        end
    end

    ret.layout = function() ret._slider_cache = nil end

    ret:set_pointer(args.pointer)
    ret:set_pointer_active(args.pointer_active)
    ret:set_pointer_disabled(args.pointer_disabled)

    ret._current_pointer = ret.data.disabled and ret.data.pointer_disabled or ret.data.pointer
    ret._value = cap(args.initial or ret.data.min, ret.data.min, ret.data.max)
    ret._cached_value = ret._value

    ret:connect_signal("button::press", function (v, x, y, button)
        if button ~= ret.data.draggable_button or ret.data.disabled then return end
        if ret.data.before_fn then ret.data.before_fn(ret._value) end

        ret._position = ret.data.vertical and y or x
        ret:emit_signal("widget::redraw_needed")

        if ret.data.draggable then
            ret._is_dragging = true
            local mouse_coords = capi.mouse.coords()
            local minx = mouse_coords.x - x
            local miny = mouse_coords.y - y
            ret._current_pointer = ret.data.pointer_active
            ret:emit_signal("slider::dragging_started")
            capi.mousegrabber.run(function (_mouse)
                if not _mouse.buttons[ret.data.draggable_button] then
                    if ret.data.after_fn then ret.after_fn(ret._value) end
                    ret._current_pointer = ret.data.pointer
                    ret._is_dragging = false
                    ret:emit_signal("widget::redraw_needed")
                    ret:emit_signal("slider::dragging_stoped")
                    return false
                end
                local new_position = ret.data.vertical and _mouse.y - miny or _mouse.x - minx
                if new_position ~= ret._position then
                    ret._position = new_position
                    ret:emit_signal("widget::redraw_needed")
                end
                return true
            end, ret.data.cursor or "fleur")
        end
    end)

    return ret
end

function slider.mt:__call(...)
    return new(...)
end

return setmetatable(slider, slider.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
