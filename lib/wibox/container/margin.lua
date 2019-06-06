---------------------------------------------------------------------------
--
--@DOC_wibox_container_defaults_margin_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @containermod wibox.container.margin
---------------------------------------------------------------------------

local pairs = pairs
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local gcolor = require("gears.color")
local cairo = require("lgi").cairo
local gtable = require("gears.table")

local margin = { mt = {} }

-- Draw a margin layout
function margin:draw(_, cr, width, height)
    local x = self._private.left
    local y = self._private.top
    local w = self._private.right
    local h = self._private.bottom
    local color = self._private.color

    if not self._private.widget or width <= x + w or height <= y + h then
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

-- Layout a margin layout
function margin:layout(_, width, height)
    if self._private.widget then
        local x = self._private.left
        local y = self._private.top
        local w = self._private.right
        local h = self._private.bottom

        return { base.place_widget_at(self._private.widget, x, y, width - x - w, height - y - h) }
    end
end

-- Fit a margin layout into the given space
function margin:fit(context, width, height)
    local extra_w = self._private.left + self._private.right
    local extra_h = self._private.top + self._private.bottom
    local w, h = 0, 0
    if self._private.widget then
        w, h = base.fit_widget(self, context, self._private.widget, width - extra_w, height - extra_h)
    end

    if self._private.draw_empty == false and (w == 0 or h == 0) then
        return 0, 0
    end

    return w + extra_w, h + extra_h
end

--- The widget to be wrapped the the margins.
-- @property widget
-- @tparam widget widget The widget

function margin:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function margin:get_widget()
    return self._private.widget
end

-- Get the number of children element
-- @treturn table The children
function margin:get_children()
    return {self._private.widget}
end

-- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function margin:set_children(children)
    self:set_widget(children[1])
end

--- Set all the margins to val.
-- @property margins
-- @tparam number|table val The margin value. It can be a number or a table with
--  the *left*/*right*/*top*/*bottom* keys.

function margin:set_margins(val)

    if type(val) == "number" or not val then
        if self._private.left   == val and
           self._private.right  == val and
           self._private.top    == val and
           self._private.bottom == val then
            return
        end

        self._private.left   = val
        self._private.right  = val
        self._private.top    = val
        self._private.bottom = val
    elseif type(val) == "table" then
        self._private.left   = val.left   or self._private.left
        self._private.right  = val.right  or self._private.right
        self._private.top    = val.top    or self._private.top
        self._private.bottom = val.bottom or self._private.bottom
    end

    self:emit_signal("widget::layout_changed")
end

--- Set the margins color to create a border.
-- @property color
-- @param color A color used to fill the margin.

function margin:set_color(color)
    self._private.color = color and gcolor(color)
    self:emit_signal("widget::redraw_needed")
end

function margin:get_color()
    return self._private.color
end

--- Draw the margin even if the content size is 0x0 (default: true)
-- @function draw_empty
-- @tparam boolean draw_empty Draw nothing is content is 0x0 or draw the margin anyway

function margin:set_draw_empty(draw_empty)
    self._private.draw_empty = draw_empty
    self:emit_signal("widget::layout_changed")
end

function margin:get_draw_empty()
    return self._private.draw_empty
end

--- Reset this layout. The widget will be unreferenced, the margins set to 0
-- and the color erased
function margin:reset()
    self:set_widget(nil)
    self:set_margins(0)
    self:set_color(nil)
end

--- Set the left margin that this layout adds to its widget.
-- @param margin The new margin to use.
-- @property left

--- Set the right margin that this layout adds to its widget.
-- @param margin The new margin to use.
-- @property right

--- Set the top margin that this layout adds to its widget.
-- @param margin The new margin to use.
-- @property top

--- Set the bottom margin that this layout adds to its widget.
-- @param margin The new margin to use.
-- @property bottom

-- Create setters for each direction
for _, v in pairs({ "left", "right", "top", "bottom" }) do
    margin["set_" .. v] = function(layout, val)
        if layout._private[v] == val then return end
        layout._private[v] = val
        layout:emit_signal("widget::layout_changed")
    end

    margin["get_" .. v] = function(layout)
        return layout._private[v]
    end
end

--- Returns a new margin container.
-- @param[opt] widget A widget to use.
-- @param[opt] left A margin to use on the left side of the widget.
-- @param[opt] right A margin to use on the right side of the widget.
-- @param[opt] top A margin to use on the top side of the widget.
-- @param[opt] bottom A margin to use on the bottom side of the widget.
-- @param[opt] color A color for the margins.
-- @param[opt] draw_empty whether or not to draw the margin when the content is empty
-- @treturn table A new margin container
-- @function wibox.container.margin
local function new(widget, left, right, top, bottom, color, draw_empty)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, margin, true)

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

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(margin, margin.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
