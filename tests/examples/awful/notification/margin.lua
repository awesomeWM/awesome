--DOC_GEN_IMAGE --DOC_NO_USAGE
require("_default_look") --DOC_HIDE
local awful   = {wibar = require("awful.wibar")} --DOC_HIDE
local wibox   = require("wibox") --DOC_HIDE
local naughty = require("naughty") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {width = 640, height = 240} --DOC_HIDE

local some_wibar = awful.wibar {position = "bottom", height = 48, visible = true} --DOC_HIDE

--DOC_NEWLINE

   -- A notification popup using the default widget_template.
   naughty.connect_signal("request::display", function(n)
       naughty.layout.box {notification = n}
   end)

--DOC_NEWLINE

   -- Notifications as widgets for any `wibox`/`awful.wibar`/`awful.popup`
   some_wibar.widget = naughty.list.notifications {
       base_layout = wibox.widget {
            spacing = beautiful.notification_spacing,
            layout  = wibox.layout.fixed.horizontal
       },
   }

--DOC_NEWLINE

   for margin = 10, 20, 5 do
       naughty.notification {
           title        = "A notification",
           margin       = margin,
           border_width = 1,
           border_color = "#ff0000",
       }
   end

require("gears.timer").run_delayed_calls_now()
