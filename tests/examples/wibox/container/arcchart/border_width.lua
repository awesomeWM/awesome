--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local cols = {"#ff000022","#00ff0022","#0000ff22","#ff00ff22"}

local l = wibox.layout.fixed.horizontal()
l.spacing = 10
parent:add(l)

for _, v in ipairs {0,1,3,6.5} do
    l:add(wibox.widget {
          {
              {
                  {
                      text   = v,
                      align  = "center",
                      valign = "center",
                      widget = wibox.widget.textbox,
                  },
                  bg= "#ff000044",
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
              bg           = "#00ff0033",
              border_width = v,
              border_color = "#000000",
              widget       = wibox.container.arcchart
          },
          bg = cols[_],
          widget = wibox.container.background
      })
end

return nil, 60

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
