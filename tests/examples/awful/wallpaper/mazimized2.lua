--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       bg     = "#0000ff",
       widget = {
           {
               image  = beautiful.wallpaper,
               resize = true,
               widget = wibox.widget.imagebox,
           },
           valign = "center",
           halign = "center",
           tiled  = false,
           widget = wibox.container.tile,
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
