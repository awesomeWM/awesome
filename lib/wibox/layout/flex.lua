---------------------------------------------------------------------------
--
--@DOC_wibox_layout_defaults_flex_EXAMPLE@
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @classmod wibox.layout.flex
---------------------------------------------------------------------------

local base = require("wibox.widget.base")
local fixed = require("wibox.layout.fixed")
local table = table
local pairs = pairs
local floor = math.floor
local gmath = require("gears.math")
local gtable = require("gears.table")

local flex = {}

--@DOC_fixed_COMMON@

--- Replace the layout children
-- @tparam table children A table composed of valid widgets
-- @name set_children
-- @class function

--- Add some widgets to the given fixed layout
-- @param layout The layout you are modifying.
-- @tparam widget ... Widgets that should be added (must at least be one)
-- @name add
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

--- Insert a new widget in the layout at position `index`
-- @tparam number index The position
-- @param widget The widget
-- @treturn boolean If the operation is successful
-- @name insert
-- @class function

function flex:layout(_, width, height)
    local result = {}
    local pos,spacing = 0, self._private.spacing
    local num = #self._private.widgets
    local total_spacing = (spacing*(num-1))

    local space_per_item
    if self._private.dir == "y" then
        space_per_item = height / num - total_spacing/num
    else
        space_per_item = width / num - total_spacing/num
    end

    if self._private.max_widget_size then
        space_per_item = math.min(space_per_item, self._private.max_widget_size)
    end

    for _, v in pairs(self._private.widgets) do
        local x, y, w, h
        if self._private.dir == "y" then
            x, y = 0, gmath.round(pos)
            w, h = width, floor(space_per_item)
        else
            x, y = gmath.round(pos), 0
            w, h = floor(space_per_item), height
        end

        table.insert(result, base.place_widget_at(v, x, y, w, h))

        pos = pos + space_per_item + spacing

        if (self._private.dir == "y" and pos-spacing >= height) or
            (self._private.dir ~= "y" and pos-spacing >= width) then
            break
        end
    end

    return result
end

-- Fit the flex layout into the given space.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function flex:fit(context, orig_width, orig_height)
    local used_in_dir = 0
    local used_in_other = 0

    -- Figure out the maximum size we can give out to sub-widgets
    local sub_height = self._private.dir == "x" and orig_height or orig_height / #self._private.widgets
    local sub_width  = self._private.dir == "y" and orig_width  or orig_width / #self._private.widgets

    for _, v in pairs(self._private.widgets) do
        local w, h = base.fit_widget(self, context, v, sub_width, sub_height)

        local max = self._private.dir == "y" and w or h
        if max > used_in_other then
            used_in_other = max
        end

        used_in_dir = used_in_dir + (self._private.dir == "y" and h or w)
    end

    if self._private.max_widget_size then
        used_in_dir = math.min(used_in_dir,
            #self._private.widgets * self._private.max_widget_size)
    end

    local spacing = self._private.spacing * (#self._private.widgets-1)

    if self._private.dir == "y" then
        return used_in_other, used_in_dir + spacing
    end
    return used_in_dir + spacing, used_in_other
end

--- Set the maximum size the widgets in this layout will take.
--That is, maximum width for horizontal and maximum height for vertical.
-- @property max_widget_size
-- @param number

function flex:set_max_widget_size(val)
    if self._private.max_widget_size ~= val then
        self._private.max_widget_size = val
        self:emit_signal("widget::layout_changed")
    end
end

local function get_layout(dir, widget1, ...)
    local ret = fixed[dir](widget1, ...)

    gtable.crush(ret, flex, true)

    ret._private.fill_space = nil

    return ret
end

--- Returns a new horizontal flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
-- @function wibox.layout.flex.horizontal
function flex.horizontal(...)
    return get_layout("horizontal", ...)
end

--- Returns a new vertical flex layout. A flex layout shares the available space
-- equally among all widgets. Widgets can be added via :add(widget).
-- @tparam widget ... Widgets that should be added to the layout.
-- @function wibox.layout.flex.vertical
function flex.vertical(...)
    return get_layout("vertical", ...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return flex

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
