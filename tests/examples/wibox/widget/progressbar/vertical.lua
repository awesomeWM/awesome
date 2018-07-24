--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

parent:add( --DOC_HIDE

    wibox.widget {
        {
            max_value     = 1,
            value         = 0.33,
            widget        = wibox.widget.progressbar,
        },
        forced_height = 100,
        forced_width  = 20,
        direction     = "east",
        layout        = wibox.container.rotate,
    }

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
