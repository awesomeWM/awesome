--DOC_GEN_IMAGE --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local function generic_widget(text)            --DOC_HIDE
    return wibox.widget{                       --DOC_HIDE
        {                                      --DOC_HIDE
            {                                  --DOC_HIDE
                text = text,                   --DOC_HIDE
                widget = wibox.widget.textbox, --DOC_HIDE
            },                                 --DOC_HIDE
            margins = 3,                       --DOC_HIDE
            widget = wibox.container.margin,   --DOC_HIDE
        },                                     --DOC_HIDE
        bg = beautiful.bg_normal,              --DOC_HIDE
        border_color = beautiful.border_color, --DOC_HIDE
        border_width = 2,                      --DOC_HIDE
        widget = wibox.container.background,   --DOC_HIDE
    }                                          --DOC_HIDE
end                                            --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    {
        generic_widget("first widget"),
        generic_widget("second widget"),
        generic_widget("third widget"),
        generic_widget("fourth widget"),
        scrollbar_width = 3,
        spacing = 10,
        layout = wibox.layout.overflow.horizontal,
    },
    width = 30,
    strategy = "max",
    widget = wibox.container.constraint,
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
