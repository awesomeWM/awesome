--DOC_NO_USAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local gears = {timer = require("gears.timer"), filesystem = require("gears.filesystem")}--DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE

   -- The "request::wallpaper" section is already in the default
   -- `rc.lua`, replace it with this.
   screen.connect_signal("request::wallpaper", function(s)
       awful.wallpaper {
           screen = s,
           bg     = "#0000ff",
           widget = {
               {
                   image  = gears.filesystem.get_random_file_from_dir(
                       "path/to/dir",
                       {".jpg", ".png", ".svg"},
                       true
                   ),
                   resize = true,
                   widget = wibox.widget.imagebox,
               },
               valign = "center",
               halign = "center",
               tiled  = false,
               widget = wibox.container.tile,
           }
       }
   end)

   --DOC_NEWLINE

   -- **Somewhere else** in the code, **not** in the `request::wallpaper` handler.
   gears.timer {
       timeout   = 1800,
       autostart = true,
       callback  = function()
           for s in screen do
               s:emit_signal("request::wallpaper")
           end
       end,
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
