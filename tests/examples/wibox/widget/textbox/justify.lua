--DOC_GEN_IMAGE --DOC_HIDE

--DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local beautiful = require("beautiful")

local widget = function(inner)
    return wibox.widget {
        {
            inner,
            margins = 5,
            widget = wibox.container.margin,
        },
        border_width = 1,
        border_color = beautiful.border_color,
        widget = wibox.container.background,
    }
end

local l = wibox.layout.flex.horizontal()

--DOC_HIDE_END

local lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed "..
    "do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad "..
    "minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex "..
    "ea commodo consequat."
--DOC_NEWLINE

l:add( --DOC_HIDE
widget{
    text    = lorem_ipsum,
    justify = false,
    widget  = wibox.widget.textbox,
}
) --DOC_HIDE
--DOC_NEWLINE
l:add( --DOC_HIDE
widget{
    text    = lorem_ipsum,
    justify = true,
    widget  = wibox.widget.textbox,
}
) --DOC_HIDE
--DOC_HIDE_START

parent:add(l)

return 320

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
