---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.margin
---------------------------------------------------------------------------

local pairs = pairs
local type = type
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local gcolor = require("gears.color")
local cairo = require("lgi").cairo

local margin = { mt = {} }

--- Draw a margin layout
function margin:draw(_, cr, width, height)
    local x = self.left
    local y = self.top
    local w = self.right
    local h = self.bottom
    local color = self.color

    if not self.widget or width <= x + w or height <= y + h then
        return
    end

    if color then
        cr:set_source(color)
        cr:rectangle(0, 0, width, height)
        cr:rectangle(x, y, width - x - w, height - y - h)
        cr:set_fill_rule(cairo.FillRule.EVEN_ODD)
        cr:fill()
    end
end

--- Layout a margin layout
function margin:layout(_, width, height)
    if self.widget then
        local x = self.left
        local y = self.top
        local w = self.right
        local h = self.bottom

        return { base.place_widget_at(self.widget, x, y, width - x - w, height - y - h) }
    end
end

--- Fit a margin layout into the given space
function margin:fit(context, width, height)
    local extra_w = self.left + self.right
    local extra_h = self.top + self.bottom
    local w, h = 0, 0
    if self.widget then
        w, h = base.fit_widget(self, context, self.widget, width - extra_w, height - extra_h)
    end

    if self._draw_empty == false and (w == 0 or h == 0) then
        return 0, 0
    end

    return w + extra_w, h + extra_h
end

--- Set the widget that this layout adds a margin on.
function margin:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self.widget = widget
    self:emit_signal("widget::layout_changed")
end

--- Get the number of children element
-- @treturn table The children
function margin:get_children()
    return {self.widget}
end

--- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function margin:set_children(children)
    self.widget = children and children[1]
    self:emit_signal("widget::layout_changed")
end

--- Set all the margins to val.
function margin:set_margins(val)
    self.left = val
    self.right = val
    self.top = val
    self.bottom = val
    self:emit_signal("widget::layout_changed")
end

--- Set the margins color to color
function margin:set_color(color)
    self.color = color and gcolor(color)
    self:emit_signal("widget::redraw_needed")
end

--- Draw the margin even if the content size is 0x0 (default: true)
-- @tparam boolean draw_empty Draw nothing is content is 0x0 or draw the margin anyway
function margin:set_draw_empty(draw_empty)
    self._draw_empty = draw_empty
    self:emit_signal("widget::layout_changed")
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
for _, v in pairs({ "left", "right", "top", "bottom" }) do
    margin["set_" .. v] = function(layout, val)
        layout[v] = val
        layout:emit_signal("widget::layout_changed")
    end
end

--- Returns a new margin layout.
-- @param[opt] widget A widget to use.
-- @param[opt] left A margin to use on the left side of the widget.
-- @param[opt] right A margin to use on the right side of the widget.
-- @param[opt] top A margin to use on the top side of the widget.
-- @param[opt] bottom A margin to use on the bottom side of the widget.
-- @param[opt] color A color for the margins.
-- @param[opt] draw_empty whether or not to draw the margin when the content is empty
local function new(widget, left, right, top, bottom, color, draw_empty)
    local ret = base.make_widget()

    for k, v in pairs(margin) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret:set_left(left or 0)
    ret:set_right(right or 0)
    ret:set_top(top or 0)
    ret:set_bottom(bottom or 0)
    ret:set_draw_empty(draw_empty)

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
