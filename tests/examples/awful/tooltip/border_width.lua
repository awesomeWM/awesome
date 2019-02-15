--DOC_GEN_IMAGE
--DOC_NO_USAGE
--DOC_HIDE_ALL
screen[1]._resize {width = 640, height = 55}
screen.no_outline = true
local awful = {tooltip = require("awful.tooltip")}
local beautiful = require("beautiful")

-- mouse.coords{x=50, y= 10}

require("gears.delayed_call").run_now()

local x_offset = 0

for _, width in ipairs{ 1,2,4,6 } do
    local tt = awful.tooltip {
        text = "width: "..width,
        mode = "mouse",
        border_width = width,
        border_color = beautiful.border_color,
    }
    tt.bg = beautiful.bg_normal
    require("gears.delayed_call").run_now()
    tt:show()
    require("gears.delayed_call").run_now()
    tt.wibox.x = x_offset
    x_offset = x_offset + 640/5
end

mouse.coords{x=125, y= 0}
-- mouse.push_history()
require("gears.delayed_call").run_now()
