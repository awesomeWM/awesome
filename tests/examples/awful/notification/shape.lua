--DOC_GEN_IMAGE --DOC_NO_USAGE
require("_default_look") --DOC_HIDE
local gears   = {shape = require("gears.shape")} --DOC_HIDE
local awful   = {wibar = require("awful.wibar")} --DOC_HIDE
local naughty = require("naughty") --DOC_HIDE

screen[1]._resize {width = 640, height = 240} --DOC_HIDE

local some_wibar = awful.wibar {position = "bottom", height = 48, visible = true} --DOC_HIDE

--DOC_NEWLINE

   -- A notification popup using the default widget_template.
   naughty.connect_signal("request::display", function(n)
       naughty.layout.box {notification = n}
   end)

--DOC_NEWLINE

   -- Notifications as widgets for any `wibox`/`awful.wibar`/`awful.popup`
   some_wibar.widget = naughty.list.notifications {}

--DOC_NEWLINE

   local shapes = {
       gears.shape.octogon,
       gears.shape.rounded_rect,
       gears.shape.rounded_bar
   }

--DOC_NEWLINE

   for idx=1, 3 do
       naughty.notification {
           title        = "A notification",
           border_color = "#0000ff",
           border_width = idx*2,
           shape        = shapes[idx],
       }
   end

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
