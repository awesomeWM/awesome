--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE
local s = screen[1] --DOC_HIDE

   awful.wallpaper {
       screen = s,
       bg     = "#0000ff",
       widget = {
           {
               shape         = gears.shape.star,
               forced_width  = 30,
               forced_height = 30,
               widget        = wibox.widget.separator,
           },
           horizontal_spacing = 5,
           vertical_spacing   = 5,
           vertical_crop      = true,
           horizontal_crop    = true,
           valign             = "center",
           halign             = "center",
           tiled              = true,
           widget             = wibox.container.tile,
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
