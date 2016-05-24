---------------------------------------------------------------------------
--
--@DOC_wibox_container_defaults_mirror_EXAMPLE@
-- @author dodo
-- @copyright 2012 dodo
-- @release @AWESOME_VERSION@
-- @classmod wibox.container.mirror
---------------------------------------------------------------------------

local type = type
local error = error
local pairs = pairs
local ipairs = ipairs
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local matrix = require("gears.matrix")

local mirror = { mt = {} }

-- Layout this layout
function mirror:layout(_, width, height)
    if not self.widget then return end

    local m = matrix.identity
    local t = { x = 0, y = 0 } -- translation
    local s = { x = 1, y = 1 } -- scale
    if self.horizontal then
        t.x = width
        s.x = -1
    end
    if self.vertical then
        t.y = height
        s.y = -1
    end
    m = m:translate(t.x, t.y)
    m = m:scale(s.x, s.y)

    return { base.place_widget_via_matrix(self.widget, m, width, height) }
end

-- Fit this layout into the given area
function mirror:fit(context, ...)
    if not self.widget then
        return 0, 0
    end
    return base.fit_widget(self, context, self.widget, ...)
end

--- Set the widget that this layout mirrors.
-- @param widget The widget to mirror
function mirror:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self.widget = widget
    self:emit_signal("widget::layout_changed")
end

--- Get the number of children element
-- @treturn table The children
function mirror:get_children()
    return {self.widget}
end

--- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function mirror:set_children(children)
    self:set_widget(children[1])
end

--- Reset this layout. The widget will be removed and the axes reset.
function mirror:reset()
    self.horizontal = false
    self.vertical = false
    self:set_widget(nil)
end

--- Set the reflection of this mirror layout.
-- @param reflection a table which contains new values for horizontal and/or vertical (booleans)
function mirror:set_reflection(reflection)
    if type(reflection) ~= 'table' then
        error("Invalid type of reflection for mirror layout: " ..
              type(reflection) .. " (should be a table)")
    end
    for _, ref in ipairs({"horizontal", "vertical"}) do
        if reflection[ref] ~= nil then
            self[ref] = reflection[ref]
        end
    end
    self:emit_signal("widget::layout_changed")
end

--- Get the reflection of this mirror layout.
--  @return a table of booleans with the keys "horizontal", "vertical".
function mirror:get_reflection()
    return { horizontal = self.horizontal, vertical = self.vertical }
end

--- Returns a new mirror container.
-- A mirror container mirrors a given widget. Use
-- `:set_widget()` to set the widget and
-- `:set_horizontal()` and `:set_vertical()` for the direction.
-- horizontal and vertical are by default false which doesn't change anything.
-- @param[opt] widget The widget to display.
-- @param[opt] reflection A table describing the reflection to apply.
-- @treturn table A new mirror container
-- @function wibox.container.mirror
local function new(widget, reflection)
    local ret = base.make_widget()
    ret.horizontal = false
    ret.vertical = false

    for k, v in pairs(mirror) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret:set_widget(widget)
    ret:set_reflection(reflection or {})

    return ret
end

function mirror.mt:__call(...)
    return new(...)
end

return setmetatable(mirror, mirror.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
