--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE
local s = screen[1] --DOC_HIDE


   awful.wallpaper {
       screen = s,
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

