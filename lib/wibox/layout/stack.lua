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
-- @author Emmanuel Lepage Vallee
-- @copyright 2016 Emmanuel Lepage Vallee
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.stack
---------------------------------------------------------------------------

local base  = require("wibox.widget.base" )
local fixed = require("wibox.layout.fixed")
local table = table
local pairs = pairs
local floor = math.floor
local util  = require("awful.util")

local stack = {mt={}}

--- Get all direct children widgets
-- @param layout The layout you are modifying.
-- @return a list of all widgets
-- @name get_children
-- @class function

--- Add some widgets to the given stack layout
-- @param layout The layout you are modifying.
-- @tparam widget ... Widgets that should be added (must at least be one)
-- @name add
-- @class function

--- Set a widget at a specific index, replace the current one
-- @tparam number index A widget or a widget index
-- @param widget2 The widget to take the place of the first one
-- @treturn boolean If the operation is successful
-- @name set
-- @class function

--- Remove a widget from the layout
-- @tparam index The widget index to remove
-- @treturn boolean index If the operation is successful
-- @name remove
-- @class function

--- Reset a stack layout. This removes all widgets from the layout.
-- @param layout The layout you are modifying.
-- @name reset
-- @class function

--- Replace the first instance of `widget` in the layout with `widget2`
-- @param widget The widget to replace
-- @param widget2 The widget to replace `widget` with
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
-- @name replace_widget
-- @class function

--- Swap 2 widgets in a layout
-- @tparam number index1 The first widget index
-- @tparam number index2 The second widget index
-- @treturn boolean If the operation is successful
-- @name swap
-- @class function

--- Swap 2 widgets in a layout
-- If widget1 is present multiple time, only the first instance is swapped
-- @param widget1 The first widget
-- @param widget2 The second widget
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
-- @name swap_widgets
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

--- Add spacing between each layout widgets
-- @param spacing Spacing between widgets.
-- @name set_spacing
-- @class function

--- Layout a stack layout. Each widget get drawn on top of each other
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function stack:layout(context, width, height)
    local result = {}
    local spacing = self._spacing

    for k, v in pairs(self.widgets) do
        table.insert(result, base.place_widget_at(v, spacing, spacing, width - 2*spacing, height - 2*spacing))
        if self._top_only then break end
    end

    return result
end

--- Fit the stack layout into the given space
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function stack:fit(context, orig_width, orig_height)
    local max_w, max_h = 0,0
    local spacing = self._spacing

    for k, v in pairs(self.widgets) do
        local w, h = base.fit_widget(self, context, v, orig_width, orig_height)
        max_w, max_h = math.max(max_w, w+2*spacing), math.max(max_h, h+2*spacing)
    end

    return math.min(max_w, orig_width), math.min(max_h, orig_height)
end

--- Get if only the first stack widget is drawn
-- @return If the only the first stack widget is drawn
function stack:get_display_top_only()
    return self._top_only
end

--- Only draw the first widget of the stack, ignore others
-- @tparam boolean top_only Only draw the top stack widget
function stack:set_display_top_only(top_only)
    self._top_only = top_only
end

--- Raise a widget at `index` to the top of the stack
-- @tparam number index the widget index to raise
function stack:raise(index)
    if (not index) or self.widgets[index] then return end

    local w = self.widgets[index]
    table.remove(self.widgets, index)
    table.insert(self.widgets, w)

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

local function new(dir, widget1, ...)
    local ret = fixed.horizontal(...)

    util.table.crush(ret, stack)

    return ret
end

function stack.mt:__call(...)
    return new(...)
end

return setmetatable(stack, stack.mt)
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
