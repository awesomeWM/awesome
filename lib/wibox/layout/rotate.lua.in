---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local error = error
local pairs = pairs
local pi = math.pi
local type = type
local setmetatable = setmetatable
local tostring = tostring
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")

-- wibox.layout.rotate
local rotate = { mt = {} }

local function transform(layout, width, height)
    local dir = layout:get_direction()
    if dir == "east" or dir == "west" then
        return height, width
    end
    return width, height
end

--- Draw this layout
function rotate:draw(wibox, cr, width, height)
    if not self.widget then return { width = 0, height = 0 } end

    local dir = self:get_direction()

    if dir == "west" then
        cr:rotate(pi / 2)
        cr:translate(0, -width)
    elseif dir == "south" then
        cr:rotate(pi)
        cr:translate(-width, -height)
    elseif dir == "east" then
        cr:rotate(3 * pi / 2)
        cr:translate(-height, 0)
    end

    -- Since we rotated, we might have to swap width and height.
    -- transform() does that for us.
    base.draw_widget(wibox, cr, self.widget, 0, 0, transform(self, width, height))
end

--- Fit this layout into the given area
function rotate:fit(width, height)
    if not self.widget then
        return 0, 0
    end
    return transform(self, base.fit_widget(self.widget, transform(self, width, height)))
end

--- Set the widget that this layout rotates.
function rotate:set_widget(widget)
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

--- Reset this layout. The widget will be removed and the rotation reset.
function rotate:reset()
    self.direction = nil
    self:set_widget(nil)
end

--- Set the direction of this rotating layout. Valid values are "north", "east",
-- "south" and "west". On an invalid value, this function will throw an error.
function rotate:set_direction(dir)
    local allowed = {
        north = true,
        east = true,
        south = true,
        west = true
    }

    if not allowed[dir] then
        error("Invalid direction for rotate layout: " .. tostring(dir))
    end

    self.direction = dir
    self._emit_updated()
end

--- Get the direction of this rotating layout
function rotate:get_direction()
    return self.direction or "north"
end

--- Returns a new rotate layout. A rotate layout rotates a given widget. Use
-- :set_widget() to set the widget and :set_direction() for the direction.
-- The default direction is "north" which doesn't change anything.
-- @param widget The widget to display (optional)
-- @param dir The direction to rotate to (optional)
local function new(widget, dir)
    local ret = widget_base.make_widget()

    for k, v in pairs(rotate) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    ret:set_widget(widget)
    ret:set_direction(dir or "north")

    return ret
end

function rotate.mt:__call(...)
    return new(...)
end

return setmetatable(rotate, rotate.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
