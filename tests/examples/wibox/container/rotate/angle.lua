--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local function create_arrow(text)                           --DOC_HIDE
    return {                                                --DOC_HIDE
        {                                                   --DOC_HIDE
            {                                               --DOC_HIDE
                text   = text,                              --DOC_HIDE
                align  = "center",                          --DOC_HIDE
                valign = "center",                          --DOC_HIDE
                widget = wibox.widget.textbox,              --DOC_HIDE
            },                                              --DOC_HIDE
            shape              = gears.shape.arrow,         --DOC_HIDE
            bg                 = beautiful.bg_normal,       --DOC_HIDE
            shape_border_color = beautiful.border_color,    --DOC_HIDE
            shape_border_width = beautiful.border_width,    --DOC_HIDE
            widget             = wibox.container.background --DOC_HIDE
        },                                                  --DOC_HIDE
        strategy = 'exact',                                 --DOC_HIDE
        width    = 70,                                      --DOC_HIDE
        height   = 70,                                      --DOC_HIDE
        widget   = wibox.container.constraint               --DOC_HIDE
    }                                                       --DOC_HIDE
end                                                         --DOC_HIDE

local normal = create_arrow("Normal")

local north  = wibox.container {
    create_arrow("North"),
    direction = "north",
    widget    = wibox.container.rotate
}

local south  = wibox.container {
    create_arrow("South"),
    direction = "south",
    widget    = wibox.container.rotate
}

local east  = wibox.container {
    create_arrow("East"),
    direction = "east",
    widget    = wibox.container.rotate
}

local west  = wibox.container {
    create_arrow("West"),
    direction = "west",
    widget    = wibox.container.rotate
}

parent : setup {                            --DOC_HIDE
    normal,                                 --DOC_HIDE
    north,                                  --DOC_HIDE
    south,                                  --DOC_HIDE
    east,                                   --DOC_HIDE
    west,                                   --DOC_HIDE
    spacing = 10,                           --DOC_HIDE
    layout  = wibox.layout.fixed.horizontal --DOC_HIDE
}                                           --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
