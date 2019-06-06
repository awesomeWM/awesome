---------------------------------------------------------------------------
-- A stacked layout.
--
-- This layout display widgets on top of each other. It can be used to overlay
-- a `wibox.widget.textbox` on top of a `awful.widget.progressbar` or manage
-- "pages" where only one is visible at any given moment.
--
-- The indices are going from 1 (the bottom of the stack) up to the top of
-- the stack. The order can be changed either using `:swap` or `:raise`.
--
--@DOC_wibox_layout_defaults_stack_EXAMPLE@
-- @author Emmanuel Lepage Vallee
-- @copyright 2016 Emmanuel Lepage Vallee
-- @layoutmod wibox.layout.stack
---------------------------------------------------------------------------

local base  = require("wibox.widget.base" )
local fixed = require("wibox.layout.fixed")
local table = table
local pairs = pairs
local gtable  = require("gears.table")

local stack = {mt={}}

--@DOC_fixed_COMMON@

--- Add some widgets to the given stack layout
-- @param layout The layout you are modifying.
-- @tparam widget ... Widgets that should be added (must at least be one)
-- @name add
-- @class function

--- Remove a widget from the layout
-- @tparam index The widget index to remove
-- @treturn boolean index If the operation is successful
-- @name remove
-- @class function

--- Insert a new widget in the layout at position `index`
-- @tparam number index The position
-- @param widget The widget
-- @treturn boolean If the operation is successful
-- @name insert
-- @class function

--- Remove one or more widgets from the layout
-- The last parameter can be a boolean, forcing a recursive seach of the
-- widget(s) to remove.
-- @param widget ... Widgets that should be removed (must at least be one)
-- @treturn boolean If the operation is successful
-- @name remove_widgets
-- @class function

--- Add spacing around the widget, similar to the margin container.
--@DOC_wibox_layout_stack_spacing_EXAMPLE@
-- @property spacing
-- @tparam number spacing Spacing between widgets.

function stack:layout(_, width, height)
    local result = {}
    local spacing = self._private.spacing

    width  = width  - math.abs(self._private.h_offset * #self._private.widgets) - 2*spacing
    height = height - math.abs(self._private.v_offset * #self._private.widgets) - 2*spacing

    local h_off, v_off = spacing, spacing

    for _, v in pairs(self._private.widgets) do
        table.insert(result, base.place_widget_at(v, h_off, v_off, width, height))
        h_off, v_off = h_off + self._private.h_offset, v_off + self._private.v_offset
        if self._private.top_only then break end
    end

    return result
end

function stack:fit(context, orig_width, orig_height)
    local max_w, max_h = 0,0
    local spacing = self._private.spacing

    for _, v in pairs(self._private.widgets) do
        local w, h = base.fit_widget(self, context, v, orig_width, orig_height)
        max_w, max_h = math.max(max_w, w+2*spacing), math.max(max_h, h+2*spacing)
    end

    return math.min(max_w, orig_width), math.min(max_h, orig_height)
end

--- If only the first stack widget is drawn
-- @property top_only

function stack:get_top_only()
    return self._private.top_only
end

function stack:set_top_only(top_only)
    self._private.top_only = top_only
end

--- Raise a widget at `index` to the top of the stack
-- @tparam number index the widget index to raise
function stack:raise(index)
    if (not index) or self._private.widgets[index] then return end

    local w = self._private.widgets[index]
    table.remove(self._private.widgets, index)
    table.insert(self._private.widgets, w)

    self:emit_signal("widget::layout_changed")
end

--- Raise the first instance of `widget`
-- @param widget The widget to raise
-- @tparam[opt=false] boolean recursive Also look deeper in the hierarchy to
--   find the widget
function stack:raise_widget(widget, recursive)
    local idx, layout = self:index(widget, recursive)

    if not idx or not layout then return end

    -- Bubble up in the stack until the right index is found
    while layout and layout ~= self do
        idx, layout = self:index(layout, recursive)
    end

    if layout == self and idx ~= 1 then
        self:raise(idx)
    end
end

--- Add an horizontal offset to each layers.
--
-- Note that this reduces the overall size of each widgets by the sum of all
-- layers offsets.
--
--@DOC_wibox_layout_stack_offset_EXAMPLE@
--
-- @property horizontal_offset
-- @param number

--- Add an vertical offset to each layers.
--
-- Note that this reduces the overall size of each widgets by the sum of all
-- layers offsets.
--
-- @property vertial_offset
-- @param number
-- @see horizontal_offset

function stack:set_horizontal_offset(value)
    self._private.h_offset = value
    self:emit_signal("widget::layout_changed")
end

function stack:set_vertical_offset(value)
    self._private.v_offset = value
    self:emit_signal("widget::layout_changed")
end

--- Create a new stack layout.
-- @function wibox.layout.stack
-- @treturn widget A new stack layout

local function new(...)
    local ret = fixed.horizontal(...)

    gtable.crush(ret, stack, true)

    ret._private.h_offset = 0
    ret._private.v_offset = 0

    return ret
end

function stack.mt:__call(_, ...)
    return new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(stack, stack.mt)
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
