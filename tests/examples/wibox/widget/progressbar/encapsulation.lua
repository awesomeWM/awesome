local parent = ... --DOC_NO_USAGE --DOC_NO_DASH --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local color  = require("gears.color") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local grad1 = color{ type = "linear", from = { 0, 0 }, to = { 20, 0 }, --DOC_HIDE
    stops = { { 0, "#00ff00" }, { 1, "#00ffff" }}} --DOC_HIDE
local grad2 = color{ type = "linear", from = { 0, 0 }, to = { 20, 0 }, --DOC_HIDE
    stops = { { 0, "#ffff00" }, { 1, "#ff00ff" }}} --DOC_HIDE
local grad3 = color{ type = "linear", from = { 0, 0 }, to = { 20, 0 }, --DOC_HIDE
    stops = { { 0, "#ff0000" }, { 1, "#0000ff" }}} --DOC_HIDE

    -- The progressbars will be on top of each other
     local mycpubar1 = wibox.widget {
        { value  = 0.2, color = grad1,
            widget = wibox.widget.progressbar },
        { value  = 0.4, color = grad2,
            widget = wibox.widget.progressbar },
        { value  = 0.6, color = grad3,
            widget = wibox.widget.progressbar },
        layout = wibox.layout.flex.vertical,
     }

    -- Now, add a rotate container, the height will become the width.
    -- It act as if the `wibox.layout.flex.vertical` was
    -- `wibox.layout.flex.horizontal`
     local mycpubar2 = wibox.widget {
        {
            { value  = 0.2, color = grad1,
                widget = wibox.widget.progressbar },
            { value  = 0.4, color = grad2,
                widget = wibox.widget.progressbar },
            { value  = 0.6, color = grad3,
                widget = wibox.widget.progressbar },
            layout = wibox.layout.flex.vertical,
        },
        direction = 'east',
        widget    = wibox.container.rotate
     }

parent:add(wibox.widget { --DOC_HIDE
           { --DOC_HIDE
               { --DOC_HIDE
                   markup = "<b>mycpubar1</b>", --DOC_HIDE
                   widget = wibox.widget.textbox --DOC_HIDE
               }, --DOC_HIDE
               { --DOC_HIDE
                   { --DOC_HIDE
                       mycpubar1, --DOC_HIDE
                       margins = 4, --DOC_HIDE
                       layout = wibox.container.margin --DOC_HIDE
                   }, --DOC_HIDE
                   margins = 1, --DOC_HIDE
                   color  = beautiful.border_color, --DOC_HIDE
                   layout = wibox.container.margin, --DOC_HIDE
               }, --DOC_HIDE
               layout = wibox.layout.fixed.vertical --DOC_HIDE
           }, --DOC_HIDE
           { --DOC_HIDE
               { --DOC_HIDE
                   markup = "<b>mycpubar2</b>", --DOC_HIDE
                   widget = wibox.widget.textbox --DOC_HIDE
               }, --DOC_HIDE
               { --DOC_HIDE
                   { --DOC_HIDE
                       mycpubar2, --DOC_HIDE
                       margins = 4, --DOC_HIDE
                       layout = wibox.container.margin --DOC_HIDE
                   }, --DOC_HIDE
                   margins = 1, --DOC_HIDE
                   color  = beautiful.border_color, --DOC_HIDE
                   layout = wibox.container.margin, --DOC_HIDE
               }, --DOC_HIDE
               layout = wibox.layout.fixed.vertical --DOC_HIDE
           }, --DOC_HIDE
           layout = wibox.layout.flex.horizontal --DOC_HIDE
       }) --DOC_HIDE

return 500, 40 --DOC_HIDE


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
