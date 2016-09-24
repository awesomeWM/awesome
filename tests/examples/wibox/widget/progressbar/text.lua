local parent = ... --DOC_NO_USAGE --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent:add( --DOC_HIDE

    wibox.widget {
        {
            max_value     = 1,
            value         = 0.5,
            forced_height = 20,
            forced_width  = 100,
            paddings      = 1,
            border_width  = 1,
            border_color  = beautiful.border_color,
            widget        = wibox.widget.progressbar,
        },
        {
            text   = "50%",
            widget = wibox.widget.textbox,
        },
        layout = wibox.layout.stack
    }

) --DOC_HIDE
