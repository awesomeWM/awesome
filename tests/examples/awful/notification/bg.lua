--DOC_GEN_IMAGE --DOC_NO_USAGE
require("_default_look") --DOC_HIDE
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

   for _, color in ipairs {"#ff0000", "#00ff00", "#0000ff"} do
       naughty.notification {
           title = "A ".. color .." notification",
           bg    = color,
       }
   end

require("gears.timer").run_delayed_calls_now()
