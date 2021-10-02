--DOC_GEN_IMAGE --DOC_GEN_OUTPUT
--DOC_HIDE_START
local awful =  { wallpaper = require("awful.wallpaper") }
local wibox = require("wibox")
local gears = {color = require("gears.color") }

screen._track_workarea = true
screen[1]._resize {x = 0, y = 0, width = 320, height = 196}
require("_default_look")
--DOC_HIDE_END

   -- Add some padding to the first screen.
   screen[1].padding = {
       left  = 30,
       right = 10,
   }

   --DOC_NEWLINE

   local wall = awful.wallpaper {
       screen                = screen[1],
       honor_workarea        = true,
       honor_padding         = true,
       bg                    = "#222222",
       uncovered_areas_color = "#ff0000",
       widget =  wibox.widget {
           fit = function(_, width, height)
               return width, height
           end,
           draw = function(_, _, cr, width, height)
               local radius = math.min(width, height)/2
               cr:arc(width/2, height/2, radius, 0, 2 * math.pi)
               cr:set_source(gears.color {
                   type = "radial",
                   from  = { width/2, radius, 20  },
                   to    = { width/2, radius, 120 },
                   stops = {
                       { 0, "#0000ff" },
                       { 1, "#ff0000" },
                       { 1, "#000000" },
                   }
               })
               cr:fill()
           end,
           widget = wibox.widget.base.make_widget()
       }
   }

--DOC_HIDE_START
require("gears.timer").run_delayed_calls_now()
--DOC_HIDE_END

--DOC_NEWLINE

   -- Areas due to the padding and the wibar (workarea).
   for _, area in ipairs(wall.uncovered_areas) do
      print("Uncovered area:", area.x, area.y, area.width, area.height)
   end
