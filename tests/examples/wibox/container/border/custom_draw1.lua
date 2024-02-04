--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")

--DOC_HIDE_END

    local w = wibox.widget {
        {
            text   = "Center widget",
            valign = "center",
            align  = "center",
            widget = wibox.widget.textbox
        },
        after_draw_children = function(_, _, cr, width, height)
            cr:set_source_rgba(1,0,0,1)
            cr:set_dash({1,1},1)
            cr:rectangle(1, 1, width-2, height-2)
            cr:rectangle(5, 5, width-10, height-10)
            cr:stroke()
        end,
        borders       = 20,
        honor_borders = false,
        forced_width  = 100, --DOC_HIDE
        forced_height = 50, --DOC_HIDE
        widget        = wibox.container.border
    }

parent:add(w) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
