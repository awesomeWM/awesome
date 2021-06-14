--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       bg     = {
           type = "radial",
           from  = { 160, 98, 20  },
           to    = { 160, 98, 120 },
           stops = {
               { 0  , "#ff0000" },
               { 0.5, "#00ff00" },
               { 1  , "#0000ff" },
           }
       }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
