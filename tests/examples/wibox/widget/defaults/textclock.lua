--DOC_GEN_IMAGE
--DOC_NO_USAGE
local parent = ... --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

    local my_textclock = wibox.widget.textclock('%a %b %d, %H:%M')

parent:add(my_textclock) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
