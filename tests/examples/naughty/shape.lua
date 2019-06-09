--DOC_GEN_IMAGE  --DOC_NO_USAGE

local beautiful = require("beautiful") --DOC_HIDE
local gears     = {shape=require("gears.shape")} --DOC_HIDE
local naughty   = require("naughty") --DOC_HIDE

   local text = [[An <b>important</b>
   <i>notification</i>
   ]]

--DOC_NEWLINE

   local shapes = {
       gears.shape.rounded_rect,
       gears.shape.hexagon,
       gears.shape.octogon,
       function(cr, w, h)
           return gears.shape.infobubble(cr, w, h, 20, 10, w/2 - 10)
       end
   }

--DOC_NEWLINE

   for _, s in ipairs(shapes) do
       naughty.notify {
           title        = "Hello world!",
           text         = text,
           icon         = beautiful.icon,
           shape        = s,
           border_width = 3,
           border_color = beautiful.bg_highlight,
           margin       = 15,
       }
   end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
