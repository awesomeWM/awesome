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
        border_color = val,
        value        = 0.75,
        widget       = wibox.container.radialprogressbar
    }
end

local l = wibox.layout {
    gen("#ff0000"),gen("#00ff00"),gen("#0000ff"),
    forced_height = 30,
    forced_width  = 400,
    layout = wibox.layout.flex.horizontal
}

parent:add(l)
