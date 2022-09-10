--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local l = wibox.layout.fixed.horizontal()
l.spacing = 10
parent:add(l)

for _, v in ipairs {0, 2, 4} do
    l:add(wibox.widget {
          {
              {
                  text   = v,
                  halign = "center",
                  valign = "center",
                  widget = wibox.widget.textbox,
              },
              bg= beautiful.bg_normal,
              widget = wibox.container.background,
          },
          colors = {
              beautiful.bg_normal,
              beautiful.bg_highlight,
              beautiful.border_color,
          },
          values = {
              1,
              2,
              3,
          },
          max_value    = 10,
          min_value    = 0,
          rounded_edge = false,
          bg           = "#ff000033",
          border_width = 0.5,
          border_color = "#000000",
          paddings     = v,
          widget       = wibox.container.arcchart
      })
end

l:add(wibox.widget {
      {
          {
              text   = 6,
              halign = "center",
              valign = "center",
              widget = wibox.widget.textbox,
          },
          bg= beautiful.bg_normal,
          widget = wibox.container.background,
      },
      colors = {
          beautiful.bg_normal,
          beautiful.bg_highlight,
          beautiful.border_color,
      },
      values = {
          1,
          2,
          3,
      },
      max_value    = 10,
      min_value    = 0,
      rounded_edge = false,
      bg           = "#ff000033",
      border_width = 0.5,
      border_color = "#000000",
      paddings     = {
          left   = 6,
          right  = 6,
          top    = 6,
          bottom = 6,
      },
      widget       = wibox.container.arcchart
  })

return nil, 60

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
