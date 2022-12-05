--DOC_GEN_IMAGE --DOC_HIDE_START
local generic_widget = ... --DOC_NO_USAGE
local wibox     = require("wibox")
local beautiful = require("beautiful")

--DOC_HIDE_END

   local lorem  = "Lorem ipsum dolor sit amet, consectetur adipiscing " ..
       "elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."

--DOC_NEWLINE

   local l = wibox.widget {
       {
           text      = lorem,
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
       homogeneous          = true,
       spacing              = 5,
       border_width         = 1,
       border_color         = beautiful.border_color,
       minimum_column_width = 10,
       minimum_row_height   = 10,
       layout               = wibox.layout.grid,
   }

return l, l:fit({dpi=96}, 400, 200) --DOC_HIDE
