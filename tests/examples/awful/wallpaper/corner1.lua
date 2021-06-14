--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { --DOC_HIDE
    wallpaper = require("awful.wallpaper"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       bg     = "#000000",
       widget = {
           {
               {
                   image  = beautiful.awesome_icon,
                   resize = false,
                   point  = awful.placement.bottom_right,
                   widget = wibox.widget.imagebox,
               },
               widget = wibox.layout.manual,
           },
           margins = 5,
           widget  = wibox.container.margin
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
