--DOC_HIDE_ALL
local wibox     = require("wibox")

-- There is literally nothing to show, but the example must exist for the
-- indexing.

return {
    text   = "Before",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        text   = "After",
        align  = "center",
        valign = "center",
        when   = function() return true end,
        widget = wibox.widget.textbox,
    },
    widget             = wibox.container.conditional
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
