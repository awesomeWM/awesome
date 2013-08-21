---------------------------------------------------------------------------
-- @author dodo
-- @copyright 2012 dodo
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local type = type
local error = error
local pairs = pairs
local ipairs = ipairs
local setmetatable = setmetatable
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")

-- wibox.layout.mirror
local mirror = { mt = {} }

--- Draw this layout
function mirror:draw(wibox, cr, width, height)
    if not self.widget then return { width = 0, height = 0 } end
    if not self.horizontal and not self.vertical then
        self.widget:draw(wibox, cr, width, height)
        return -- nothing changed
    end

    cr:save()

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
    cr:translate(t.x, t.y)
    cr:scale(s.x, s.y)

    self.widget:draw(wibox, cr, width, height)

    -- Undo the scale and translation from above.
    cr:restore()
end

--- Fit this layout into the given area
function mirror:fit(...)
    if not self.widget then
        return 0, 0
    end
    return base.fit_widget(self.widget, ...)
end

--- Set the widget that this layout mirrors.
-- @param widget The widget to mirror
function mirror:set_widget(widget)
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
    self._emit_updated()
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
-- @param widget The widget to display (optional)
-- @param reflection A table describing the reflection to apply (optional)
local function new(widget, reflection)
    local ret = widget_base.make_widget()
    ret.horizontal = false
    ret.vertical = false

    for k, v in pairs(mirror) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
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
