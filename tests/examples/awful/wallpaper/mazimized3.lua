--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       widget = {
            horizontal_fit_policy = "fit",
            vertical_fit_policy   = "fit",
            image                 = beautiful.wallpaper,
            widget                = wibox.widget.imagebox,
        },
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE

