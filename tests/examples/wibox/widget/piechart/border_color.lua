--DOC_HIDE_ALL
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )

local l = wibox.layout.fixed.horizontal()
parent:add(l)

for _, v in ipairs {"#ff0000", "#00ff00", "#0000ff"} do
    l:add(wibox.widget {
        data_list = {
            { 'L1', 100 },
            { 'L2', 200 },
            { 'L3', 300 },
        },
        border_width = 1,
        border_color = v,
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
