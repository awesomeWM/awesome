--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local generic_widget_ = ...
local wibox     = require("wibox") --DOC_HIDE

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
           column_count = 2,
           row_count    = 2,
           homogeneous  = true,
           spacing      = width,
           border_width = 1,
           border_color = "red",
           layout       = wibox.layout.grid,
       }

       --DOC_HIDE_START
       l:add(wibox.widget {
           wibox.widget.textbox("<b> `spacing = ".. width .."`</b>:                 "),
           w,
           layout  = wibox.layout.fixed.vertical
       })
       --DOC_HIDE_END
   end
--DOC_HIDE_START

return l, l:fit({dpi=96}, 9999, 9999)
