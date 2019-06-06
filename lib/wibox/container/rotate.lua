---------------------------------------------------------------------------
-- A container rotating the conained widget by 90 degrees.
--
--@DOC_wibox_container_defaults_rotate_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @containermod wibox.container.rotate
---------------------------------------------------------------------------

local error = error
local pi = math.pi
local setmetatable = setmetatable
local tostring = tostring
local base = require("wibox.widget.base")
local matrix = require("gears.matrix")
local gtable = require("gears.table")

local rotate = { mt = {} }

local function transform(layout, width, height)
    local dir = layout:get_direction()
    if dir == "east" or dir == "west" then
        return height, width
    end
    return width, height
end

-- Layout this layout
function rotate:layout(_, width, height)
    if not self._private.widget or not self._private.widget._private.visible then
        return
    end

    local dir = self:get_direction()

    local m = matrix.identity
    if dir == "west" then
        m = m:rotate(pi / 2)
        m = m:translate(0, -width)
    elseif dir == "south" then
        m = m:rotate(pi)
        m = m:translate(-width, -height)
    elseif dir == "east" then
        m = m:rotate(3 * pi / 2)
        m = m:translate(-height, 0)
    end

    -- Since we rotated, we might have to swap width and height.
    -- transform() does that for us.
    return { base.place_widget_via_matrix(self._private.widget, m, transform(self, width, height)) }
end

-- Fit this layout into the given area
function rotate:fit(context, width, height)
    if not self._private.widget then
        return 0, 0
    end
    return transform(self, base.fit_widget(self, context, self._private.widget, transform(self, width, height)))
end

--- The widget to be rotated.
-- @property widget
-- @tparam widget widget The widget

function rotate:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function rotate:get_widget()
    return self._private.widget
end

function rotate:get_children()
    return {self._private.widget}
end

function rotate:set_children(children)
    self:set_widget(children[1])
end

--- Reset this layout. The widget will be removed and the rotation reset.
-- @method reset
function rotate:reset()
    self._private.direction = nil
    self:set_widget(nil)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

--- The direction of this rotating container.
-- Valid values are:
--
-- * *north*
-- * *east*
-- * *south*
-- * *north*
--
--@DOC_wibox_container_rotate_angle_EXAMPLE@
-- @property direction
-- @tparam string dir The direction

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

    self._private.direction = dir
    self:emit_signal("widget::layout_changed")
end

-- Get the direction of this rotating layout
function rotate:get_direction()
    return self._private.direction or "north"
end

--- Returns a new rotate container.
-- A rotate container rotates a given widget. Use
-- :set_widget() to set the widget and :set_direction() for the direction.
-- The default direction is "north" which doesn't change anything.
-- @param[opt] widget The widget to display.
-- @param[opt] dir The direction to rotate to.
-- @treturn table A new rotate container.
-- @function wibox.container.rotate
local function new(widget, dir)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, rotate, true)

    ret:set_widget(widget)
    ret:set_direction(dir or "north")

    return ret
end

function rotate.mt:__call(...)
    return new(...)
end

return setmetatable(rotate, rotate.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
