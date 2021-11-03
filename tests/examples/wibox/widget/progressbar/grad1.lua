--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent:add( --DOC_HIDE

    wibox.widget {
        color = {
            type  = "linear",
            from  = { 0  , 0 },
            to    = { 100, 0 },
            stops = {
                { 0  , "#0000ff" },
                { 0.8, "#0000ff" },
                { 1  , "#ff0000" }
            }
        },
        max_value     = 1,
        value         = 1,
        forced_height = 20,
        forced_width  = 100,
        paddings      = 1,
        border_width  = 1,
        border_color  = beautiful.border_color,
        widget        = wibox.widget.progressbar,
    }

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
