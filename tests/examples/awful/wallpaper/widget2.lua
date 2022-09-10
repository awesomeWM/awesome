--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful =  { wallpaper = require("awful.wallpaper") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

math.randomseed(1) --DOC_HIDE

screen[1]._resize {x = 0, y = 0, width = 320, height = 196} --DOC_HIDE

   local function binary()
       local ret = {}

       for _=1, 15 do
           for _=1, 57 do
               table.insert(ret, math.random() > 0.5 and 1 or 0)
           end
           table.insert(ret, "\n")
       end

       return table.concat(ret)
   end

   --DOC_NEWLINE

   awful.wallpaper {
       screen = screen[1], --DOC_HIDE
       bg     = "#000000",
       fg     = "#55ff5577",
       widget = wibox.widget {
           {
               {
                   markup = "<tt><b>[SYSTEM FAILURE]</b></tt>",
                   valign = "center",
                   halign = "center",
                   widget = wibox.widget.textbox
               },
               fg = "#00ff00",
               widget = wibox.container.background
           },
           {
               wrap   = "word",
               text   = binary(),
               widget = wibox.widget.textbox,
           },
           widget = wibox.layout.stack
       },
   }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
