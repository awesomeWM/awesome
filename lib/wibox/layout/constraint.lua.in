---------------------------------------------------------------------------
-- @author Lukáš Hrázký
-- @copyright 2012 Lukáš Hrázký
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local pairs = pairs
local type = type
local setmetatable = setmetatable
local base = require("wibox.layout.base")
local widget_base = require("wibox.widget.base")
local math = math

-- wibox.layout.constraint
local constraint = { mt = {} }

--- Draw a constraint layout
function constraint:draw(wibox, cr, width, height)
    if not self.widget then
        return
    end

    base.draw_widget(wibox, cr, self.widget, 0, 0, width, height)
end

--- Fit a constraint layout into the given space
function constraint:fit(width, height)
    local w, h
    if self.widget then
        w = self._strategy(width, self._width)
        h = self._strategy(height, self._height)

        w, h = base.fit_widget(self.widget, w, h)
    else
        w, h = 0, 0
    end

    w = self._strategy(w, self._width)
    h = self._strategy(h, self._height)

    return w, h
end

--- Set the widget that this layout adds a constraint on.
function constraint:set_widget(widget)
    if self.widget then
        self.widget:disconnect_signal("widget::updated", self._emit_updated)
    end
    if widget then
        widget_base.check_widget(widget)
        widget:connect_signal("widget::updated", self._emit_updated)
    end
    self.widget = widget
    self:emit_signal("widget::updated")
end

--- Set the strategy to use for the constraining. Valid values are 'max',
-- 'min' or 'exact'. Throws an error on invalid values.
function constraint:set_strategy(val)
    local func = {
        min = function(real_size, constraint)
            return constraint and math.max(constraint, real_size) or real_size
        end,
        max = function(real_size, constraint)
            return constraint and math.min(constraint, real_size) or real_size
        end,
        exact = function(real_size, constraint)
            return constraint or real_size
        end
    }

    if not func[val] then
        error("Invalid strategy for constraint layout: " .. tostring(val))
    end

    self._strategy = func[val]
    self:emit_signal("widget::updated")
end

--- Set the maximum width to val. nil for no width limit.
function constraint:set_width(val)
    self._width = val
    self:emit_signal("widget::updated")
end

--- Set the maximum height to val. nil for no height limit.
function constraint:set_height(val)
    self._height = val
    self:emit_signal("widget::updated")
end

--- Reset this layout. The widget will be unreferenced, strategy set to "max"
-- and the constraints set to nil.
function constraint:reset()
    self._width = nil
    self._height = nil
    self:set_strategy("max")
    self:set_widget(nil)
end

--- Returns a new constraint layout. This layout will constraint the size of a
-- widget according to the strategy. Note that this will only work for layouts
-- that respect the widget's size, eg. fixed layout. In layouts that don't
-- (fully) respect widget's requested size, the inner widget still might get
-- drawn with a size that does not fit the constraint, eg. in flex layout.
-- @param widget A widget to use (optional)
-- @param strategy How to constraint the size. 'max' (default), 'min' or
-- 'exact'. (optional)
-- @param width The maximum width of the widget. nil for no limit. (optional)
-- @param height The maximum height of the widget. nil for no limit. (optional)
local function new(widget, strategy, width, height)
    local ret = widget_base.make_widget()

    for k, v in pairs(constraint) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._emit_updated = function()
        ret:emit_signal("widget::updated")
    end

    ret:set_strategy(strategy or "max")
    ret:set_width(width)
    ret:set_height(height)

    if widget then
        ret:set_widget(widget)
    end

    return ret
end

function constraint.mt:__call(...)
    return new(...)
end

return setmetatable(constraint, constraint.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
