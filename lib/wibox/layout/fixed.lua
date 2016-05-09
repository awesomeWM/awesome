---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @classmod wibox.layout.fixed
---------------------------------------------------------------------------

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local base  = require("wibox.widget.base")
local table = table
local pairs = pairs
local util = require("awful.util")

local fixed = {}

--@DOC_fixed_COMMON@

-- Layout a fixed layout. Each widget gets just the space it asks for.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function fixed:layout(context, width, height)
    local result = {}
    local pos,spacing = 0, self._spacing

    for k, v in pairs(self.widgets) do
        local x, y, w, h, _
        if self.dir == "y" then
            x, y = 0, pos
            w, h = width, height - pos
            if k ~= #self.widgets or not self._fill_space then
                _, h = base.fit_widget(self, context, v, w, h);
            end
            pos = pos + h + spacing
        else
            x, y = pos, 0
            w, h = width - pos, height
            if k ~= #self.widgets or not self._fill_space then
                w, _ = base.fit_widget(self, context, v, w, h);
            end
            pos = pos + w + spacing
        end

        if (self.dir == "y" and pos-spacing > height) or
            (self.dir ~= "y" and pos-spacing > width) then
            break
        end
        table.insert(result, base.place_widget_at(v, x, y, w, h))
    end
    return result
end

--- Add some widgets to the given fixed layout
-- @param ... Widgets that should be added (must at least be one)
function fixed:add(...)
    -- No table.pack in Lua 5.1 :-(
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    for i=1, args.n do
        base.check_widget(args[i])
        table.insert(self.widgets, args[i])
    end
    self:emit_signal("widget::layout_changed")
end


--- Remove a widget from the layout
-- @tparam number index The widget index to remove
-- @treturn boolean index If the operation is successful
function fixed:remove(index)
    if not index or index < 1 or index > #self.widgets then return false end

    table.remove(self.widgets, index)

    self:emit_signal("widget::layout_changed")

    return true
end

--- Remove one or more widgets from the layout
-- The last parameter can be a boolean, forcing a recursive seach of the
-- widget(s) to remove.
-- @param widget ... Widgets that should be removed (must at least be one)
-- @treturn boolean If the operation is successful
function fixed:remove_widgets(...)
    local args = { ... }

    local recursive = type(args[#args]) == "boolean" and args[#args]

    local ret = true
    for k, rem_widget in ipairs(args) do
        if recursive and k == #args then break end

        local idx, l = self:index(rem_widget, recursive)

        if idx and l and l.remove then
            l:remove(idx, false)
        else
            ret = false
        end

    end

    return #args > (recursive and 1 or 0) and ret
end

--- Get all children of this layout
-- @treturn table a list of all widgets
function fixed:get_children()
    return self.widgets
end

--- Replace the layout children
-- @tparam table children A table composed of valid widgets
function fixed:set_children(children)
    self:reset()
    if #children > 0 then
        self:add(unpack(children))
    end
end

--- Replace the first instance of `widget` in the layout with `widget2`
-- @param widget The widget to replace
-- @param widget2 The widget to replace `widget` with
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
function fixed:replace_widget(widget, widget2, recursive)
    local idx, l = self:index(widget, recursive)

    if idx and l then
        l:set(idx, widget2)
        return true
    end

    return false
end

function fixed:swap(index1, index2)
    if not index1 or not index2 or index1 > #self.widgets
        or index2 > #self.widgets then
        return false
    end

    local widget1, widget2 = self.widgets[index1], self.widgets[index2]

    self:set(index1, widget2)
    self:set(index2, widget1)

    return true
end

function fixed:swap_widgets(widget1, widget2, recursive)
    base.check_widget(widget1)
    base.check_widget(widget2)

    local idx1, l1 = self:index(widget1, recursive)
    local idx2, l2 = self:index(widget2, recursive)

    if idx1 and l1 and idx2 and l2 and (l1.set or l1.set_widget) and (l2.set or l2.set_widget) then
        if l1.set then
            l1:set(idx1, widget2)
        elseif l1.set_widget then
            l1:set_widget(widget2)
        end
        if l2.set then
            l2:set(idx2, widget1)
        elseif l2.set_widget then
            l2:set_widget(widget1)
        end

        return true
    end

    return false
end

function fixed:set(index, widget2)
    if (not widget2) or (not self.widgets[index]) then return false end

    base.check_widget(widget2)

    self.widgets[index] = widget2

    self:emit_signal("widget::layout_changed")

    return true
end

--- Insert a new widget in the layout at position `index`
-- @tparam number index The position
-- @param widget The widget
-- @treturn boolean If the operation is successful
function fixed:insert(index, widget)
    if not index or index < 1 or index > #self.widgets + 1 then return false end

    base.check_widget(widget)
    table.insert(self.widgets, index, widget)
    self:emit_signal("widget::layout_changed")

    return true
end

-- Fit the fixed layout into the given space
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function fixed:fit(context, orig_width, orig_height)
    local width, height = orig_width, orig_height
    local used_in_dir, used_max = 0, 0

    for _, v in pairs(self.widgets) do
        local w, h = base.fit_widget(self, context, v, width, height)
        local in_dir, max
        if self.dir == "y" then
            max, in_dir = w, h
            height = height - in_dir
        else
            in_dir, max = w, h
            width = width - in_dir
        end
        if max > used_max then
            used_max = max
        end
        used_in_dir = used_in_dir + in_dir

        if width <= 0 or height <= 0 then
            if self.dir == "y" then
                used_in_dir = orig_height
            else
                used_in_dir = orig_width
            end
            break
        end
    end

    local spacing = self._spacing * (#self.widgets-1)

    if self.dir == "y" then
        return used_max, used_in_dir + spacing
    end
    return used_in_dir + spacing, used_max
end

function fixed:reset()
    self.widgets = {}
    self:emit_signal("widget::layout_changed")
end

--- Set the layout's fill_space property. If this property is true, the last
-- widget will get all the space that is left. If this is false, the last widget
-- won't be handled specially and there can be space left unused.
function fixed:fill_space(val)
    if self._fill_space ~= val then
        self._fill_space = not not val
        self:emit_signal("widget::layout_changed")
    end
end

local function get_layout(dir, widget1, ...)
    local ret = base.make_widget()

    util.table.crush(ret, fixed)

    ret.dir = dir
    ret.widgets = {}
    ret:set_spacing(0)
    ret:fill_space(false)

    if widget1 then
        ret:add(widget1, ...)
    end

    return ret
end

--- Returns a new horizontal fixed layout. Each widget will get as much space as it
-- asks for and each widget will be drawn next to its neighboring widget.
-- Widgets can be added via :add() or as arguments to this function.
-- @tparam widget ... Widgets that should be added to the layout.
-- @function wibox.layout.fixed.horizontal
function fixed.horizontal(...)
    return get_layout("x", ...)
end

--- Returns a new vertical fixed layout. Each widget will get as much space as it
-- asks for and each widget will be drawn next to its neighboring widget.
-- Widgets can be added via :add() or as arguments to this function.
-- @tparam widget ... Widgets that should be added to the layout.
-- @function wibox.layout.fixed.vertical
function fixed.vertical(...)
    return get_layout("y", ...)
end

--- Add spacing between each layout widgets
-- @param spacing Spacing between widgets.
function fixed:set_spacing(spacing)
    if self._spacing ~= spacing then
        self._spacing = spacing
        self:emit_signal("widget::layout_changed")
    end
end

return fixed

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
