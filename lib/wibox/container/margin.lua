---------------------------------------------------------------------------
-- Add a margin around a widget.
--
--@DOC_wibox_container_defaults_margin_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @containermod wibox.container.margin
-- @supermodule wibox.widget.base
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

        local resulting_width = width - x - w
        local resulting_height = height - y - h

        if resulting_width >= 0 and resulting_height >= 0 then
            return { base.place_widget_at(self._private.widget, x, y, resulting_width, resulting_height) }
        end
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
--
-- @property widget
-- @tparam widget widget The widget
-- @interface container

margin.set_widget = base.set_widget_common

function margin:get_widget()
    return self._private.widget
end

function margin:get_children()
    return {self._private.widget}
end

function margin:set_children(children)
    self:set_widget(children[1])
end

--- Set all the margins to val.
--
-- @property margins
-- @tparam number|table val The margin value. It can be a number or a table with
--  the *left*/*right*/*top*/*bottom* keys.
-- @propemits false false

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
    self:emit_signal("property::margins")
end

--- Set the margins color to create a border.
--
-- @property color
-- @param color A color used to fill the margin.
-- @propemits true false

function margin:set_color(color)
    self._private.color = color and gcolor(color)
    self:emit_signal("widget::redraw_needed")
    self:emit_signal("property::color", color)
end

function margin:get_color()
    return self._private.color
end

--- Draw the margin even if the content size is 0x0.
--
-- @property draw_empty
-- @tparam[opt=true] boolean draw_empty Draw nothing is content is 0x0 or draw
--  the margin anyway.
-- @propemits true false

function margin:set_draw_empty(draw_empty)
    self._private.draw_empty = draw_empty
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::draw_empty", draw_empty)
end

function margin:get_draw_empty()
    return self._private.draw_empty
end

--- Reset this layout. The widget will be unreferenced, the margins set to 0
-- and the color erased
-- @method reset
-- @interface container
function margin:reset()
    self:set_widget(nil)
    self:set_margins(0)
    self:set_color(nil)
end

--- Set the left margin that this layout adds to its widget.
--
-- @property left
-- @tparam number left The new margin to use.
-- @propemits true false

--- Set the right margin that this layout adds to its widget.
--
-- @property right
-- @tparam number right The new margin to use.
-- @propemits true false

--- Set the top margin that this layout adds to its widget.
--
-- @property top
-- @tparam number top The new margin to use.
-- @propemits true false

--- Set the bottom margin that this layout adds to its widget.
--
-- @property bottom
-- @tparam number bottom The new margin to use.
-- @propemits true false

-- Create setters for each direction
for _, v in pairs({ "left", "right", "top", "bottom" }) do
    margin["set_" .. v] = function(layout, val)
        if layout._private[v] == val then return end
        layout._private[v] = val
        layout:emit_signal("widget::layout_changed")
        layout:emit_signal("property::".. v, val)
    end

    margin["get_" .. v] = function(layout)
        return layout._private[v]
    end
end

--- Returns a new margin container.
--
-- @param[opt] widget A widget to use.
-- @param[opt] left A margin to use on the left side of the widget.
-- @param[opt] right A margin to use on the right side of the widget.
-- @param[opt] top A margin to use on the top side of the widget.
-- @param[opt] bottom A margin to use on the bottom side of the widget.
-- @param[opt] color A color for the margins.
-- @param[opt] draw_empty whether or not to draw the margin when the content is empty
-- @treturn table A new margin container
-- @constructorfct wibox.container.margin
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

return setmetatable(margin, margin.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
