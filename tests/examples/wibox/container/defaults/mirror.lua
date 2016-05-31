--DOC_HIDE_ALL
local wibox     = require("wibox")

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
        widget = wibox.widget.textbox,
    },
    reflection = {horizontal = true},
    widget     = wibox.container.mirror
}
