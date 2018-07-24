--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {shape = require("gears.shape") } --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE


local l = wibox.layout { --DOC_HIDE
    forced_height = 120, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    layout = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

for _, shape in ipairs {"rounded_bar", "octogon", "hexagon", "powerline" } do
    l:add(wibox.widget {
          value            = 0.33,
          bar_shape        = gears.shape[shape],
          bar_border_color = beautiful.border_color,
          bar_border_width = 1,
          border_width     = 2,
          border_color     = beautiful.border_color,
          margins          = 5, --DOC_HIDE
          paddings         = 1,
          widget           = wibox.widget.progressbar,
      })
end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
