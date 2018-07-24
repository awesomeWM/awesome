--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local l = wibox.layout.fixed.horizontal()
l.spacing = 10
parent:add(l)

for _, v in ipairs {0,1,7,99} do
    l:add(wibox.widget {
          colors = {
              beautiful.bg_normal,
              beautiful.bg_highlight,
              beautiful.border_color,
          },
          value = v,
          max_value    = 10,
          min_value    = 0,
          rounded_edge = false,
          bg           = "#ff000033",
          border_width = 0.5,
          border_color = "#000000",
          widget       = wibox.container.arcchart
      })
end

return nil, 60

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
