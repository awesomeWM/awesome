--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE
local gears = require("gears") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE
local s = screen[1] --DOC_HIDE

   awful.wallpaper {
       screen = s,
       widget = {
            image  = gears.surface.crop_surface {
                surface = gears.surface.load_uncached(beautiful.wallpaper),
                ratio = s.geometry.width/s.geometry.height,
            },
            widget = wibox.widget.imagebox,
        },
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE

