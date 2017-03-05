local generic_widget = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local first  = wibox.widget(generic_widget( "first"  )) --DOC_HIDE
local second = wibox.widget(generic_widget( "second" )) --DOC_HIDE
local third  = wibox.widget(generic_widget( "t\nh\ni\nr\nd"  )) --DOC_HIDE
local fourth = wibox.widget(generic_widget( "fourth" )) --DOC_HIDE
local fifth  = wibox.widget(generic_widget( "fifth"  )) --DOC_HIDE
local lorem  = wibox.widget(generic_widget("Lorem ipsum dolor sit amet, consectetur " ..   --DOC_HIDE
    "adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."))  --DOC_HIDE

    local l = wibox.widget {
        homogeneous   = true,
        spacing       = 5,
        min_cols_size = 10,
        min_rows_size = 10,
        layout        = wibox.layout.grid,
    }
    l:add_widget_at(first , 2, 1, 1, 2)
    l:add_widget_at(second, 3, 1, 1, 2)
    l:add_widget_at(third , 2, 3, 2, 1)
    l:add_widget_at(fourth, 4, 1, 1, 1)
    l:add_widget_at(fifth , 4, 2, 1, 2)
    l:insert_row(1)
    l:add_widget_at(lorem, 1, 1, 1, 3)

return l, l:fit({dpi=96}, 300, 200) --DOC_HIDE
