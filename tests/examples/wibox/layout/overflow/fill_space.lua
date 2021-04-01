--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local first = generic_widget("first") --DOC_HIDE
local second = generic_widget("second") --DOC_HIDE
local third = generic_widget("third") --DOC_HIDE

local fill_space = {
        first,
        second,
        third,
        fill_space = true,
        layout = wibox.layout.overflow.vertical,
    }

local no_fill_space = {
        first,
        second,
        third,
        fill_space = false,
        layout = wibox.layout.overflow.vertical,
    }

local ret = wibox.widget{                          --DOC_HIDE
    {                                              --DOC_HIDE
        {                                          --DOC_HIDE
            markup = "<b>fill_space = false</b>",  --DOC_HIDE
            widget = wibox.widget.textbox          --DOC_HIDE
        },                                         --DOC_HIDE
        {                                          --DOC_HIDE
            no_fill_space,                         --DOC_HIDE
            border_width = 1,                      --DOC_HIDE
            widget = wibox.container.background,   --DOC_HIDE
        },                                         --DOC_HIDE
        layout = wibox.layout.fixed.vertical,      --DOC_HIDE
    },                                             --DOC_HIDE
    {                                              --DOC_HIDE
        {                                          --DOC_HIDE
            markup = "<b>fill_space = true</b>",   --DOC_HIDE
            widget = wibox.widget.textbox          --DOC_HIDE
        },                                         --DOC_HIDE
        {                                          --DOC_HIDE
            fill_space,                            --DOC_HIDE
            border_width = 1,                      --DOC_HIDE
            widget = wibox.container.background,   --DOC_HIDE
        },                                         --DOC_HIDE
        layout = wibox.layout.fixed.vertical,      --DOC_HIDE
    },                                             --DOC_HIDE
    spacing = 5,                                   --DOC_HIDE
    layout = wibox.layout.flex.horizontal,         --DOC_HIDE
}                                                  --DOC_HIDE

--DOC_HIDE The vertical size, in combination with the above layout is chosen
--DOC_HIDE specifically to force the `overflow` layouts to render a scrollbar.
--DOC_HIDE While not strictly necessary to demonstrate the bahvior of
--DOC_HIDE `fill_space`, it helps visualize that this is specific to `overflow`.
return ret, 300, 70 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
