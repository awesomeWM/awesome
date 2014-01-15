---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local pairs = pairs
local type = type
local setmetatable = setmetatable
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")
local gcolor = require("gears.color")
local cairo = require("lgi").cairo

-- wibox.layout.margin
local margin = { mt = {} }

--- Draw a margin layout
function margin:draw(wibox, cr, width, height)
    local x = self.left
    local y = self.top
    local w = self.right
    local h = self.bottom
    local color = self.color

    if not self.widget or width <= x + w or height <= y + h then
        return
    end

    if color then
        cr:save()
        cr:set_source(color)
        cr:rectangle(0, 0, width, height)
        cr:rectangle(x, y, width - x - w, height - y - h)
        cr:set_fill_rule(cairo.FillRule.EVEN_ODD)
        cr:fill()
        cr:restore()
    end

    base.draw_widget(wibox, cr, self.widget, x, y, width - x - w, height - y - h)
end

--- Fit a margin layout into the given space
function margin:fit(width, height)
    local extra_w = self.left + self.right
    local extra_h = self.top + self.bottom
    local w, h = 0, 0
    if self.widget then
        w, h = base.fit_widget(self.widget, width - extra_w, height - extra_h)
    end
    return w + extra_w, h + extra_h
end

--- Set the widget that this layout adds a margin on.
function margin:set_widget(widget)
    if self.widget then
        self.widget:disconnect_signal("widget::updated", self._emit_updated)
    end
    if widget then
        widget_base.check_widget(widget)
        widget:connect_signal("widget::updated", self._emit_updated)
    end
    self.widget = widget
    self._emit_updated()
end

--- Set all the margins to val.
function margin:set_margins(val)
    self.left = val
    self.right = val
    self.top = val
    self.bottom = val
    self:emit_signal("widget::updated")
end

--- Set the margins color to color
function margin:set_color(color)
    self.color = color and gcolor(color)
    self._emit_updated()
end

--- Reset this layout. The widget will be unreferenced, the margins set to 0
-- and the color erased
function margin:reset()
    self:set_widget(nil)
    self:set_margins(0)
    self:set_color(nil)
end

--- Set the left margin that this layout adds to its widget.
-- @param layout The layout you are modifying.
-- @param margin The new margin to use.
-- @name set_left
-- @class function

--- Set the right margin that this layout adds to its widget.
-- @param layout The layout you are modifying.
-- @param margin The new margin to use.
-- @name set_right
-- @class function

--- Set the top margin that this layout adds to its widget.
-- @param layout The layout you are modifying.
-- @param margin The new margin to use.
-- @name set_top
-- @class function

--- Set the bottom margin that this layout adds to its widget.
-- @param layout The layout you are modifying.
-- @param margin The new margin to use.
-- @name set_bottom
-- @class function

-- Create setters for each direction
for k, v in pairs({ "left", "right", "top", "bottom" }) do
    margin["set_" .. v] = function(layout, val)
        layout[v] = val
        layout:emit_signal("widget::updated")
    end
end

--- Returns a new margin layout.
-- @param widget A widget to use (optional)
-- @param left A margin to use on the left side of the widget (optional)
-- @param right A margin to use on the right side of the widget (optional)
-- @param top A margin to use on the top side of the widget (optional)
-- @param bottom A margin to use on the bottom side of the widget (optional)
-- @param color A color for the margins (optional)
local function new(widget, left, right, top, bottom, color)
    local ret = widget_base.make_widget()

    for k, v in pairs(margin) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    ret:set_left(left or 0)
    ret:set_right(right or 0)
    ret:set_top(top or 0)
    ret:set_bottom(bottom or 0)

    ret:set_color(color)

    if widget then
        ret:set_widget(widget)
    end

    return ret
end

function margin.mt:__call(...)
    return new(...)
end

return setmetatable(margin, margin.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
