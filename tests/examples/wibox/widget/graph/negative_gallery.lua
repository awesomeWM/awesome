--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = { --DOC_HIDE
    matrix = require("gears.matrix"), --DOC_HIDE
    shape = require("gears.shape"), --DOC_HIDE
} --DOC_HIDE

local data = { --DOC_HIDE
    -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, --DOC_HIDE
} --DOC_HIDE

-- For converting normally "horizontal" shapes into vertical ones
local function transpose(shape)
    local swap_coords = gears.matrix.create(0, 1, 1, 0, 0, 0)
    return function(cr, width, height, ...)
        gears.shape.transform(shape):multiply(swap_coords) (cr, height, width, ...)
    end
end

local shapes = {
    gears.shape.rectangle,
-- DOC_HIDE rectangular_tag illustrates the mode differences the best
    transpose(gears.shape.rectangular_tag),
    gears.shape.squircle,
-- DOC_HIDE behaves badly: gears.shape.star,
-- DOC_HIDE looks without params like rounded_bar: gears.shape.rounded_rect,
    gears.shape.rounded_bar,
    gears.shape.arrow,
    transpose(gears.shape.hexagon),
    transpose(gears.shape.powerline),
    gears.shape.isosceles_triangle,
    gears.shape.cross,
    gears.shape.octogon,
-- DOC_HIDE centers itself in the bar: gears.shape.circle,
    gears.shape.losange,
}

-- When asked to draw the shape with negative height,
-- draw its counterpart with positive height and mirror it.
local function mirror_negative(shape)
    local mirrored_shape = gears.shape.transform(shape):scale(1, -1)
    return function(cr, width, height, ...)
        if height < 0 then
           mirrored_shape(cr, width, -height, ...)
        else
           shape(cr, width, height, ...)
        end
    end
end

-- When asked to draw the shape with negative height,
-- draw its counterpart with positive height and shift it down.
local function flip_negative(shape)
    return function(cr, width, height, ...)
        if height < 0 then
           gears.shape.transform(shape):translate(0, height) (cr, width, -height, ...)
        else
           shape(cr, width, height, ...)
        end
    end
end

for _, shape in ipairs(shapes) do

    local w_normal = --DOC_HIDE
    wibox.widget {
        step_width    = 9,
        step_spacing  = 1,
        step_shape    = shape,
        scale         = true, --DOC_HIDE
        border_width  = 2, --DOC_HIDE
        color         = "#00ff00", --DOC_HIDE
        background_color = "#000000", --DOC_HIDE
        margins       = 5, --DOC_HIDE
        forced_height = 104, --DOC_HIDE
        widget        = wibox.widget.graph,
    }

    --DOC_NEWLINE

    local w_mirror = --DOC_HIDE
    wibox.widget {
        step_width    = 9,
        step_spacing  = 1,
        step_shape    = mirror_negative(shape),
        scale         = true, --DOC_HIDE
        border_width  = 2, --DOC_HIDE
        color         = "#00ff00", --DOC_HIDE
        background_color = "#000000", --DOC_HIDE
        margins       = 5, --DOC_HIDE
        forced_height = 104, --DOC_HIDE
        widget        = wibox.widget.graph,
    }

    --DOC_NEWLINE

    local w_flip = --DOC_HIDE
    wibox.widget {
        step_width    = 9,
        step_spacing  = 1,
        step_shape    = flip_negative(shape),
        scale         = true, --DOC_HIDE
        border_width  = 2, --DOC_HIDE
        color         = "#00ff00", --DOC_HIDE
        background_color = "#000000", --DOC_HIDE
        margins       = 5, --DOC_HIDE
        forced_height = 104, --DOC_HIDE
        widget        = wibox.widget.graph,
    }

    --DOC_NEWLINE

    for _, v in ipairs(data) do --DOC_HIDE
        w_normal:add_value(v) --DOC_HIDE
        w_mirror:add_value(v) --DOC_HIDE
        w_flip:add_value(v) --DOC_HIDE
    end --DOC_HIDE

    parent:add(wibox.layout {--DOC_HIDE
        {--DOC_HIDE
            {--DOC_HIDE
                markup = "<b>shape</b>",--DOC_HIDE
                widget = wibox.widget.textbox,--DOC_HIDE
            },--DOC_HIDE
            w_normal,--DOC_HIDE
            layout = wibox.layout.fixed.vertical,--DOC_HIDE
        },--DOC_HIDE
        {--DOC_HIDE
            {--DOC_HIDE
                markup = "<b>mirror(shape)</b>",--DOC_HIDE
                widget = wibox.widget.textbox,--DOC_HIDE
            },--DOC_HIDE
            w_mirror,--DOC_HIDE
            layout = wibox.layout.fixed.vertical,--DOC_HIDE
        },--DOC_HIDE
        {--DOC_HIDE
            {--DOC_HIDE
                markup = "<b>flip(shape)</b>",--DOC_HIDE
                widget = wibox.widget.textbox,--DOC_HIDE
            },--DOC_HIDE
            w_flip,--DOC_HIDE
            layout = wibox.layout.fixed.vertical,--DOC_HIDE
        },--DOC_HIDE

        forced_width  = 342, --DOC_HIDE
        spacing       = 5, --DOC_HIDE
        layout        = wibox.layout.flex.horizontal --DOC_HIDE
    }) --DOC_HIDE
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
