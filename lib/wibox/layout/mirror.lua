---------------------------------------------------------------------------
-- @author dodo
-- @copyright 2012 dodo
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.mirror
---------------------------------------------------------------------------

local type = type
local error = error
local pairs = pairs
local ipairs = ipairs
local setmetatable = setmetatable
local base = require("wibox.widget.base")
local Matrix = require("lgi").cairo.Matrix

local mirror = { mt = {} }

--- Layout this layout
function mirror:layout(context, cr, width, height)
    if not self.widget then return end
    if not self.horizontal and not self.vertical then
        base.draw_widget(wibox, cr, self.widget, 0, 0, width, height)
        return
    end

    local m = Matrix.create_identity()
    local t = { x = 0, y = 0 } -- translation
    local s = { x = 1, y = 1 } -- scale
    if self.horizontal then
        t.y = height
        s.y = -1
    end
    if self.vertical then
        t.x = width
        s.x = -1
    end
    m:translate(t.x, t.y)
    m:scale(s.x, s.y)

    return base.place_widget_via_matrix(widget, m, width, height)
end

--- Fit this layout into the given area
function mirror:fit(context, ...)
    if not self.widget then
        return 0, 0
    end
    return base.fit_widget(context, self.widget, ...)
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

--- Returns a new mirror layout. A mirror layout mirrors a given widget. Use
-- :set_widget() to set the widget and
-- :set_horizontal() and :set_vertical() for the direction.
-- horizontal and vertical are by default false which doesn't change anything.
-- @param[opt] widget The widget to display.
-- @param[opt] reflection A table describing the reflection to apply.
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
