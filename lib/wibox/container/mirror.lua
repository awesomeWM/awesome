---------------------------------------------------------------------------
--
--@DOC_wibox_container_defaults_mirror_EXAMPLE@
-- @author dodo
-- @copyright 2012 dodo
-- @classmod wibox.container.mirror
---------------------------------------------------------------------------

local type = type
local error = error
local ipairs = ipairs
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local matrix = require("gears.matrix")
local gtable = require("gears.table")

local mirror = { mt = {} }

-- Layout this layout
function mirror:layout(_, width, height)
    if not self._private.widget then return end

    local m = matrix.identity
    local t = { x = 0, y = 0 } -- translation
    local s = { x = 1, y = 1 } -- scale
    if self._private.horizontal then
        t.x = width
        s.x = -1
    end
    if self._private.vertical then
        t.y = height
        s.y = -1
    end
    m = m:translate(t.x, t.y)
    m = m:scale(s.x, s.y)

    return { base.place_widget_via_matrix(self._private.widget, m, width, height) }
end

-- Fit this layout into the given area
function mirror:fit(context, ...)
    if not self._private.widget then
        return 0, 0
    end
    return base.fit_widget(self, context, self._private.widget, ...)
end

--- The widget to be reflected.
-- @property widget
-- @tparam widget widget The widget

function mirror:set_widget(widget)
    if widget then
        base.check_widget(widget)
    end
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function mirror:get_widget()
    return self._private.widget
end

--- Get the number of children element
-- @treturn table The children
function mirror:get_children()
    return {self._private.widget}
end

--- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function mirror:set_children(children)
    self:set_widget(children[1])
end

--- Reset this layout. The widget will be removed and the axes reset.
function mirror:reset()
    self._private.horizontal = false
    self._private.vertical = false
    self:set_widget(nil)
end

function mirror:set_reflection(reflection)
    if type(reflection) ~= 'table' then
        error("Invalid type of reflection for mirror layout: " ..
              type(reflection) .. " (should be a table)")
    end
    for _, ref in ipairs({"horizontal", "vertical"}) do
        if reflection[ref] ~= nil then
            self._private[ref] = reflection[ref]
        end
    end
    self:emit_signal("widget::layout_changed")
end

--- Get the reflection of this mirror layout.
-- @property reflection
-- @param table reflection A table of booleans with the keys "horizontal", "vertical".
-- @param boolean reflection.horizontal
-- @param boolean reflection.vertical

function mirror:get_reflection()
    return { horizontal = self._private.horizontal, vertical = self._private.vertical }
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
    local ret = base.make_widget(nil, nil, {enable_properties = true})
    ret._private.horizontal = false
    ret._private.vertical = false

    gtable.crush(ret, mirror, true)

    ret:set_widget(widget)
    ret:set_reflection(reflection or {})

    return ret
end

function mirror.mt:__call(...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(mirror, mirror.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
