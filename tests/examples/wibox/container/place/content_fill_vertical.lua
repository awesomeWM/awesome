--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require( "beautiful" )  --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_width  = 340, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
} --DOC_HIDE

for _, i in ipairs {true, false} do
    local w = wibox.widget {
        {
            {
                image         = beautiful.awesome_icon,
                forced_height = 30,
                forced_width  = 30,
                widget        = wibox.widget.imagebox
            },
            bg     = "#ff0000",
            widget = wibox.container.background
        },
        forced_height = 80, --DOC_HIDE
        forced_width = 120, --DOC_HIDE
        content_fill_vertical = i,
        widget = wibox.container.place
    }

    l:add(wibox.widget {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>`content_fill_vertical` = "..(i and "true" or "false").."</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    }) --DOC_HIDE
end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
