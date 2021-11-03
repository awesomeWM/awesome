--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local l = wibox.layout.fixed.vertical() --DOC_HIDE
l.spacing = 5 --DOC_HIDE

    for _, value in ipairs { 0.3, 0.5, 0.7, 1 } do
        l:add( --DOC_HIDE
        wibox.widget {
            color = {
                type  = "linear",
                from  = { 0  , 0 },
                to    = { 100, 0 },
                stops = {
                    { 0  , "#00ff00" },
                    { 0.5, "#00ff00" },
                    { 0.5, "#ffff00" },
                    { 0.7, "#ffff00" },
                    { 0.7, "#ffaa00" },
                    { 0.8, "#ffaa00" },
                    { 0.8, "#ff0000" },
                    { 1  , "#ff0000" }
                }
            },
            max_value     = 1,
            value         = value,
            forced_height = 20,
            forced_width  = 100,
            paddings      = 1,
            border_width  = 1,
            border_color  = beautiful.border_color,
            widget        = wibox.widget.progressbar,
        }
        ) --DOC_HIDE
    end

parent:add(l) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
