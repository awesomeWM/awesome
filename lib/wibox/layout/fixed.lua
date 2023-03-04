---------------------------------------------------------------------------
-- Place many widgets in a column or row, until the available space is used up.
--
-- A `fixed` layout may be initialized with any number of child widgets, and
-- during runtime widgets may be added and removed dynamically.
--
-- On the main axis, child widgets are given a fixed size of exactly as much
-- space as they ask for. The layout will then resize according to the sum of
-- all child widgets. If the space available to the layout is not enough to
-- include all child widgets, the excessive ones are not drawn at all.
--
-- Additionally, the layout allows adding empty spacing or even placing a custom
-- spacing widget between the child widget.
--
-- On its secondary axis, the layout's size is determined by the largest child
-- widget. Smaller child widgets are then placed with the same size.
-- Therefore, child widgets may ignore their `forced_width` or `forced_height`
-- properties for vertical and horizontal layouts respectively.
--
--@DOC_wibox_layout_defaults_fixed_EXAMPLE@
--
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @layoutmod wibox.layout.fixed
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local base   = require("wibox.widget.base")
local table  = table
local math   = math
local ipairs = ipairs
local select = select
local gtable = require("gears.table")

local fixed = {}

-- Layout a fixed layout. Each widget gets just the space it asks for.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function fixed:layout(context, width, height)
    local widget_count = #self._private.widgets
    local is_y = self._private.dir == "y"
    local is_x = not is_y
    local spacing = self._private.spacing or 0
    local abs_spacing = math.abs(spacing)
    local offset_spacing = math.min(0, spacing)
    local spacing_widget = spacing ~= 0 and self._private.spacing_widget or nil

    local result = {}
    local x, y = 0, 0
    local empty = true
    local last_widget_placement_info

    for i = 1, widget_count do
        local widget = self._private.widgets[i]
        local w, h = width - x, height - y

        -- Some widget might be zero sized either because this is their
        -- minimum space or just because they are really empty. In this case,
        -- they must still be added to the layout. Otherwise, if their size
        -- change and this layout is resizable, they are lost "forever" until
        -- a full relayout is called on this fixed layout object.
        local zero = false

        if is_y then
            if not empty and h > 0 then
                -- If we want to place another widget we also need space for the spacing widget.
                h = h - spacing
            end

            if h > 0 then
                h = select(2, base.fit_widget(self, context, widget, w, h))
            end

            if h <= 0 then
                h = 0
                zero = true
            end
        else
            if not empty and w > 0 then
                -- If we want to place another widget we also need space for the spacing widget.
                w = w - spacing
            end

            if w > 0 then
                w = select(1, base.fit_widget(self, context, widget, w, h))
            end

            if w <= 0 then
                w = 0
                zero = true
            end
        end

        -- Add the spacing widget (if needed).
        if spacing ~= 0 and i > 1 then
            local nw = (empty or zero) and 0 or (is_x and abs_spacing or w)
            local nh = (empty or zero) and 0 or (is_y and abs_spacing or h)

            if spacing_widget then
                local nx = is_x and x + offset_spacing or x
                local ny = is_y and y + offset_spacing or y
                table.insert(result, base.place_widget_at(spacing_widget, nx, ny, nw, nh))
            end

            x = is_x and x + (nw ~= 0 and spacing or 0) or x
            y = is_y and y + (nh ~= 0 and spacing or 0) or y
        end

        -- Place widget, even if it has zero width/height. Otherwise
        -- any layout change for zero-sized widget would become invisible.
        do
            local nw = w
            local nh = h

            local widget_placement_info
            do
                local nx = x
                local ny = y

                widget_placement_info = base.place_widget_at(widget, nx, ny, nw, nh)
                table.insert(result, widget_placement_info)
            end

            x = is_x and x + nw or x
            y = is_y and y + nh or y

            if not zero then
                last_widget_placement_info = widget_placement_info
            end
        end

        empty = empty and zero
    end

    if self._private.fill_space and last_widget_placement_info then
        if is_y then
            local h = height - y
            if h > 0 then
                -- TODO: Is this `_height` and `_matrix` ok?
                last_widget_placement_info._height = last_widget_placement_info._height + h
            end
        else
            local w = width - x
            if w > 0 then
                -- TODO: Is changing `_width` and `_matrix` ok?
                last_widget_placement_info._width = last_widget_placement_info._width + w
            end
        end
    end

    return result
end

--- Add some widgets to the given layout.
--
-- @method add
-- @tparam widget ... Widgets that should be added (must at least be one).
-- @noreturn
-- @interface layout
function fixed:add(...)
    -- No table.pack in Lua 5.1 :-(
    local args = { n=select('#', ...), ... }
    assert(args.n > 0, "need at least one widget to add")
    for i=1, args.n do
        local w = base.make_widget_from_value(args[i])
        base.check_widget(w)
        table.insert(self._private.widgets, w)
    end
    self:emit_signal("widget::layout_changed")
end


--- Remove a widget from the layout.
--
-- @method remove
-- @tparam number index The widget index to remove
-- @treturn boolean index If the operation is successful
-- @interface layout
function fixed:remove(index)
    if not index or index < 1 or index > #self._private.widgets then return false end

    table.remove(self._private.widgets, index)

    self:emit_signal("widget::layout_changed")

    return true
end

--- Remove one or more widgets from the layout.
--
-- The last parameter can be a boolean, forcing a recursive seach of the
-- widget(s) to remove.
-- @method remove_widgets
-- @tparam widget ... Widgets that should be removed (must at least be one)
-- @treturn boolean If the operation is successful
-- @interface layout
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

function fixed:get_children()
    return self._private.widgets
end

function fixed:set_children(children)
    self:reset()
    if #children > 0 then
        self:add(unpack(children))
    end
end

--- Replace the first instance of `widget` in the layout with `widget2`.
-- @method replace_widget
-- @tparam widget widget The widget to replace
-- @tparam widget widget2 The widget to replace `widget` with
-- @tparam[opt=false] boolean recursive Digg in all compatible layouts to find the widget.
-- @treturn boolean If the operation is successful
-- @interface layout
function fixed:replace_widget(widget, widget2, recursive)
    local idx, l = self:index(widget, recursive)

    if idx and l then
        l:set(idx, widget2)
        return true
    end

    return false
end

function fixed:swap(index1, index2)
    if not index1 or not index2 or index1 > #self._private.widgets
        or index2 > #self._private.widgets then
        return false
    end

    local widget1, widget2 = self._private.widgets[index1], self._private.widgets[index2]

    self:set(index1, widget2)
    self:set(index2, widget1)

    self:emit_signal("widget::swapped", widget1, widget2, index2, index1)

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
            if l1 == self then
                self:emit_signal("widget::swapped", widget1, widget2, idx2, idx1)
            end
        elseif l1.set_widget then
            l1:set_widget(widget2)
        end
        if l2.set then
            l2:set(idx2, widget1)
            if l2 == self then
                self:emit_signal("widget::swapped", widget1, widget2, idx2, idx1)
            end
        elseif l2.set_widget then
            l2:set_widget(widget1)
        end

        return true
    end

    return false
end

function fixed:set(index, widget2)
    if (not widget2) or (not self._private.widgets[index]) then return false end

    base.check_widget(widget2)

    local w = self._private.widgets[index]

    self._private.widgets[index] = widget2

    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::replaced", widget2, w, index)

    return true
end

--- A widget to insert as a separator between child widgets.
--
-- If this property is a valid widget and `spacing` is greater than `0`, a
-- copy of this widget is inserted between each child widget, with its size in
-- the layout's main direction determined by `spacing`.
--
-- By default no widget is used and any `spacing` is applied as an empty offset.
--
--@DOC_wibox_layout_fixed_spacing_widget_EXAMPLE@
--
-- @property spacing_widget
-- @tparam[opt=nil] widget|nil spacing_widget
-- @propemits true false
-- @interface layout

function fixed:set_spacing_widget(wdg)
    self._private.spacing_widget = base.make_widget_from_value(wdg)
    self:emit_signal("widget::layout_changed")
    self:emit_signal("property::spacing_widget", wdg)
end

--- Insert a new widget in the layout at position `index`.
--
-- @method insert
-- @tparam number index The position.
-- @tparam widget widget The widget.
-- @treturn boolean If the operation is successful.
-- @emits widget::inserted
-- @emitstparam widget::inserted widget self The fixed layout.
-- @emitstparam widget::inserted widget widget index The inserted widget.
-- @emitstparam widget::inserted number count The widget count.
-- @interface layout
function fixed:insert(index, widget)
    if not index or index < 1 or index > #self._private.widgets + 1 then return false end

    base.check_widget(widget)
    table.insert(self._private.widgets, index, widget)
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::inserted", widget, #self._private.widgets)

    return true
end

-- Fit the fixed layout into the given space.
-- @param context The context in which we are fit.
-- @param orig_width The available width.
-- @param orig_height The available height.
function fixed:fit(context, orig_width, orig_height)
    local widget_count = #self._private.widgets

    -- When no widgets exist the function can be called with orig_width or
    -- orig_height equal to nil. Exit early in this case.
    if widget_count == 0 then
        return 0, 0
    end

    local is_y = self._private.dir == "y"
    local is_x = not is_y
    local spacing = self._private.spacing or 0
    local abs_spacing = math.abs(spacing)

    local x, y = 0, 0
    local empty = true
    local used_max = 0

    for i = 1, widget_count do
        local widget = self._private.widgets[i]
        local w, h = orig_width - x, orig_height - y
        local used = 0

        -- Some widget might be zero sized either because this is their
        -- minimum space or just because they are really empty. In this case,
        -- they must still be added to the layout. Otherwise, if their size
        -- change and this layout is resizable, they are lost "forever" until
        -- a full relayout is called on this fixed layout object.
        local zero = false

        if is_y then
            if not empty and h > 0 then
                -- If we want to place another widget we also need space for the spacing widget.
                h = h - spacing
            end

            if h > 0 then
                used, h = base.fit_widget(self, context, widget, w, h)
            end

            if h <= 0 then
                h = 0
                zero = true
            end
        else
            if not empty and w > 0 then
                -- If we want to place another widget we also need space for the spacing widget.
                w = w - spacing
            end

            if w > 0 then
                w, used = base.fit_widget(self, context, widget, w, h)
            end

            if w <= 0 then
                w = 0
                zero = true
            end
        end

        if used_max < used then
            used_max = used
        end

        -- Add the spacing widget (if needed).
        if spacing ~= 0 and i > 1 then
            local nw = (empty or zero) and 0 or (is_x and abs_spacing or w)
            local nh = (empty or zero) and 0 or (is_y and abs_spacing or h)
            x = is_x and x + (nw ~= 0 and spacing or 0) or x
            y = is_y and y + (nh ~= 0 and spacing or 0) or y
        end

        -- Place widget, even if it has zero width/height. Otherwise
        -- any layout change for zero-sized widget would become invisible.
        x = is_x and x + w or x
        y = is_y and y + h or y

        empty = empty and zero
    end

    if empty then
        return 0, 0
    end

    local size

    if self._private.fill_space then
        size = is_y and orig_height or orig_width
    else
        size = is_y and y or x
    end

    if is_y then
        return used_max, size
    else
        return size, used_max
    end
end

function fixed:reset()
    self._private.widgets = {}
    self:emit_signal("widget::layout_changed")
    self:emit_signal("widget::reseted")
    self:emit_signal("widget::reset")
end

--- Set the layout's fill_space property. If this property is true, the last
-- widget will get all the space that is left. If this is false, the last widget
-- won't be handled specially and there can be space left unused.
-- @property fill_space
-- @tparam[opt=false] boolean fill_space
-- @propemits true false

function fixed:fill_space(val)
    if self._private.fill_space ~= val then
        self._private.fill_space = not not val
        self:emit_signal("widget::layout_changed")
        self:emit_signal("property::fill_space", val)
    end
end

local function get_layout(dir, widget1, ...)
    local ret = base.make_widget(nil, nil, {enable_properties = true})

    gtable.crush(ret, fixed, true)

    ret._private.dir = dir
    ret._private.widgets = {}
    ret:set_spacing(0)
    ret:fill_space(false)

    if widget1 then
        ret:add(widget1, ...)
    end

    return ret
end

--- Creates and returns a new horizontal fixed layout.
--
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.fixed.horizontal
function fixed.horizontal(...)
    return get_layout("x", ...)
end

--- Creates and returns a new vertical fixed layout.
--
-- @tparam widget ... Widgets that should be added to the layout.
-- @constructorfct wibox.layout.fixed.vertical
function fixed.vertical(...)
    return get_layout("y", ...)
end

--- The amount of space inserted between the child widgets.
--
-- If a `spacing_widget` is defined, this value is used for its size.
--
--@DOC_wibox_layout_fixed_spacing_EXAMPLE@
--
-- @property spacing
-- @tparam[opt=0] number spacing Spacing between widgets.
-- @negativeallowed true
-- @propemits true false
-- @interface layout

function fixed:set_spacing(spacing)
    if self._private.spacing ~= spacing then
        self._private.spacing = spacing
        self:emit_signal("widget::layout_changed")
        self:emit_signal("property::spacing", spacing)
    end
end

function fixed:get_spacing()
    return self._private.spacing or 0
end

--@DOC_fixed_COMMON@

return fixed

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
