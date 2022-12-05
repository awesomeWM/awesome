--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local generic_widget_ = ...
local wibox     = require("wibox")
local beautiful = require("beautiful")

local l = wibox.widget {
    spacing = 10,
    layout  = wibox.layout.fixed.horizontal
}

local function generic_widget(txt)
    return generic_widget_(txt, nil, 0)
end

--DOC_HIDE_END
   local w = wibox.widget {
       --DOC_HIDE_START
       -- [...] Some widgets here.
       {
           text      = "none",
           row_index = 1,
           col_index = 1,
           col_span  = 3,
           widget    = generic_widget
       },
       {
           text      = "first",
           col_span = 2,
           row_index = 2,
           col_index = 1,
           widget    = generic_widget
       },
       {
           text      = "third",
           row_index = 2,
           col_index = 3,
           row_span  = 2,
           widget    = generic_widget
       },
       {
           text      = "second",
           row_index = 3,
           col_index = 1,
           col_span  = 2,
           widget    = generic_widget
       },
       {
           text      = "fourth",
           row_index = 4,
           col_index = 1,
           widget    = generic_widget
       },
       {
           text      = "fifth",
           row_index = 4,
           col_index = 2,
           col_span  = 2,
           widget    = generic_widget
       },
       {
           text      = "sixth",
           row_index = 1,
           col_index = 4,
           row_span  = 4,
           widget    = generic_widget
       },
      --DOC_HIDE_END
       homogeneous          = true,
       spacing              = 0,
       border_width         = 4,
       border_color         = beautiful.border_color,
       minimum_column_width = 10,
       minimum_row_height   = 10,
       layout               = wibox.layout.grid,
   }

   --DOC_NEWLINE
   w:add_row_border(1, 40, { color = "red"   })
   w:add_row_border(2, 5 , { color = "green" , dashes = {5, 3, 10, 3}})
   w:add_row_border(3, 10, { color = "blue"  , dashes = {5, 3, 10, 3}, dash_offset = 5})
   w:add_row_border(4, 30, { color = "orange", dashes = {5, 40}, caps = "round"})
   w:add_row_border(5, 10, { color = "yellow"})
   --DOC_NEWLINE
   w:add_column_border(1, 5, { color = "purple"})
   w:add_column_border(2, 10, { color = "cyan"})
   w:add_column_border(3, 5, { color = "magenta"})
   w:add_column_border(4, 5, { color = "black"})
   w:add_column_border(5, 10, { color = "grey"})

l:add(w) --DOC_HIDE
return l, l:fit({dpi=96}, 9999, 9999) --DOC_HIDE
