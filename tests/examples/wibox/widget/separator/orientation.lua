--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE_ALL
local wibox  = require("wibox")

parent:add(
wibox.widget {
    {
        {
            markup = "<b>orientation</b> = <i>horizontal</i>",
            widget = wibox.widget.textbox
        },
        {
            orientation = "horizontal",
            widget = wibox.widget.separator,
        },
        forced_width = 200,
        layout = wibox.layout.fixed.vertical
    },
    {
        {
            markup = "<b>orientation</b> = <i>vertical</i>",
            widget = wibox.widget.textbox
        },
        {
            orientation = "vertical",
            widget = wibox.widget.separator,
        },
        forced_width = 200,
        layout = wibox.layout.fixed.vertical
    },
    forced_height = 30,
    forced_width = 400,
    layout = wibox.layout.fixed.horizontal
}
)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
