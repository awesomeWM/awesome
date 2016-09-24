--DOC_HIDE_ALL
local parent    = ...
local wibox     = require("wibox")

local function gen(val)
    return wibox.widget {
        {
            text   = "Value: "..(val*100).."%",
            align  = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        padding            = 5,
        value              = val,
        max_value          = 1,
        min_value          = 0,
        widget             = wibox.container.radialprogressbar
    }
end

local l = wibox.layout {
    gen(0.0),gen(0.3),gen(1.0),
    forced_height = 30,
    forced_width  = 400,
    layout = wibox.layout.flex.horizontal
}

parent:add(l)
