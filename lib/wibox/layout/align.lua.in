---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local table = table
local pairs = pairs
local type = type
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")

-- wibox.layout.align
local align = {}

--- Draw an align layout.
-- @param wibox The wibox that this widget is drawn to.
-- @param cr The cairo context to use.
-- @param width The available width.
-- @param height The available height.
-- @return The total space needed by the layout.
function align:draw(wibox, cr, width, height)
    local size_first = 0
    local size_third = 0
    local size_limit = self.dir == "y" and height or width

    if self.first then
        local w, h, _ = width, height, nil
        if self.dir == "y" then
            _, h = base.fit_widget(self.first, w, h)
            size_first = h
        else
            w, _ = base.fit_widget(self.first, w, h)
            size_first = w
        end
        base.draw_widget(wibox, cr, self.first, 0, 0, w, h)
    end

    if self.third and size_first < size_limit then
        local w, h, x, y, _
        if self.dir == "y" then
            w, h = width, height - size_first
            _, h = base.fit_widget(self.third, w, h)
            x, y = 0, height - h
            size_third = h
        else
            w, h = width - size_first, height
            w, _ = base.fit_widget(self.third, w, h)
            x, y = width - w, 0
            size_third = w
        end
        base.draw_widget(wibox, cr, self.third, x, y, w, h)
    end

    if self.second and size_first + size_third < size_limit then
        local x, y, w, h
        if self.dir == "y" then
            w, h = width, size_limit - size_first - size_third
            local real_w, real_h = base.fit_widget(self.second, w, h)
            x, y = 0, size_first + h / 2 - real_h / 2
            h = real_h
        else
            w, h = size_limit - size_first - size_third, height
            local real_w, real_h = base.fit_widget(self.second, w, h)
            x, y = size_first + w / 2 - real_w / 2, 0
            w = real_w
        end
        base.draw_widget(wibox, cr, self.second, x, y, w, h)
    end
end

local function widget_changed(layout, old_w, new_w)
    if old_w then
        old_w:disconnect_signal("widget::updated", layout._emit_updated)
    end
    if new_w then
        widget_base.check_widget(new_w)
        new_w:connect_signal("widget::updated", layout._emit_updated)
    end
    layout._emit_updated()
end

--- Set the layout's first widget. This is the widget that is at the left/top
function align:set_first(widget)
    widget_changed(self, self.first, widget)
    self.first = widget
end

--- Set the layout's second widget. This is the centered one.
function align:set_second(widget)
    widget_changed(self, self.second, widget)
    self.second = widget
end

--- Set the layout's third widget. This is the widget that is at the right/bottom
function align:set_third(widget)
    widget_changed(self, self.third, widget)
    self.third = widget
end

--- Fit the align layout into the given space. The align layout will
-- take all available space in its direction and the maximum size of
-- it's all three inner widgets in the other axis.
-- @param orig_width The available width.
-- @param orig_height The available height.
function align:fit(orig_width, orig_height)
    local used_max = 0

    for k, v in pairs{self.first, self.second, self.third} do
        local w, h = base.fit_widget(v, orig_width, orig_height)

        local max = self.dir == "y" and w or h

        if max > used_max then
            used_max = max
        end
    end

    if self.dir == "y" then
        return used_max, orig_height
    end
    return orig_width, used_max
end

function align:reset()
    for k, v in pairs({ "first", "second", "third" }) do
        self[v] = nil
    end
    self:emit_signal("widget::updated")
end

local function get_layout(dir)
    local ret = widget_base.make_widget()
    ret.dir = dir
    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    for k, v in pairs(align) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    return ret
end

--- Returns a new horizontal align layout. An align layout can display up to
-- three widgets. The widget set via :set_left() is left-aligned. :set_right()
-- sets a widget which will be right-aligned. The remaining space between those
-- two will be given to the widget set via :set_middle().
function align.horizontal()
    local ret = get_layout("x")

    ret.set_left = ret.set_first
    ret.set_middle = ret.set_second
    ret.set_right = ret.set_third

    return ret
end

--- Returns a new vertical align layout. An align layout can display up to
-- three widgets. The widget set via :set_top() is top-aligned. :set_bottom()
-- sets a widget which will be bottom-aligned. The remaining space between those
-- two will be given to the widget set via :set_middle().
function align.vertical()
    local ret = get_layout("y")

    ret.set_top = ret.set_first
    ret.set_middle = ret.set_second
    ret.set_bottom = ret.set_third

    return ret
end

return align

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
