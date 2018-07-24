--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local l = wibox.layout.fixed.horizontal()
parent:add(l)

for _, v in ipairs {0,1,3,5} do
    l:add(wibox.widget {
          data_list = {
              { 'L1', 100 },
              { 'L2', 200 },
              { 'L3', 300 },
          },
          border_width = v,
          forced_height = 50,
          forced_width  = 100,
          colors = {
              beautiful.bg_normal,
              beautiful.bg_highlight,
              beautiful.border_color,
          },
          widget = wibox.widget.piechart
      })
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
