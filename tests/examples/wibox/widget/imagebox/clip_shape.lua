--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require( "beautiful" )  --DOC_HIDE
local gears = {shape=require("gears.shape")} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_width  = 380, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

local names = {"circle", "squircle", "rounded_rect"} --DOC_HIDE

for _, resize in ipairs {true, false} do
    local row = wibox.layout { --DOC_HIDE
        spacing = 5, --DOC_HIDE
        layout = wibox.layout.fixed.horizontal --DOC_HIDE
    } --DOC_HIDE

    row:add(wibox.widget { --DOC_HIDE
        markup = "<b>resize = "..(resize and "true" or "false").."</b>", --DOC_HIDE
        forced_width = 80, --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }) --DOC_HIDE

    for idx, shape in ipairs {gears.shape.circle, gears.shape.squircle, gears.shape.rounded_rect} do
        local w = wibox.widget {
            {
                {
                    image         = beautiful.awesome_icon,
                    forced_height = 32,
                    forced_width  = 32,
                    clip_shape    = shape,
                    resize        = resize,
                    widget        = wibox.widget.imagebox
                },
                widget = wibox.container.place
            },
            forced_height = 64, --DOC_HIDE
            forced_width = 64, --DOC_HIDE
            widget = wibox.container.background
        }


        row:add(wibox.widget {--DOC_HIDE
            {--DOC_HIDE
                markup = "<b>`shape` = "..names[idx].."</b>",--DOC_HIDE
                widget = wibox.widget.textbox,--DOC_HIDE
            },--DOC_HIDE
            w,--DOC_HIDE
            layout = wibox.layout.fixed.vertical,--DOC_HIDE
        }) --DOC_HIDE
    end

    l:add(row) --DOC_HIDE
end


parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
