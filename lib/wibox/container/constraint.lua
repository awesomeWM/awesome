---------------------------------------------------------------------------
--
--@DOC_wibox_container_defaults_constraint_EXAMPLE@
-- @author Lukáš Hrázký
-- @copyright 2012 Lukáš Hrázký
-- @containermod wibox.container.constraint
---------------------------------------------------------------------------

local setmetatable = setmetatable
local base = require("wibox.widget.base")
local gtable = require("gears.table")
local math = math

local constraint = { mt = {} }

-- Layout a constraint layout
function constraint:layout(_, width, height)
    if self._private.widget then
        return { base.place_widget_at(self._private.widget, 0, 0, width, height) }
    end
end

-- Fit a constraint layout into the given space
function constraint:fit(context, width, height)
    local w, h
    if self._private.widget then
        w = self._private.strategy(width, self._private.width)
        h = self._private.strategy(height, self._private.height)

        w, h = base.fit_widget(self, context, self._private.widget, w, h)
    else
        w, h = 0, 0
    end

    w = self._private.strategy(w, self._private.width)
    h = self._private.strategy(h, self._private.height)

    return w, h
end

--- The widget to be constrained.
-- @property widget
-- @tparam widget widget The widget

function constraint:set_widget(widget)
    self._private.widget = widget
    self:emit_signal("widget::layout_changed")
end

function constraint:get_widget()
    return self._private.widget
end

--- Get the number of children element
-- @treturn table The children
function constraint:get_children()
    return {self._private.widget}
end

--- Replace the layout children
-- This layout only accept one children, all others will be ignored
-- @tparam table children A table composed of valid widgets
function constraint:set_children(children)
    self:set_widget(children[1])
end

--- Set the strategy to use for the constraining. Valid values are 'max',
-- 'min' or 'exact'. Throws an error on invalid values.
-- @property strategy

function constraint:set_strategy(val)
    local func = {
        min = function(real_size, limit)
            return limit and math.max(limit, real_size) or real_size
        end,
        max = function(real_size, limit)
            return limit and math.min(limit, real_size) or real_size
        end,
        exact = function(real_size, limit)
            return limit or real_size
        end
    }

    if not func[val] then
        error("Invalid strategy for constraint layout: " .. tostring(val))
    end

    self._private.strategy = func[val]
    self:emit_signal("widget::layout_changed")
end

function constraint:get_strategy()
    return self._private.strategy
end

--- Set the maximum width to val. nil for no width limit.
-- @property height
-- @param number

function constraint:set_width(val)
    self._private.width = val
    self:emit_signal("widget::layout_changed")
end

function constraint:get_width()
    return self._private.width
end

--- Set the maximum height to val. nil for no height limit.
-- @property width
-- @param number

function constraint:set_height(val)
    self._private.height = val
    self:emit_signal("widget::layout_changed")
end

function constraint:get_height()
    return self._private.height
end

--- Reset this layout. The widget will be unreferenced, strategy set to "max"
-- and the constraints set to nil.
function constraint:reset()
    self._private.width = nil
    self._private.height = nil
    self:set_strategy("max")
    self:set_widget(nil)
end

--- Returns a new constraint container.
-- This container will constraint the size of a
-- widget according to the strategy. Note that this will only work for layouts
-- that respect the widget's size, eg. fixed layout. In layouts that don't
-- (fully) respect widget's requested size, the inner widget still might get
-- drawn with a size that does not fit the constraint, eg. in flex layout.
-- @param[opt] widget A widget to use.
-- @param[opt] strategy How to constraint the size. 'max' (default), 'min' or
-- 'exact'.
-- @param[opt] width The maximum width of the widget. nil for no limit.
-- @param[opt] height The maximum height of the widget. nil for no limit.
-- @treturn table A new constraint container
-- @function wibox.container.constraint
local function new(widget, strategy, width, height)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, constraint, true)

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

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(constraint, constraint.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
