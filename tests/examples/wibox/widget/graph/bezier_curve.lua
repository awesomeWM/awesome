--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = { --DOC_HIDE
    color = require("gears.color"), --DOC_HIDE
    math = require("gears.math"), --DOC_HIDE
    shape = require("gears.shape"), --DOC_HIDE
} --DOC_HIDE

local data = { --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_height = 100, --DOC_HIDE
    forced_width  = 330, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
} --DOC_HIDE

local colors = {
    "#ff0000",
    "#00ff00",
    "#0000ff"
}

local bezier = require("gears.math.bezier")
--DOC_NEWLINE

local function curvaceous(cr, x, y, b, step_width, options, draw_line)
    local interpolate = bezier.cubic_from_derivative_and_points_min_stretch

    local state = options.curvaceous_state
    if not state or state.last_group ~= options.group_idx then
        -- New data series is being drawn, reset state.
        state = { last_group = options.group_idx, x = x, y = y, b = b }
        options.curvaceous_state = state
        return
    end

    -- Compute if the bar needs to be cut due to spacing and how much
    local step_spacing = options.graph.step_spacing
    local step_fraction = step_spacing ~= 0 and step_width/(step_width+step_spacing)

    -- Get coordinates from the previous step
    local x0, y0, b0 = state.x, state.y, state.b
    -- Update coordinates in state
    state.x, state.y, state.b = x, y, b
    -- Get derivatives from the previous step
    local y_d, b_d = state.y_d or 0, state.b_d or 0

    -- Guard against NaNs in the y coordinate
    y0 = (y0 == y0) and y0 or b0
    y = (y == y) and y or b

    -- Horizontal linear movement of the curves
    local x_curve = {bezier.cubic_through_points(x0, x0+step_width)}

    -- Vertical movement curve for y
    local y_curve = {interpolate(y_d, y0, y)}
    state.y_d = bezier.curve_derivative_at_one(y_curve)
    if step_fraction then
        y_curve = bezier.curve_split_at(y_curve, step_fraction)
    end

    -- Paint the value curve
    cr:move_to(x_curve[1], y_curve[1])
    cr:curve_to(
        x_curve[2], y_curve[2],
        x_curve[3], y_curve[3],
        x_curve[4], y_curve[4]
    )

    if not draw_line then
        -- Vertical movement curve for the baseline
        local b_curve = {interpolate(b_d, b0, b)}
        state.b_d = bezier.curve_derivative_at_one(b_curve)
        if step_fraction then
            b_curve = bezier.curve_split_at(b_curve, step_fraction)
        end

        -- Paint the bar bounded by the baseline curve from below
        cr:line_to(x_curve[4], b_curve[4])
        cr:curve_to(
            x_curve[3], b_curve[3],
            x_curve[2], b_curve[2],
            x_curve[1], b_curve[1]
        )
        cr:close_path()
    end
end

--DOC_NEWLINE

local w1 = --DOC_HIDE
wibox.widget {
    stack        = true,
    group_colors = colors,
    step_width   = 15,
    step_hook    = curvaceous,
    scale        = true, --DOC_HIDE,
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w2 = --DOC_HIDE
wibox.widget {
    stack        = true,
    group_colors = colors,
    step_width   = 10,
    step_spacing = 5,
    step_hook    = curvaceous,
    group_start  = 0.5, -- Make vertical 1px-width lines sharper
    group_finish = function(cr)
        -- Paint the colored bars
        cr:fill_preserve()
        -- Paint a semi-black outline around bars
        cr:set_line_width(1)
        cr:set_source(gears.color("#0000007f"))
        cr:stroke()
    end,
    scale        = true, --DOC_HIDE,
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w3 = --DOC_HIDE
wibox.widget {
    stack        = false,
    group_colors = colors,
    step_width   = 15,
    step_hook    = function(cr, x, value_y, baseline_y, step_width, options)
        -- Draw the curve for the previous step
        curvaceous(cr, x, value_y, baseline_y, step_width, options, true)
        -- Draw a tick for this step
        if value_y == value_y then
            cr:move_to(x, value_y - 2)
            cr:line_to(x, value_y + 2)
        end
    end,
    group_start  = 0.5, -- Make vertical 1px-width lines sharper
    group_finish = function(cr)
        -- Paint all lines
        cr:set_line_width(1)
        cr:stroke()
    end,
    scale        = true, --DOC_HIDE,
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_HIDE We need one value more than usual for the best visual result
--DOC_HIDE due to the way interpolation is done
local function cdvn(...) --DOC_HIDE
    return wibox.widget.graph.compute_drawn_values_num(...) + 1 --DOC_HIDE
end --DOC_HIDE

w1.compute_drawn_values_num = cdvn --DOC_HIDE
w2.compute_drawn_values_num = cdvn --DOC_HIDE
w3.compute_drawn_values_num = cdvn --DOC_HIDE

for _, v in ipairs(data) do --DOC_HIDE
    for group, value in ipairs(v) do --DOC_HIDE
        w1:add_value(value, group) --DOC_HIDE
        w2:add_value(value, group) --DOC_HIDE
        w3:add_value(value, group) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

l:add(w1) --DOC_HIDE
l:add(w2) --DOC_HIDE
l:add(w3) --DOC_HIDE

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
