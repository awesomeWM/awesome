--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local beautiful = require( "beautiful" )
local gears = { color = require("gears.color")}

local red_logo    = beautiful._logo(nil, gears.color("#ff0000"))
local green_logo  = beautiful._logo(nil, gears.color("#00ff00"))
local blue_logo   = beautiful._logo(nil, gears.color("#0000ff"))
local yellow_logo = beautiful._logo(nil, gears.color("#ffff00"))
local orange_logo = beautiful._logo(nil, gears.color("#ffbb00"))
local purple_logo = beautiful._logo(nil, gears.color("#ff00ff"))
local cyan_logo   = beautiful._logo(nil, gears.color("#00ffff"))
local black_logo  = beautiful._logo(nil, gears.color("#000000"))

--DOC_HIDE_END

local w1 = wibox.widget {
    {
        text          = "Single image",
        valign        = "center",
        align         = "center",
        widget        = wibox.widget.textbox
    },
    borders          = 20,
    sides_fit_policy = "repeat",
    border_images    = blue_logo,
    widget           = wibox.container.border
}

--DOC_NEWLINE

local w2 = wibox.widget {
    {
        text          = "Multiple images",
        valign        = "center",
        align         = "center",
        widget        = wibox.widget.textbox
    },
    sides_fit_policy = "repeat",
    border_images    = {
        top_left     = red_logo,
        top          = green_logo,
        top_right    = blue_logo,
        right        = yellow_logo,
        bottom_right = orange_logo,
        bottom       = purple_logo,
        bottom_left  = cyan_logo,
        left         = black_logo,
    },
    widget = wibox.container.border
}

--DOC_HIDE_START

parent.spacing = 20
parent:add(w1)
parent:add(w2)
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
