--DOC_GEN_IMAGE --DOC_NO_USAGE
--DOC_HIDE_START
local awful     = require("awful")
local wibox     = require("wibox")

screen[1]._resize {width = 480, height = 196}
--DOC_HIDE_END

   local colors = {
       top    = "#ffff00",
       bottom = "#ff00ff",
       left   = "#00ffff",
       right  = "#ffcc00"
   }

   --DOC_NEWLINE

   for _, position in ipairs { "top", "bottom", "left", "right" } do
       awful.wibar {
           position = position,
           bg       = colors[position],
           widget   = {
               {
                   text   = position,
                   halign = "center",
                   widget = wibox.widget.textbox
               },
               direction = (position == "left" or position == "right") and
                   "east" or "north",
               widget    = wibox.container.rotate
           },
       }
   end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

