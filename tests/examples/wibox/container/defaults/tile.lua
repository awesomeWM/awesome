--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local wibox     = require("wibox")

return {
    text   = "Before",
    halign = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        {
            text   = "After",
            halign = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        valign = "bottom",
        halign = "right",
        widget = wibox.container.tile
    },
    margins = 5,
    layout = wibox.container.margin
}
