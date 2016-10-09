local generic_widget = ... --DOC_HIDE
local wibox          = require("wibox") --DOC_HIDE
local beautiful      = require("beautiful") --DOC_HIDE

return --DOC_HIDE
wibox.widget {
{ --DOC_HIDE
    wibox.widget.textbox( "     first  "  ),
    wibox.widget.textbox( "     second  " ),
    wibox.widget.textbox( "     third  "  ),
    border_width = 2,
    border_color = beautiful.border_color,
    bgs          = {
        beautiful.bg_normal,
        beautiful.bg_alternate,
    },
    layout = wibox.layout.shaped
}, --DOC_HIDE
    margins = 5, --DOC_HIDE
    widget  = wibox.container.margin, --DOC_HIDE
}
