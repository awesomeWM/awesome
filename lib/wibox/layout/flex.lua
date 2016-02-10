---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.flex
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local fixed = require("wibox.layout.fixed")
local table = table
local pairs = pairs
local floor = math.floor
local util = require("awful.util")

local flex = {}

--- Layout a fixed layout. Each widget gets just the space it asks for.
-- @param layout The layout you are modifying.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
-- @name layout
-- @class function

--- Get all children of this layout
-- @param layout The layout you are modifying.
-- @warning If the widget contain itself and recursive is true, this will cause
--   a stack overflow
-- @param[opt] recursive Also add all widgets of childrens
-- @return a list of all widgets
-- @name get_children
-- @class function

--- Replace the layout children
-- @tparam table children A table composed of valid widgets
-- @name set_children
-- @class function

--- Add some widgets to the given fixed layout
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

--- Remove one or more widgets from the layout
-- The last parameter can be a boolean, forcing a recursive seach of the
-- widget(s) to remove.
-- @param widget ... Widgets that should be removed (must at least be one)
-- @treturn boolean If the operation is successful
-- @name remove_widgets
-- @class function

--- Fit the fixed layout into the given space
-- @param layout The layout you are modifying.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
-- @name fit
-- @class function

--- Reset a fixed layout. This removes all widgets from the layout.
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

--- Layout a flex layout. Each widget gets an equal share of the available space.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function flex:layout(context, width, height)
    local result = {}
    local pos,spacing = 0, self._spacing
    local num = #self.widgets
    local total_spacing = (spacing*(num-1))

    local space_per_item
    if self.dir == "y" then
        space_per_item = height / num - total_spacing/num
    else
        space_per_item = width / num - total_spacing/num
    end

    if self._max_widget_size then
        space_per_item = math.min(space_per_item, self._max_widget_size)
    end

    for k, v in pairs(self.widgets) do
        local x, y, w, h
        if self.dir == "y" then
            x, y = 0, util.round(pos)
            w, h = width, floor(space_per_item)
        else
            x, y = util.round(pos), 0
            w, h = floor(space_per_item), height
        end

        table.insert(result, base.place_widget_at(v, x, y, w, h))

        pos = pos + space_per_item + spacing

        if (self.dir == "y" and pos-spacing >= height) or
            (self.dir ~= "y" and pos-spacing >= width) then
            break
        end
    end

    return result
end

--- Fit the flex layout into the given space.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function flex:fit(context, orig_width, orig_height)
    local used_in_dir = 0
    local used_in_other = 0

    -- Figure out the maximum size we can give out to sub-widgets
    local sub_height = self.dir == "x" and orig_height or orig_height / #self.widgets
    local sub_width  = self.dir == "y" and orig_width  or orig_width / #self.widgets

    for k, v in pairs(self.widgets) do
        local w, h = base.fit_widget(self, context, v, sub_width, sub_height)

        local max = self.dir == "y" and w or h
        if max > used_in_other then
            used_in_other = max
        end

        used_in_dir = used_in_dir + (self.dir == "y" and h or w)
    end

    if self._max_widget_size then
        used_in_dir = math.min(used_in_dir,
            #self.widgets * self._max_widget_size)
    end

    local spacing = self._spacing * (#self.widgets-1)

    if self.dir == "y" then
        return used_in_other, used_in_dir + spacing
    end
    return used_in_dir + spacing, used_in_other
end

--- Set the maximum size the widgets in this layout will take (that is,
-- maximum width for horizontal and maximum height for vertical).
-- @param val The maximum size of the widget.
function flex:set_max_widget_size(val)
    if self._max_widget_size ~= val then
        self._max_widget_size = val
        self:emit_signal("widget::layout_changed")
    end
end

local function get_layout(dir, widget1, ...)
    local ret = fixed[dir](widget1, ...)

    util.table.crush(ret, flex)

    ret.fill_space = nil

    return ret
end

--- Returns a new horizontal flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
function flex.horizontal(...)
    return get_layout("horizontal", ...)
end

--- Returns a new vertical flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
function flex.vertical(...)
    return get_layout("vertical", ...)
end

return flex

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
