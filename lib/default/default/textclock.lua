local wibox = require("wibox")

-- Create a textclock widget
local mytextclock = wibox.widget.textclock()

table.insert(globalwidgets.right, mytextclock)
