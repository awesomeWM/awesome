--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE
local s = screen[1] --DOC_HIDE

   awful.wallpaper {
       screen = s,
       bg     = "#0000ff",
       widget = {
           {
               image  = beautiful.awesome_icon,
               resize = false,
               widget = wibox.widget.imagebox,
           },
           horizontal_spacing = 5,
           vertical_spacing   = 5,
           valign             = "top",
           halign             = "left",
           tiled              = true,
           widget             = wibox.container.tile,
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
