--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE


   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       bg     = {
           type  = "linear" ,
           from  = { 0, 0  },
           to    = { 0, 240 },
           stops = {
               { 0, "#0000ff" },
               { 1, "#ff0000" }
           }
        }
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
