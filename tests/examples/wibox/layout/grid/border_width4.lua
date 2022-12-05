--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local generic_widget_ = ...
local wibox     = require("wibox")
local gears     = { color = require("gears.color") }

local l = wibox.widget {
    spacing = 10,
    layout  = wibox.layout.fixed.horizontal
}

local function generic_widget(txt)
    return generic_widget_(txt, nil, 0)
end

--DOC_HIDE_END
   for _, width in ipairs { 0, 1, 2, 4, 10 } do
       local w = wibox.widget {
           generic_widget( "first"  ),
           generic_widget( "second" ),
           generic_widget( "third"  ),
           generic_widget( "fourth" ),
           forced_num_cols = 2,
           forced_num_rows = 2,
           homogeneous     = true,
           spacing         = 10,
           border_width    = {
               inner = width,
               outer = 10 - width,
           },
           border_color    = {
               inner = gears.color {
                    type  = "linear",
                    from  = { 0  , 0 },
                    to    = { 100, 100 },
                    stops = {
                        { 0, "#ff0000" },
                        { 1, "#ffff00" },
                    }
                },
               outer = gears.color {
                    type  = "linear",
                    from  = { 0  , 0 },
                    to    = { 100, 100 },
                    stops = {
                        { 0, "#0000ff" },
                        { 1, "#ff0000" },
                    }
                },
           },
           layout          = wibox.layout.grid,
       }

       --DOC_HIDE_START
       l:add(wibox.widget {
           wibox.widget.textbox(
               "<b>"..
               "`border_width.outer = ".. (10-width) .."`\n"..
               "`border_width.inner = ".. width .."`"..
               "</b>:"),
           w,
           layout  = wibox.layout.fixed.vertical
       })
       --DOC_HIDE_STOP
   end
--DOC_HIDE_START

return l, l:fit({dpi=96}, 9999, 9999)
