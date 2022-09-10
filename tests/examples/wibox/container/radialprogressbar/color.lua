--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require("wibox")

local function gen(val)
    return wibox.widget {
        {
            text   = "Value: "..val,
            halign = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        color              = val,
        value              = 0.75,
        widget             = wibox.container.radialprogressbar
    }
end

local l = wibox.layout {
    gen("#ff0000"),gen("#00ff00"),gen("#0000ff"),
    forced_height = 30,
    forced_width  = 400,
    layout = wibox.layout.flex.horizontal
}

parent:add(l)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
