
local beautiful = require("beautiful") --DOC_HIDE

local text = [[An <b>important</b>
<i>notification</i>
]]

require("naughty").notify {
    title        = "Hello world!",
    text         = text,
    icon         = beautiful.icon,
    bg           = "#0000ff",
    fg           = "#ff0000",
    fond         = "verdana 14",
    border_width = 1,
    border_color = "#ff0000"
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
