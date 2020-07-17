local awful = require("awful")
-- Widget and layout library
local wibox = require("wibox")


-- Create a textclock widget
local mysystray = wibox.widget.systray(),

table.insert(globalwidgets.right, mysystray)
