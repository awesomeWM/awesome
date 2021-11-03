--DOC_GEN_IMAGE  --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")
local gears = { shape = require("gears.shape") }
local beautiful = require("beautiful")

local l1 = wibox.layout.fixed.horizontal()
local l2 = wibox.layout.fixed.horizontal()
local l3 = wibox.layout.fixed.horizontal()
l1.spacing = 5
l2.spacing = 5
l3.spacing = 5

--DOC_HIDE_END
   for _, size in ipairs { 0, 2, 4, 6 } do

       -- Plane shapes.
       local pb = --DOC_HIDE
       wibox.widget {
           value        = 1,
           border_width = 2,
           ticks        = true,
           ticks_size   = size,
           ticks_gap    = 3,
           paddings     = 2,
           bar_shape    = gears.shape.rounded_bar,
           widget       = wibox.widget.progressbar,
       }

       --DOC_NEWLINE

       -- With a border for each shape.
       local pb3 = --DOC_HIDE
       wibox.widget {
           value            = 1,
           border_width     = 2,
           ticks            = true,
           ticks_size       = size,
           ticks_gap        = 3,
           paddings         = 2,
           bar_shape        = gears.shape.rounded_bar,
           bor_border_width = 2,
           bar_border_color = beautiful.border_color,
           widget           = wibox.widget.progressbar,
       }

       --DOC_NEWLINE

       -- With a gradient.
       local pb2 = --DOC_HIDE
       wibox.widget {
           color = {
               type  = "linear",
               from  = { 0 , 0 },
               to    = { 65, 0 },
               stops = {
                   { 0   , "#0000ff" },
                   { 0.75, "#0000ff" },
                   { 1   , "#ff0000" }
               }
           },
           paddings     = 2,
           value        = 1,
           border_width = 2,
           ticks        = true,
           ticks_size   = size,
           ticks_gap    = 3,
           bar_shape    = gears.shape.rounded_bar,
           widget       = wibox.widget.progressbar,
       }

       --DOC_HIDE_START
       l1:add(wibox.widget {
           pb,
           {
               text   = size,
               align  = "center",
               widget = wibox.widget.textbox,
           },
           forced_height = 30,
           forced_width  = 75,
           layout = wibox.layout.stack
       })
       l2:add(wibox.widget {
           pb3,
           {
               text   = size,
               align  = "center",
               widget = wibox.widget.textbox,
           },
           forced_height = 30,
           forced_width  = 75,
           layout = wibox.layout.stack
       })

       l3:add(wibox.widget {
           pb2,
           {
               {
                   {
                       {
                           text   = size,
                           align  = "center",
                           widget = wibox.widget.textbox,
                       },
                       margins = 4,
                       widget = wibox.container.margin,
                   },
                   bg = "#ffffff",
                   shape = gears.shape.circle,
                   widget = wibox.container.background,
               },
               align = "center",
               valign = "center",
               widget = wibox.container.place,
           },
           forced_height = 30,
           forced_width  = 75,
           layout = wibox.layout.stack
       })

       --DOC_HIDE_END
   end

parent:add(l1) --DOC_HIDE
parent:add(l2) --DOC_HIDE
parent:add(l3) --DOC_HIDE
parent.spacing = 10 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
