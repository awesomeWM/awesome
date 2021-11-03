--DOC_GEN_IMAGE  --DOC_HIDE_START --DOC_NO_USAGE
local parent = ...
local wibox  = require("wibox")

--DOC_HIDE_END
local bg = --DOC_HIDE
   wibox.widget {
       paddings = {
           top    = 4,
           bottom = 2,
           right  = 10,
           left   = 5
       },
       value            = 1,
       border_width     = 2,
       border_color     = "#00ff00",
       bar_border_wisth = 2,
       bar_border_color = "#ffff00",
       bor_color        = "#ff00ff",
       background       = "#0000ff",
       forced_width     = 75, --DOC_HIDE
       forced_height    = 30, --DOC_HIDE
       widget           = wibox.widget.progressbar,
   }
parent:add(bg) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
