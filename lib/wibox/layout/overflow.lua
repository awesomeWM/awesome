local base = require('wibox.widget.base')
local fixed = require('wibox.layout.fixed')
local separator = require('wibox.widget.separator')
local gtable = require('gears.table')
local gshape = require('gears.shape')
local gobject = require('gears.object')
local mousegrabber = mousegrabber

local overflow = { mt = {} }

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
    for _, widget in pairs(widgets) do
        local w, h = base.fit_widget(self, context, widget, width, height)

        if is_y then
            used_max = math.max(used_max, w)
            used_in_dir = used_in_dir + h
        else
            used_in_dir = used_in_dir + w
            used_max = math.max(used_max, h)
        end
    end

    local spacing = self._private.spacing * (num_widgets - 1)
    used_in_dir = used_in_dir + spacing

    local need_scrollbar = used_in_dir > avail_in_dir and scrollbar_enabled

    -- If the direction perpendicular to scrolling (e.g. width in vertical
    -- scrolling) is not fully covered by any of the widgets, we can add our
    -- scrollbar width to that value. Otherwise widget size will be reduced
    -- during `layout` to make space for the scrollbar.
    if need_scrollbar
        and (
            (is_y and used_max < orig_width)
            or (not is_y and used_max < orig_height)
        ) then
        used_max = used_max + scrollbar_width
    end

    if is_y then
        return used_max, used_in_dir
    else
        return used_in_dir, used_max
    end
end

-- Adapted version of `wibox.layout.fixed.layout`
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
            used_in_dir = used_in_dir + w
            used_max = math.max(used_max, h)
        end
    end

    used_in_dir = used_in_dir + self._private.spacing * (#widgets-1)

    -- Save size for scrolling behavior
    self._private.avail_in_dir = avail_in_dir
    self._private.used_in_dir = used_in_dir

    local need_scrollbar = used_in_dir > avail_in_dir and scrollbar_enabled

    local scroll_position = self._private.position

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
        local bar_pos = (avail_in_dir - bar_length) * self._private.position

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

    for i, w in pairs(widgets) do
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

function overflow:set_step(step)
    self._private.step = step
    -- We don't need to emit enything here, since changing step only really
    -- takes effect the next time the user scrolls
end

function overflow:scroll(amount)
    if amount == 0 then
        return
    end
    local interval = self._private.used_in_dir
    local delta = self._private.step / interval

    local pos = self._private.position + (delta * amount)
    self:set_position(pos)
end

function overflow:set_position(pos)
    local current = self._private.position
    local interval = self._private.used_in_dir - self._private.avail_in_dir
    if current == pos
        -- the content takes less space than what is available, i.e. everything
        -- is already visible
        or interval <= 0
        -- the position is out of range
        or (current <= 0 and pos < 0)
        or (current >= 1 and pos > 1) then
        return
    end

    self._private.position = math.min(1, math.max(pos, 0))

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::position", pos)
end

function overflow:get_position()
    return self._private.position
end

function overflow:set_scrollbar_width(width)
    if self._private.scrollbar_width == width then
        return
    end

    self._private.scrollbar_width = width

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_width", width)
end

function overflow:set_scrollbar_position(position)
    if self._private.scrollbar_position == position then
        return
    end

    self._private.scrollbar_position = position

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_position", position)
end

function overflow:set_scrollbar_enabled(enabled)
    if self._private.scrollbar_enabled == enabled then
        return
    end

    self._private.scrollbar_enabled = enabled

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_enabled", enabled)
end

local function build_grabber(container)
    local is_y = container._private.dir == "y"
    local bar_interval = container._private.avail_in_dir - container._private.bar_length
    local start_pos = container._private.position * bar_interval
    local coords = mouse.coords()
    local start = is_y and coords.y or coords.x

    return function(mouse)
        if not mouse.buttons[1] then
            return false
        end

        local pos = is_y and mouse.y or mouse.x
        container:set_position((start_pos + (pos - start)) / bar_interval)

        return true
    end
end

local function apply_scrollbar_mouse_signal(container, w)
    w:connect_signal('button::press', function(_, _, _, button_id)
        if button_id ~= 1 then
            return
        end
        mousegrabber.run(build_grabber(container), "fleur")
    end)
end

function overflow:set_scrollbar_widget(widget)
    local w = base.make_widget_from_value(widget)

    apply_scrollbar_mouse_signal(self, w)

    self._private.scrollbar_widget = w

    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::scrollbar_widget", widget)
end

local function new(dir, ...)
    local ret = fixed[dir](...)

    gtable.crush(ret, overflow, true)
    ret.widget_name = gobject.modulename(2)

    -- Manually set the position here. We don't know the bounding size yet.
    ret._private.position = 0

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

function overflow.horizontal(...)
    return new("horizontal", ...)
end

function overflow.vertical(...)
    return new("vertical", ...)
end

function overflow.mt:__call(...)
        return new(...)
end

return setmetatable(overflow, overflow.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
