--DOC_HIDE_ALL
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local l = wibox.layout.fixed.horizontal()
l.spacing = 10
parent:add(l)

for _, v in ipairs {0, math.pi/2, math.pi} do
    l:add(wibox.widget {
          {
              text   = v,
              align  = "center",
              valign = "center",
              widget = wibox.widget.textbox,
          },
          colors = {
              beautiful.bg_normal,
              beautiful.bg_highlight,
              beautiful.border_color,
          },
          value        = 1,
          max_value    = 10,
          min_value    = 0,
          rounded_edge = false,
          bg           = "#ff000033",
          start_angle  = v,
          border_width = 0.5,
          border_color = "#000000",
          widget       = wibox.container.arcchart
      })
end

return nil, 60
