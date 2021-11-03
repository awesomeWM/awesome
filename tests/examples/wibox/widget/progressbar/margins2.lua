--DOC_GEN_IMAGE  --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")

--DOC_HIDE_END
local bg = --DOC_HIDE
   wibox.widget {
       {
           margins = {
               top    = 4,
               bottom = 2,
               right  = 10,
               left   = 5
           },
           value        = 0.33,
           border_width = 2,
           border_color = "#00ff00",
           background   = "#0000ff",
           widget       = wibox.widget.progressbar,
       },
       forced_height = 30, --DOC_HIDE
       forced_width = 75, --DOC_hIDE
       bg     = "#ff0000",
       widget = wibox.container.background
   }

parent:add(bg) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
