---------------------------------------------------------------------------
-- A layout that allows its children to take more space than what's available
-- in the surrounding container. If the content does exceed the available
-- size, a scrollbar is added and scrolling behavior enabled.
--
--@DOC_wibox_layout_defaults_overflow_EXAMPLE@
-- @author Lucas Schwiderski
-- @copyright 2021 Lucas Schwiderski
-- @layoutmod wibox.layout.overflow
-- @supermodule wibox.layout.fixed
---------------------------------------------------------------------------

local base = require('wibox.widget.base')
local fixed = require('wibox.layout.fixed')
local separator = require('wibox.widget.separator')
local gtable = require('gears.table')
local gshape = require('gears.shape')
local gobject = require('gears.object')
local mousegrabber = mousegrabber

local overflow = { mt = {} }

-- Determine the required space to draw the layout's children and, if necessary,
-- the scrollbar.
function overflow:fit(context, orig_width, orig_height)
    local widgets = self._private.widgets
    local num_widgets = #widgets
    if num_widgets < 1 then
        return 0, 0
    end

    local width, height = orig_width, orig_height
    local scrollbar_width = self._private.scrollbar_width
    local scrollbar_enabled = self._private.scrollbar_enabled
    local used_in_dir, used_max = 0, 0
    local is_y = self._private.dir == "y"
    local avail_in_dir = is_y and orig_height or orig_width

    -- Set the direction covered by scrolling to the maximum value
    -- to allow widgets to take as much space as they want.
    if is_y then
        height = math.huge
    else
        width = math.huge
    end

    -- First, determine widget sizes.
    -- Only when the content doesn't fit and needs scrolling should
    -- we reduce content size to make space for a scrollbar.
    for _, widget in ipairs(widgets) do
        local w, h = base.fit_widget(self, context, widget, width, height)

        if is_y then
            used_max = math.max(used_max, w)
            used_in_dir = used_in_dir + h
        else
            used_max = math.max(used_max, h)
            used_in_dir = used_in_dir + w
        end
    end

    local spacing = self._private.spacing * (num_widgets - 1)
    used_in_dir = used_in_dir + spacing

    local need_scrollbar = scrollbar_enabled and used_in_dir > avail_in_dir

    -- Even if `used_max == orig_(width|height)` already, `base.fit_widget`
    -- will clamp return values, so we can "overextend" here.
    if need_scrollbar then
        used_max = used_max + scrollbar_width
    end

    if is_y then
        return used_max, used_in_dir
    else
        return used_in_dir, used_max
    end
end

-- Layout children, scrollbar and spacing widgets.
-- Only those widgets that are currently visible will be placed.
function overflow:layout(context, orig_width, orig_height)
    local result = {}
    local is_y = self._private.dir == "y"
    local widgets = self._private.widgets
    local avail_in_dir = is_y and orig_height or orig_width
    local scrollbar_width = self._private.scrollbar_width
    local scrollbar_enabled = self._private.scrollbar_enabled
    local scrollbar_position = self._private.scrollbar_position
    local width, height = orig_width, orig_height
    local widget_x, widget_y = 0, 0
    local used_in_dir, used_max = 0, 0

    -- Set the direction covered by scrolling to the maximum value
    -- to allow widgets to take as much space as they want.
    if is_y then
        height = math.huge
    else
        width = math.huge
    end

    -- First, determine widget sizes.
    -- Only when the content doesn't fit and needs scrolling should
    -- we reduce content size to make space for a scrollbar.
    for _, widget in pairs(widgets) do
        local w, h = base.fit_widget(self, context, widget, width, height)

        if is_y then
            used_max = math.max(used_max, w)
            used_in_dir = used_in_dir + h
        else
            used_max = math.max(used_max, h)
            used_in_dir = used_in_dir + w
        end
    end

    used_in_dir = used_in_dir + self._private.spacing * (#widgets-1)

    -- Save size for scrolling behavior
    self._private.avail_in_dir = avail_in_dir
    self._private.used_in_dir = used_in_dir

    local need_scrollbar = used_in_dir > avail_in_dir and scrollbar_enabled

    local scroll_position = self._private.scroll_factor

    if need_scrollbar then
        local scrollbar_widget = self._private.scrollbar_widget
        local bar_x, bar_y = 0, 0
        local bar_w, bar_h
        -- The percentage of how much of the content can be visible within
        -- the available space
        local visible_percent = avail_in_dir / used_in_dir
        -- Make scrollbar length reflect `visible_percent`
        -- TODO: Apply a default minimum length
        local bar_length = math.floor(visible_percent * avail_in_dir)
        local bar_pos = (avail_in_dir - bar_length) * self._private.scroll_factor

        if is_y then
            bar_w, bar_h = base.fit_widget(self, context, scrollbar_widget, scrollbar_width, bar_length)
            bar_y = bar_pos

            if scrollbar_position == "left" then
                widget_x = widget_x + bar_w
            elseif scrollbar_position == "right" then
                bar_x = orig_width - bar_w
            end

            self._private.bar_length = bar_h

            width = width - bar_w
        else
            bar_w, bar_h = base.fit_widget(self, context, scrollbar_widget, bar_length, scrollbar_width)
            bar_x = bar_pos

            if scrollbar_position == "top" then
                widget_y = widget_y + bar_h
            elseif scrollbar_position == "bottom" then
                bar_y = orig_height - bar_h
            end

            self._private.bar_length = bar_w

            height = height - bar_h
        end

        table.insert(result, base.place_widget_at(
            scrollbar_widget,
            math.floor(bar_x),
            math.floor(bar_y),
            math.floor(bar_w),
            math.floor(bar_h)
        ))
    end

    local pos, spacing = 0, self._private.spacing
    local interval = used_in_dir - avail_in_dir

    local spacing_widget = self._private.spacing_widget
    if spacing_widget then
        if is_y then
            local _
            _, spacing = base.fit_widget(self, context, spacing_widget, width, spacing)
        else
            spacing = base.fit_widget(self, context, spacing_widget, spacing, height)
        end
    end

    for i, w in ipairs(widgets) do
        local content_x, content_y
        local content_w, content_h = base.fit_widget(self, context, w, width, height)

        -- When scrolling down, the content itself moves up -> substract
        local scrolled_pos = pos - (scroll_position * interval)

        -- Stop processing completely once we're passed the visible portion
        if scrolled_pos > avail_in_dir then
            break
        end

        if is_y then
            content_x, content_y = widget_x, scrolled_pos
            pos = pos + content_h + spacing

            if self._private.fill_space then
                content_w = width
            end
        else
            content_x, content_y = scrolled_pos, widget_y
            pos = pos + content_w + spacing

            if self._private.fill_space then
                content_h = height
            end
        end

        local is_in_view = is_y
                           and (scrolled_pos + content_h > 0)
                           or (scrolled_pos + content_w > 0)

        if is_in_view then
            -- Add the spacing widget, but not before the first widget
            if i > 1 and spacing_widget then
                table.insert(result, base.place_widget_at(
                    spacing_widget,
                    -- The way how spacing is added for regular widgets
                    -- and the `spacing_widget` is disconnected:
                    -- The offset for regular widgets is added to `pos` one
                    -- iteration _before_ the one where the widget is actually
                    -- placed.
                    -- Because of that, the placement for the spacing widget
                    -- needs to substract that offset to be placed right after
                    -- the previous regular widget.
                    math.floor(is_y and content_x or (content_x - spacing)),
                    math.floor(is_y and (content_y - spacing) or content_y),
                    math.floor(is_y and content_w or spacing),
                    math.floor(is_y and spacing or content_h)
                ))
            end

            table.insert(result, base.place_widget_at(
                w,
                math.floor(content_x),
                math.floor(content_y),
                math.floor(content_w),
                math.floor(content_h)
            ))
        end
    end

    return result
end

function overflow:before_draw_children(_, cr, width, height)
    -- Clip drawing for children to the space we're allowed to draw in
    cr:rectangle(0, 0, width, height)
    cr:clip()
end


--- The amount of units to advance per scroll event.
--
-- This affects calls to `scroll` and the default mouse wheel handler.
--
-- The default is `10`.
--
-- @property step
-- @tparam number step The step size.
function overflow:set_step(step)
    self._private.step = step
    -- We don't need to emit enything here, since changing step only really
    -- takes effect the next time the user scrolls
end


--- Scroll the layout's content by `amount * step`.
--
-- A positive amount scrolls down/right, a negative amount scrolls up/left.
--
-- The amount of units scrolled is affected by `step`.
--
-- @method overflow:scroll
-- @tparam number amount The amount to scroll by.
-- @emits property::overflow::scroll_factor
-- @emitstparam property::overflow::scroll_factor number scroll_factor The new
--   scroll factor.
-- @emits widget::layout_changed
-- @emits widget::redraw_needed
function overflow:scroll(amount)
    if amount == 0 then
        return
    end
    local interval = self._private.used_in_dir
    local delta = self._private.step / interval

    local factor = self._private.scroll_factor + (delta * amount)
    self:set_scroll_factor(factor)
end


--- The scroll factor.
--
-- The scroll factor represents how far the layout's content is currently
-- scrolled. It is represented as a fraction from `0` to `1`, where `0` is the
-- start of the content and `1` is the end.
--
-- @property scroll_factor
-- @tparam number scroll_factor The scroll factor.
-- @propemits true false

function overflow:set_scroll_factor(factor)
    local current = self._private.scroll_factor
    local interval = self._private.used_in_dir - self._private.avail_in_dir
    if current == factor
        -- the content takes less space than what is available, i.e. everything
        -- is already visible
        or interval <= 0
        -- the scroll factor is out of range
        or (current <= 0 and factor < 0)
        or (current >= 1 and factor > 1) then
        return
    end

    self._private.scroll_factor = math.min(1, math.max(factor, 0))

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scroll_factor", factor)
end

function overflow:get_scroll_factor()
    return self._private.scroll_factor
end


--- The scrollbar width.
--
-- For horizontal scrollbars, this is the scrollbar height
--
-- The default is `5`.
--
--@DOC_wibox_layout_overflow_scrollbar_width_EXAMPLE@
--
-- @property scrollbar_width
-- @tparam number scrollbar_width The scrollbar width.
-- @propemits true false

function overflow:set_scrollbar_width(width)
    if self._private.scrollbar_width == width then
        return
    end

    self._private.scrollbar_width = width

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_width", width)
end


--- The scrollbar position.
--
-- For horizontal scrollbars, this can be `"top"` or `"bottom"`,
-- for vertical scrollbars this can be `"left"` or `"right"`.
-- The default is `"right"`/`"bottom"`.
--
--@DOC_wibox_layout_overflow_scrollbar_position_EXAMPLE@
--
-- @property scrollbar_position
-- @tparam string scrollbar_position The scrollbar position.
-- @propemits true false

function overflow:set_scrollbar_position(position)
    if self._private.scrollbar_position == position then
        return
    end

    self._private.scrollbar_position = position

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_position", position)
end

function overflow:get_scrollbar_position()
    return self._private.scrollbar_position
end


--- The scrollbar visibility.
--
-- If this is set to `false`, no scrollbar will be rendered, even if the layout's
-- content overflows. Mouse wheel scrolling will work regardless.
--
-- The default is `true`.
--
-- @property scrollbar_enabled
-- @tparam boolean scrollbar_enabled The scrollbar visibility.
-- @propemits true false

function overflow:set_scrollbar_enabled(enabled)
    if self._private.scrollbar_enabled == enabled then
        return
    end

    self._private.scrollbar_enabled = enabled

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_enabled", enabled)
end

function overflow:get_scrollbar_enabled()
    return self._private.scrollbar_enabled
end

-- Wraps a callback function for `mousegrabber` that is capable of
-- updating the scroll factor.
local function build_grabber(container, initial_x, initial_y, geo)
    local is_y = container._private.dir == "y"
    local bar_interval = container._private.avail_in_dir - container._private.bar_length
    local start_pos = container._private.scroll_factor * bar_interval
    local start = is_y and initial_y or initial_x

    -- Calculate a matrix transforming from screen coordinates into widget
    -- coordinates.
    -- This is required for mouse movement to work when the widget has been
    -- transformed by something like `wibox.container.rotate`.
    local matrix_from_device = geo.hierarchy:get_matrix_from_device()
    local wgeo = geo.drawable.drawable:geometry()
    local matrix = matrix_from_device:translate(-wgeo.x, -wgeo.y)

    return function(mouse)
        if not mouse.buttons[1] then
            return false
        end

        local x, y = matrix:transform_point(mouse.x, mouse.y)
        local pos = is_y and y or x
        container:set_scroll_factor((start_pos + (pos - start)) / bar_interval)

        return true
    end
end

-- Applies a mouse button signal using `build_grabber` to a scrollbar widget.
local function apply_scrollbar_mouse_signal(container, w)
    w:connect_signal('button::press', function(_, x, y, button_id, _, geo)
        if button_id ~= 1 then
            return
        end
        mousegrabber.run(build_grabber(container, x, y, geo), "fleur")
    end)
end


--- The scrollbar widget.
-- This widget is rendered as the scrollbar element.
--
-- The default is `wibox.widget.separator{ shape = gears.shape.rectangle }`.
--
--@DOC_wibox_layout_overflow_scrollbar_widget_EXAMPLE@
--
-- @property scrollbar_widget
-- @tparam widget scrollbar_widget The scrollbar widget.
-- @propemits true false

function overflow:set_scrollbar_widget(widget)
    local w = base.make_widget_from_value(widget)

    apply_scrollbar_mouse_signal(self, w)

    self._private.scrollbar_widget = w

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_widget", widget)
end

function overflow:get_scrollbar_widget()
    return self._private.scrollbar_widget
end


function overflow:reset()
    self._private.widgets = {}
    self._private.scroll_factor = 0

    local scrollbar_widget = separator({ shape = gshape.rectangle })
    apply_scrollbar_mouse_signal(self, scrollbar_widget)
    self._private.scrollbar_widget = scrollbar_widget

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::reset")
    self:emit_signal("widget::reseted")
end

local function new(dir, ...)
    local ret = fixed[dir](...)

    gtable.crush(ret, overflow, true)
    ret.widget_name = gobject.modulename(2)
    -- Tell the widget system to prevent clicks outside the layout's extends
    -- to register with child widgets, even if they actually extend that far.
    -- This prevents triggering button presses on hidden/clipped widgets.
    ret.clip_child_extends = true

    -- Manually set the scroll factor here. We don't know the bounding size yet.
    ret._private.scroll_factor = 0

    -- Apply defaults. Bypass setters to avoid signals.
    ret._private.step = 10
    ret._private.fill_space = true
    ret._private.scrollbar_width = 5
    ret._private.scrollbar_enabled = true
    ret._private.scrollbar_position = dir == "vertical" and "right" or "bottom"

    local scrollbar_widget = separator({ shape = gshape.rectangle })
    apply_scrollbar_mouse_signal(ret, scrollbar_widget)
    ret._private.scrollbar_widget = scrollbar_widget

    ret:connect_signal('button::press', function(self, _, _, button)
        if button == 4 then
            self:scroll(-1)
        elseif button == 5 then
            self:scroll(1)
        end
    end)

    return ret
end


--- Returns a new horizontal overflow layout.
-- Child widgets are placed similar to `wibox.layout.fixed`, except that
-- they may take as much width as they want. If the total width of all child
-- widgets exceeds the width available whithin the layout's outer container
-- a scrollbar will be added and scrolling behavior enabled.
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.overflow.horizontal
function overflow.horizontal(...)
    return new("horizontal", ...)
end


--- Returns a new vertical overflow layout.
-- Child widgets are placed similar to `wibox.layout.fixed`, except that
-- they may take as much height as they want. If the total height of all child
-- widgets exceeds the height available whithin the layout's outer container
-- a scrollbar will be added and scrolling behavior enabled.
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.overflow.vertical
function overflow.vertical(...)
    return new("vertical", ...)
end

return setmetatable(overflow, overflow.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
