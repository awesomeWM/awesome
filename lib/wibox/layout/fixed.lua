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
local base  = require("wibox.widget.base")
local table = table
local pairs = pairs
local gtable = require("gears.table")

local fixed = {}

-- Layout a fixed layout. Each widget gets just the space it asks for.
-- @param context The context in which we are drawn.
-- @param width The available width.
-- @param height The available height.
function fixed:layout(context, width, height)
    local result = {}
    local spacing = self._private.spacing or 0
    local is_y = self._private.dir == "y"
    local is_x = not is_y
    local abspace = math.abs(spacing)
    local spoffset = spacing < 0 and 0 or spacing
    local widgets_nr = #self._private.widgets
    local spacing_widget
    local x, y = 0, 0

    spacing_widget = spacing ~= 0 and self._private.spacing_widget or nil

    for index, widget in pairs(self._private.widgets) do
        local w, h, local_spacing = width - x, height - y, spacing

        -- Some widget might be zero sized either because this is their
        -- minimum space or just because they are really empty. In this case,
        -- they must still be added to the layout. Otherwise, if their size
        -- change and this layout is resizable, they are lost "forever" until
        -- a full relayout is called on this fixed layout object.
        local zero = false

        if is_y then
            if index ~= widgets_nr or not self._private.fill_space then
                h = select(2, base.fit_widget(self, context, widget, w, h))
                zero = h == 0
            end

            if y - spacing >= height then
                -- pop the spacing widget added in previous iteration if used
                if spacing_widget then
                    table.remove(result)

                    -- Avoid adding zero-sized widgets at an out-of-bound
                    -- position.
                    y = y - spacing
                end

                -- Never display "random" widgets as soon as a non-zero sized
                -- one doesn't fit.
                if not zero then
                    break
                end
            end
        else
            if index ~= widgets_nr or not self._private.fill_space then
                w = select(1, base.fit_widget(self, context, widget, w, h))
                zero = w == 0
            end

            if x - spacing >= width then
                -- pop the spacing widget added in previous iteration if used
                if spacing_widget then
                    table.remove(result)

                    -- Avoid adding zero-sized widgets at an out-of-bound
                    -- position.
                    x = x - spacing
                end

                -- Never display "random" widgets as soon as a non-zero sized
                -- one doesn't fit.
                if not zero then
                    break
                end
            end
        end

        if zero then
            local_spacing = 0
        end

        -- Place widget, even if it has zero width/height. Otherwise
        -- any layout change for zero-sized widget would become invisible.
        table.insert(result, base.place_widget_at(widget, x, y, w, h))

        x = is_x and x + w + local_spacing or x
        y = is_y and y + h + local_spacing or y

        -- Add the spacing widget (if needed)
        if index < widgets_nr and spacing_widget then
            table.insert(result, base.place_widget_at(
                spacing_widget,
                is_x and (x - spoffset) or x,
                is_y and (y - spoffset) or y,
                is_x and abspace or w,
                is_y and abspace or h
            ))
        end
    end

    return result
end

--- Add some widgets to the given layout.
--
-- @method add
-- @tparam widget ... Widgets that should be added (must at least be one).
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
-- @tparam widget spacing_widget
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
    local width_left, height_left = orig_width, orig_height
    local spacing = self._private.spacing or 0
    local widgets_nr = #self._private.widgets
    local is_y = self._private.dir == "y"
    local used_max = 0

    -- when no widgets exist the function can be called with orig_width or
    -- orig_height equal to nil. Exit early in this case.
    if widgets_nr == 0 then
        return 0, 0
    end

    for k, v in pairs(self._private.widgets) do
        local w, h = base.fit_widget(self, context, v, width_left, height_left)
        local max

        if is_y then
            max = w
            height_left = height_left - h
        else
            max = h
            width_left = width_left - w
        end

        if max > used_max then
            used_max = max
        end

        if k < widgets_nr then
            if is_y then
                height_left = height_left - spacing
            else
                width_left = width_left - spacing
            end
        end

        if width_left <= 0 or height_left <= 0 then
            -- this complicated two lines determine whether we're out-of-space
            -- because of spacing, or if the last widget doesn't fit in
            if is_y then
                 height_left = k < widgets_nr and height_left + spacing or height_left
                 height_left = height_left < 0 and 0 or height_left
            else
                 width_left = k < widgets_nr and width_left + spacing or width_left
                 width_left = width_left < 0 and 0 or width_left
            end
            break
        end
    end

    if is_y then
        return used_max, orig_height - height_left
    end

    return orig_width - width_left, used_max
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
-- @tparam boolean fill_space
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
-- @tparam number spacing Spacing between widgets.
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
