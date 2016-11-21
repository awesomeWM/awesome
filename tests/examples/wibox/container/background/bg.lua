local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local text_widget = {
    text   = "Hello world!",
    widget = wibox.widget.textbox
}

parent : setup {
    {
        text_widget,
        bg     = '#ff0000',
        widget = wibox.container.background
    },
    {
        text_widget,
        bg     = '#00ff00',
        widget = wibox.container.background
    },
    {
        text_widget,
        bg     = '#0000ff',
        widget = wibox.container.background
    },
    spacing = 10,
    layout  = wibox.layout.fixed.vertical
}
