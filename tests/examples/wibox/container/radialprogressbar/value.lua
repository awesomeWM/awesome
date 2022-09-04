--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require("wibox")

local function gen(val)
    return wibox.widget {
        {
            text   = "Value: "..(val*100).."%",
            halign = "center",
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

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
