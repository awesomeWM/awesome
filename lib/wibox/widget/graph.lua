---------------------------------------------------------------------------
--- A graph widget.
--
-- The graph goes from left to right. To change this to right to left, use
-- a `wibox.container.mirror` widget. This can also be used to have data
-- shown from top to bottom.
--
-- To add text on top of the graph, use a `wibox.layout.stack` and a
-- `wibox.container.align` widgets.
--
-- To display the graph vertically, use a `wibox.container.rotate` widget.
--
--@DOC_wibox_widget_defaults_graph_EXAMPLE@
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @widgetmod wibox.widget.graph
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local setmetatable = setmetatable
local ipairs = ipairs
local math = math
local math_max = math.max
local math_min = math.min
local table = table
local color = require("gears.color")
local gdebug = require("gears.debug")
local gtable = require("gears.table")
local base = require("wibox.widget.base")
local beautiful = require("beautiful")

local graph = { mt = {} }

--- Set the graph border_width.
--
--@DOC_wibox_widget_graph_border_width_EXAMPLE@
--
-- @property border_width
-- @tparam number border_width
-- @propemits true false
-- @see border_color

--- Set the graph border color.
--
--@DOC_wibox_widget_graph_border_color_EXAMPLE@
--
-- @property border_color
-- @tparam gears.color border_color The border color to set.
-- @propbeautiful
-- @propemits true false
-- @see gears.color

--- Set the graph foreground color.
--
-- This color is used, when `group_colors` isn't set.
--
--@DOC_wibox_widget_graph_color_EXAMPLE@
--
-- @property color
-- @tparam color color The graph color.
-- @usebeautiful beautiful.graph_fg
-- @propemits true false
-- @see gears.color

--- Set the graph background color.
--
--@DOC_wibox_widget_graph_background_color_EXAMPLE@
--
-- @property background_color
-- @tparam gears.color background_color The graph background color.
-- @usebeautiful beautiful.graph_bg
-- @propemits true false
-- @see gears.color

--- Set the colors for data groups.
--
-- Colors in this table are used to paint respective data groups.
-- When this property is unset (default), the `color` property is used
-- instead for all data groups.
-- When this property is set, but there's no color for a data group in it
-- (i.e. `group_colors`[group] is nil or false), then the respective
-- data group is disabled, i.e. not drawn.
--
-- @DOC_wibox_widget_graph_stacked_group_disable_EXAMPLE@
--
-- @property group_colors
-- @tparam table colors A table with colors for data groups.
-- @see gears.color

--- The maximum value the graph should handle.
--
-- This value corresponds to the top of the graph.
-- If `scale` is also set, the graph never scales up below this value, but it
-- automatically scales down to make all data fit.
-- If `scale` and `max_value` are unset, `max_value` defaults to 1.
--
-- @DOC_wibox_widget_graph_max_value_EXAMPLE@
--
-- @property max_value
-- @tparam number max_value
-- @propemits true false

--- The minimum value the graph should handle.
--
-- This value corresponds to the bottom of the graph.
-- If `scale` is also set, the graph never scales up above this value, but it
-- automatically scales down to make all data fit.
-- If `scale` and `min_value` are unset, `min_value` defaults to 0.
--
-- @DOC_wibox_widget_graph_min_value_EXAMPLE@
--
-- @property min_value
-- @tparam number min_value
-- @propemits true false

--- Set the graph to automatically scale its values. Default is false.
--
-- If this property is set to true, the graph calculates
-- effective `min_value` and `max_value` based on the displayed data,
-- so that all data fits on screen. The properties themselves aren't changed,
-- but the graph is drawn as though `min_value`(`max_value`) were equal to
-- the minimum(maximum) value among itself and the currently drawn values.
-- If `min_value`(`max_value`) is unset, then only the drawn values
-- are considered in this calculation.
--
-- @DOC_wibox_widget_graph_scale1_EXAMPLE@
--
-- @property scale
-- @tparam boolean scale
-- @propemits true false

--- Clamp graph bars to keep them inside the widget for out-of-range values.
--
-- Drawing values outside the [`min_value`, `max_value`] range leads to
-- bar shapes that exceed physical widget dimensions.
-- Most of the time this doesn't matter, because bar shapes are rectangles
-- and bar heights aren't large enough to trigger errors in the drawing system.
-- However for some shapes and values it does make a difference and leads
-- to visibly different and/or invalid result.
--
-- When this property is set to true (the default), the graph clamps
-- bars' heights to keep them within the graph.
--
-- @DOC_wibox_widget_graph_clamp_bars_EXAMPLE@
--
-- @property clamp_bars
-- @tparam boolean clamp_bars
-- @propemits true false

--- The value corresponding to the starting point of graph bars. Default is 0.
--
-- @DOC_wibox_widget_graph_baseline_value_EXAMPLE@
--
-- @property baseline_value
-- @tparam number baseline_value
-- @propemits true false

--- Set the width or the individual steps.
--
--@DOC_wibox_widget_graph_step_EXAMPLE@
--
-- @property step_width
-- @tparam[opt=1] number step_width
-- @propemits true false

--- Set the spacing between the steps.
--
--@DOC_wibox_widget_graph_step_spacing_EXAMPLE@
--
-- @property step_spacing
-- @tparam[opt=0] number step_spacing
-- @propemits true false

--- The step shape.
--
--@DOC_wibox_widget_graph_step_shape_EXAMPLE@
--
-- @property step_shape
-- @tparam[opt=rectangle] gears.shape|function step_shape
-- @propemits true false
-- @see gears.shape

--- Set the graph to draw stacks. Default is false.
--
-- When set to true, bars of each successive data group are drawn on top of
-- bars of previous groups, instead of the baseline.
-- This necessitates all data values to be non-negative.
-- Negative values, if present, will trigger @{nan_color|NaN indication}.
--
-- @DOC_wibox_widget_graph_normal_vs_stacked_EXAMPLE@
--
-- @property stack
-- @tparam boolean stack
-- @propemits true false

--- Display NaN indication. Default is true.
--
-- When the data contains [NaN](https://en.wikipedia.org/wiki/NaN) values,
-- and `nan_indication` is set, the corresponding area,
-- where the value bar should have been drawn, is filled
-- with the `nan_color` from top to bottom.
-- The painting is done after all other data is rendered,
-- to make sure that it won't be overpainted and go unnoticed.
--
-- @DOC_wibox_widget_graph_nan_color_EXAMPLE@
--
-- @property nan_indication
-- @tparam boolean nan_indication
-- @propemits true false

--- The color of NaN indication.
--
-- The color used when `nan_indication` is set.
-- Defaults to a yellow-black diagonal stripes pattern.
--
-- @DOC_wibox_widget_graph_stacked_nan_EXAMPLE@
--
-- @property nan_color
-- @tparam gears.color nan_color The color of NaN indication.
-- @propemits true false
-- @see gears.color

--- The graph foreground color
-- Used, when the `color` property isn't set.
--
-- @beautiful beautiful.graph_fg
-- @param color

--- The graph background color.
-- Used, when the `background_color` property isn't set.
--
-- @beautiful beautiful.graph_bg
-- @param color

--- The graph border color.
-- Used, when the `border_color` property isn't set.
--
-- @beautiful beautiful.graph_border_color
-- @param color

local properties = { "width", "height", "border_color", "stack",
                     "stack_colors", "color", "background_color",
                     "max_value", "scale", "min_value", "step_shape",
                     "step_spacing", "step_width", "border_width",
                     "clamp_bars", "baseline_value",
                     "capacity", "nan_color", "nan_indication",
                     "group_colors",
}

-- This is what the properties are set to on widget construction.
local prop_defaults = {
    baseline_value   = 0,
    clamp_bars       = true,
    nan_indication   = true,
    step_width       = 1,
    step_spacing     = 0,

-- These aren't very useful to set, and the docs don't distinguish between
-- "defaults to" (equals to in a fresh instance) and
-- "falls back to" (is assumed to be equal to, when nil) anyway.
--  scale          = false,
--  stack          = false,
}

-- This is what the properties are assumed to be in the code, when unset/falsy.
local prop_fallbacks = {
-- This one might become beautiful-themed in the future, so we can't set it
-- in the constructor.
    border_width     = 0,

-- These are better left unreplaced in code, because they're used only in one
-- place and the intent is more clear when the numbers are directly visible.
--  min_value        = 0,
--  max_value        = 1,

-- This one is set later. It's not in `prop_defaults`, because I don't
-- want to make it accessible through the getter, lest the user somehow mutates
-- the Cairo pattern and breaks NaN indication for all other graphs.
--  nan_color        = make_fallback_nan_color()
}

-- All property defaults are also necessarily fallbacks.
gtable.crush(prop_fallbacks, prop_defaults, true)

-- These fallbacks are themed and can change on the fly.
setmetatable(prop_fallbacks, {
    __index = function(_, key)
        -- TODO: maybe theme graph.nan_color too?
        if key == "background_color" then
            return beautiful.graph_bg or "#000000aa"
        elseif key == "border_color" then
            return beautiful.graph_border_color or "#ffffff"
        elseif key == "color" then
            return beautiful.graph_fg or "#ff0000"
        end
    end
})

-- This function sets up default implementations for property getters/setters,
-- when none exist yet. It will be called at the end of this class module.
local function build_properties(prototype, prop_names)
    for _, prop in ipairs(prop_names) do
        if not prototype["set_" .. prop] then
            prototype["set_" .. prop] = function(self, value)
                if self._private[prop] ~= value then
                    self._private[prop] = value
                    self:emit_signal("widget::redraw_needed")
                    self:emit_signal("property::"..prop, value)
                end
                return self
            end
        end
        if not prototype["get_" .. prop] then
            prototype["get_" .. prop] = function(self)
                return self._private[prop]
            end
        end
    end
end

-- Creates a yellow-black danger stripe @{gears.color} for NaN indication.
local function build_fallback_nan_color()
    local clr = color.create_pattern_uncached({
        ["type"] = "linear",
        from = {0, 0}, to = {4, 4},
        stops={
            {0, "#000000"},
            {0.25, "#000000"}, {0.25, "#ffff00"},
            {0.50, "#ffff00"}, {0.50, "#000000"},
            {0.75, "#000000"}, {0.75, "#ffff00"},
            {1, "#ffff00"},
        },
    })
    clr:set_extend("REPEAT")
    return clr
end

-- Set up the nan_color fallback.
prop_fallbacks.nan_color = build_fallback_nan_color()

--
-- Module and prototype methods.
--

local function graph_gather_drawn_values_num_stats(self, new_value)
    -- Because the graphs are at risk of dividing by zero, the value can
    -- be `inf`, `-inf` and `-nan`. For example:
    --
    -- nan = 0/0
    -- print(not (nan >= 0), nan < 0)
    -- > true, false
    --
    -- Because if this, we need to ignore the luacheck rule which promotes
    -- the removal of double negative and DeMorgan's law style optimizations.
    -- Those only works on real numbers, which `NaN` and `Inf` are not part of.
    --
    -- The `luacheck` project decided that adding this check by default and
    -- adding annotations where it doesn't work is the way to go
    -- https://github.com/lunarmodules/luacheck/issues/43#issuecomment-1014949507
    if not (new_value >= 0) then return end --luacheck: ignore 581

    local last_value = self._private.last_drawn_values_num or 0
    -- Grow instantly and shrink slow
    if new_value < last_value then
        self._private.last_drawn_values_num = last_value - 1
    else
        self._private.last_drawn_values_num = new_value
    end
end

--- Determine the color to paint a data group with.
--
-- The graph uses this method to choose a color for a given data group.
-- The default implementation uses a color from the `group_colors` table,
-- if present, otherwise it falls back to `color`, then
-- `beautiful.graph_fg` and finally to color red (#ff0000).
--
-- @method pick_data_group_color
-- @tparam number group_idx The index of the data group.
-- @treturn gears.color The color to paint the data group's values with.
function graph:pick_data_group_color(group_idx)
    -- Use an individual group color, if there's one
    local data_group_colors = self._private.group_colors
    local clr = data_group_colors and data_group_colors[group_idx]
    -- Or fall back to some other colors
    return clr or self._private.color or prop_fallbacks.color
end

--- Determine if a data group should be rendered.
--
-- The graph uses this method to decide whether the given data group
-- should get its values rendered.
--
-- The default implementation says yes to all data groups, unless
-- `group_colors` property is set, in which case only those groups are
-- rendered, for which there are colors in the `group_colors` table,
-- so one can e.g. disable groups by setting their colors to false.
--
-- @method should_draw_data_group
-- @tparam number group_idx The index of the data group.
-- @treturn boolean true if the group should be rendered, false otherwise.
-- @local I'm not confident, that this is good API, so I'm making it private.
local function graph_should_draw_data_group(self, group_idx)
    -- This default implementation decides, whether a group should be drawn,
    -- based on presence of colors, for reasons of backward compatibility.
    local data_group_colors = self._private.group_colors
    if not data_group_colors then
        -- The colors table isn't set, all data groups are deemed enabled.
        return true
    end

    -- A group is enabled if it has a color, i.e. nil color means "don't draw it"
    return not not data_group_colors[group_idx]
end

local function graph_preprocess_values(self, values, drawn_values_num)
    -- TODO: elevate to function documentation, if we decide to make it public API.
    --- Preprocesses values before drawing them.
    -- This function can return up to 2 values: drawn_values and scaling_values
    -- The former will be used as values to draw in place of the original data.
    -- The latter will be used as values to scan for min/max value for scaling.
    -- Either can be nil, which means: use values as is.

    -- This default implementation is only used to implement
    -- presumming values for stacked graphs.
    if not self._private.stack then
        return
    end

    -- Prepare to draw a stacked graph

    -- summed_values[i] = sum [1,#values] of values[c][i]
    local summed_values = {}
    -- drawn_values[c][i] = sum [1,c] of values[c][i]
    local drawn_values = {}

    local nan = 0/0

    -- Add stacked values up to get values we need to render
    for group_idx, group_values in ipairs(values) do
        local drawn_row = {}
        drawn_values[group_idx] = drawn_row

        if graph_should_draw_data_group(self, group_idx) then
            for idx, value in ipairs(group_values) do
                if idx > drawn_values_num then
                    break
                end

                -- drawn_values will have NaN values in it due to negatives/NaNs in input.
                -- we can't simply treat them like zeros during rendering,
                -- in case step_shape() draws visible shapes for actual zero values too.
                local acc = summed_values[idx] or 0
                if value >= 0 then
                    acc = acc + value
                    drawn_row[idx] = acc
                else
                    drawn_row[idx] = nan
                end
                summed_values[idx] = acc
            end
        end
    end

    -- In a stacked graph it's sufficient to examine only the last summed row
    -- to determine the max_value, since all values are necessarily >= 0
    -- and the min_value should be always at most 0
    local scaling_values = { {0}, summed_values }

    return drawn_values, scaling_values
end

local function graph_map_value_to_widget_coordinates(self, value, min_value, max_value, height)
    -- Scale the value so that [min_value..max_value] maps to [0..1]
    value = (value - min_value) / (max_value - min_value)

    -- Check whether value is NaN
    if value == value then
        if self._private.clamp_bars then
            -- Don't allow the bar to exceed widget's dimensions
            value = math_min(1, math_max(0, value))
        end

        -- Drawing bars up from the lower edge of the widget
        return height * (1 - value)
    end
    return value --NaN
end

local function graph_choose_coordinate_system(self, scaling_values, drawn_values_num, height)
    local scale = self._private.scale
    local max_value = self._private.max_value or (scale and -math.huge or 1)
    local min_value = self._private.min_value or (scale and math.huge or 0)

    if scale then
        for _, group_values in ipairs(scaling_values) do
            for idx, v in ipairs(group_values) do
                -- Do not let off-screen values affect autoscaling
                if idx > drawn_values_num then
                    break
                end

                -- We don't use math.min/max here to be sure that
                -- min/max_value don't accidentally get assigned a NaN
                if v > max_value then
                    max_value = v
                end
                if min_value > v then
                    min_value = v
                end
            end
        end
        if min_value == max_value then
            -- If all values are equal in an autoscaled graph,
            -- simply draw them in the middle
            min_value, max_value = min_value - 1, max_value + 1
        end
    end

    -- The position of the baseline in value coordinates
    -- It defaults to the usual zero axis
    local baseline_value = self._private.baseline_value or prop_fallbacks.baseline_value
    -- Let's map it into widget coordinates
    local baseline_y = graph_map_value_to_widget_coordinates(
        self, baseline_value, min_value, max_value, height
    )

    return min_value, max_value, baseline_y
end

local function graph_draw_values(self, cr, _, height, drawn_values_num)
    local values = self._private.values

    local step_shape = self._private.step_shape
    local step_spacing = self._private.step_spacing or prop_fallbacks.step_spacing
    local step_width = self._private.step_width or prop_fallbacks.step_width

    -- Cache methods used in the inner loop for a 3x performance boost
    local cairo_rectangle = cr.rectangle
    local cairo_translate = cr.translate
    local cairo_set_matrix = cr.set_matrix
    local map_coords = graph_map_value_to_widget_coordinates

    -- Preserve the transform centered at the top-left corner of the graph
    local pristine_transform = step_shape and cr:get_matrix()

    local drawn_values, scaling_values = graph_preprocess_values(self, values, drawn_values_num)
    -- If preprocessor returned drawn_values = nil, then simply draw the values we have
    drawn_values = drawn_values or values
    -- If preprocessor returned scaling_values = nil, then
    -- all drawn values need to be examined to determine proper scaling
    scaling_values = scaling_values or drawn_values

    local min_value, max_value, baseline_y = graph_choose_coordinate_system(
        self, scaling_values, drawn_values_num, height
    )

    local nan_x = self._private.nan_indication and {}
    local prev_y = self._private.stack and {}

    for group_idx, group_values in ipairs(drawn_values) do
        if graph_should_draw_data_group(self, group_idx) then
            -- Set the data series' color early, in case the user
            -- wants to do their own painting inside step_shape()
            cr:set_source(color(self:pick_data_group_color(group_idx)))

            for i = 1, math_min(#group_values, drawn_values_num) do
                local value = group_values[i]

                local value_y = map_coords(self, value, min_value, max_value, height)
                local not_nan = value_y == value_y

                -- The coordinate of the i-th bar's left edge
                local x = (i-1)*(step_width + step_spacing)

                if not_nan then
                    local base_y = baseline_y
                    if prev_y then
                        -- Draw from where the previous stacked series left off
                        base_y = prev_y[i] or base_y
                        -- Save our y for the next stacked series
                        prev_y[i] = value_y
                    end

                    if step_shape then
                        -- Shift to the bar beginning
                        cairo_translate(cr, x, value_y)
                        step_shape(cr, step_width, base_y - value_y)
                        -- Undo the shift
                        cairo_set_matrix(cr, pristine_transform)
                    else
                        cairo_rectangle(cr, x, value_y, step_width, base_y - value_y)
                    end
                end

                if not not_nan and nan_x then
                    -- Keep the coordinate to draw NaN indication later
                    table.insert(nan_x, x)
                end
            end

            -- Paint the data series
            cr:fill()
        end
    end

    if nan_x and #nan_x > 0 then
        cr:set_source(color(self._private.nan_color or prop_fallbacks.nan_color))
        for _, x in ipairs(nan_x) do
            -- Draw full-height rectangle with nan_color to indicate NaN
            cairo_rectangle(cr, x, 0, step_width, height)
        end
        cr:fill()
    end
end

function graph:draw(_, cr, width, height)
    local border_width = self._private.border_width or prop_fallbacks.border_width
    local drawn_values_num = self:compute_drawn_values_num(width-2*border_width)

    -- Track our usage to help us guess the necessary values array capacity
    graph_gather_drawn_values_num_stats(self, drawn_values_num)

    -- Draw the background first
    cr:set_source(color(self._private.background_color or prop_fallbacks.background_color))
    cr:paint()

    -- Draw the values
    if drawn_values_num > 0 then
        cr:save()

        -- Account for the border width
        if border_width > 0 then
            cr:translate(border_width, border_width)
        end

        local values_width = width - 2*border_width
        local values_height = height - 2*border_width

        graph_draw_values(self, cr, values_width, values_height, drawn_values_num)

        -- Undo the cr:translate() for the border and step shapes
        cr:restore()
    end

    -- Draw the border last so that it overlaps already drawn values
    if border_width > 0 then
        cr:set_line_width(border_width)
        cr:rectangle(border_width/2, border_width/2, width - border_width, height - border_width)
        cr:set_source(color(self._private.border_color or prop_fallbacks.border_color))
        cr:stroke()
    end
end

function graph:fit(_, width, height)
    return width, height
end

--- Determine how many values should be drawn for a given widget width.
--
-- The graph uses this method to determine the upper bound on the
-- number of values that will be drawn from each data group. This affects,
-- among other things, how many values will be considered for autoscaling,
-- when `scale` is true, and, indirectly, how many values will be kept in
-- the backing array, when `capacity` is unset.
--
-- The default implementation computes the minimum number that is enough
-- to completely cover the given width with `step_width` + `step_spacing`
-- intervals. The graph calls this method on every redraw and the width
-- passed is the width of the value drawing area, i.e the graph borders
-- are subtracted (2\*`border_width`).
--
-- @method compute_drawn_values_num
-- @tparam number usable_width
function graph:compute_drawn_values_num(usable_width)
    if usable_width <= 0 then
        return 0
    end
    local step_width = self._private.step_width or prop_fallbacks.step_width
    local step_spacing = self._private.step_spacing or prop_fallbacks.step_spacing
    return math.ceil(usable_width / (step_width + step_spacing))
end

local function guess_capacity(self)
    local capacity = self._private.capacity
    if capacity then
        -- Ensure it's integer, no matter what the user sets.
        return math.ceil(capacity)
    end

    local ldwn = self._private.last_drawn_values_num
    if not ldwn then
        -- We haven't been drawn even once yet,
        -- maybe the user will push a ton of values now.
        -- Our widget is 8K-display-ready.
        return 8192
    end

    -- Calculate an appropriate capacity from drawn values num
    -- with some wiggle room for widget resizes
    return math.ceil(ldwn/64 + 1)*64
end

--- Add a value to the graph.
--
-- The graph widget keeps its values grouped in _data groups_. Each data group
-- is drawn with its own set of bars, starting with the latest value
-- in the data group at the left edge of the graph.
--
-- Simply calling this method with a particular data group index is the only
-- thing necessary and sufficient for creating a data group.
-- Any natural integer as a group number is ok, but the user is advised to keep
-- the group numbers low and consecutive for performance reasons.
--
-- There are no constraints on the value parameter, other than it should
-- be a number.
--
-- @method add_value
-- @tparam[opt=NaN] number value The value to be added to a graph's data group.
-- @tparam[opt=1] integer group The index of the data group.
function graph:add_value(value, group)
    value = value or 0/0 -- default to NaN
    group = group or 1

    local values = self._private.values
    if not values[group] then
        -- Ensure that there are no gaps in the values array,
        -- so that ipairs() can reach all data groups.
        for i = #values+1, group do
            values[i] = {}
        end
        -- If the above loop hasn't set it, then
        -- `group` wasn't a non-negative integer.
        if not values[group] then
            error("Invalid data group index: " .. tostring(group))
        end
    end
    values = values[group]

    local capacity = guess_capacity(self)
    -- Map negatives, NaNs and zero to nil
    capacity = (capacity >= 1) and capacity

    -- Remove old values over capacity
    -- Invalid capacity means "remove everything"
    local i = capacity or 1
    while values[i] do
        values[i] = nil
        i = i + 1
    end

    if capacity then
        table.insert(values, 1, value)
    end

    self:emit_signal("widget::redraw_needed")
    return self
end

--- Clear the graph.
--
-- Removes all values from all data groups.
--
-- @method clear
function graph:clear()
    self._private.values = {}
    self:emit_signal("widget::redraw_needed")
    return self
end

--- Set the graph capacity.
--
-- Since the typical uses of the graph widget imply that `add_value` will be
-- called an indefinite number of times, the widget needs a way to know, when
-- to start discarding old values from the backing array.
--
-- When `capacity` is set, it defines the maximum number of values to keep in
-- each data group.
--
-- When `capacity` is unset (default), the number is determined heuristically,
-- which is sufficient most of the time, unless the widget gets resized
-- too much too fast.
--
-- @property capacity
-- @tparam[opt=nil] integer|nil capacity The maximum number of values to keep
-- per data group (`nil` for automatic guess).
-- @propemits true false
function graph:set_capacity(capacity)
    -- Property override to avoid emitting the "redraw_needed" signal,
    -- because nothing visibly changes until the next add_value() call,
    -- which emits the signal itself.
    -- It might have been prudent to truncate the values array here
    -- and emit the signal, but I don't think anyone really needs that.
    if self._private.capacity ~= capacity then
        self._private.capacity = capacity
        self:emit_signal("property::capacity", capacity)
    end
    return self
end

--- Set the graph height.
--
-- This property is deprecated.  Use a `wibox.container.constraint` widget or
-- `forced_height`.
---
-- @deprecatedproperty height
-- @tparam number height The height to set.
-- @renamedin 5.0 forced_height
-- @propemits true false
function graph:set_height(height)
    gdebug.deprecate("Use a `wibox.container.constraint` widget or `forced_height`", {deprecated_in=5})
    if awesome.api_level <= 5 then
        if height >= 5 then
            -- this sends "layout_changed" for us
            self:set_forced_height(height)
            -- signal, because we did it before
            self:emit_signal("property::height", height)
        end
        return self
    end
end

function graph:get_height()
    gdebug.deprecate("Use `forced_height`", {deprecated_in=5})
    return awesome.api_level <= 5 and self._private.forced_height or nil
end

--- Set the graph width.
--
-- This property is deprecated.  Use a `wibox.container.constraint` widget or
-- `forced_width`.
---
-- @deprecatedproperty width
-- @tparam number width The width to set.
-- @renamedin 5.0 forced_width
-- @propemits true false
function graph:set_width(width)
    gdebug.deprecate("Use a `wibox.container.constraint` widget or `forced_width`", {deprecated_in=5})
    if awesome.api_level <= 5 then
        if width >= 5 then
            -- this sends "layout_changed" for us
            self:set_forced_width(width)
            -- signal, because we did it before
            self:emit_signal("property::width", width)
        end
        return self
    end
end

function graph:get_width()
    gdebug.deprecate("Use `forced_width`", {deprecated_in=5})
    return awesome.api_level <= 5 and self._private.forced_width or nil
end

--- Set the colors for data groups.
--
-- This property is deprecated. Use `group_colors` instead.
---
-- @deprecatedproperty stack_colors
-- @renamedin 5.0 group_colors
-- @tparam table colors A table with colors for data groups.
-- @see group_colors
function graph:set_stack_colors(colors)
    gdebug.deprecate("Use `group_colors`", {deprecated_in=5})
    if awesome.api_level <= 5 then
        if self._private.group_colors ~= colors then
            -- this sends "redraw_needed" for us
            self:set_group_colors(colors)
            -- signal, because we did it before
            self:emit_signal("property::stack_colors", colors)
        end
        return self
    end
end

function graph:get_stack_colors()
    gdebug.deprecate("Use `group_colors`", {deprecated_in=5})
    return awesome.api_level <= 5 and self._private.group_colors or nil
end


--- Create a graph widget.
--
-- @tparam table args Standard widget() arguments.
-- @treturn wibox.widget.graph A new graph widget.
-- @constructorfct wibox.widget.graph
function graph.new(args)
    args = args or {}

    local _graph = base.make_widget(nil, nil, {enable_properties = true})

    if args.width or args.height then
        gdebug.deprecate(
            "`args.width` and `args.height` are deprecated. "..
            "Use a `wibox.container.constraint` widget "..
            "or `forced_width`/`forced_height`",
            {deprecated_in=5, raw=true}
        )
    end

    if awesome.api_level <= 5 then
        local width = args.width or 100
        local height = args.height or 20

        if width < 5 or height < 5 then return end

        _graph._private.forced_width = width
        _graph._private.forced_height = height
    end

    -- Set initial values for properties.
    gtable.crush(_graph._private, prop_defaults, true)
    _graph._private.values    = {}
    -- Copy methods and properties over
    gtable.crush(_graph, graph, true)
    -- Except those, which don't belong in the widget instance
    rawset(_graph, "new", nil)
    rawset(_graph, "mt", nil)

    return _graph
end

function graph.mt:__call(...)
    return graph.new(...)
end

-- Setup default impls for property accessors that haven't been implemented explicitly.
build_properties(graph, properties)

return setmetatable(graph, graph.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
