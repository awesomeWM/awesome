--DOC_HIDE_ALL
local parent    = ...
local wibox     = require("wibox")

local function gen(val)
    return wibox.widget {
        {
            text   = "Value: "..val,
            align  = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        paddings           = 0,
        border_width       = val,
        value              = 0.75,
        max_value          = 1,
        min_value          = 0,
        widget             = wibox.container.radialprogressbar
    }
end

local l = wibox.layout {
    gen(1),gen(3),gen(5),
    forced_height = 30,
    forced_width  = 400,
    layout = wibox.layout.flex.horizontal
}

parent:add(l)
