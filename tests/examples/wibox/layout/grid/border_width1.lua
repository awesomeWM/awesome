--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local generic_widget_ = ...
local wibox     = require("wibox")

local l = wibox.widget {
    spacing = 10,
    layout  = wibox.layout.fixed.vertical
}

local function generic_widget(txt)
    return generic_widget_(txt, nil, 0)
end

--DOC_HIDE_END

   for _, expand in ipairs { true, false } do
      for _, homogeneous in ipairs { true, false } do
          --DOC_HIDE_START
          local row = wibox.widget {
              spacing = 10,
              layout  = wibox.layout.fixed.horizontal
          }
          l:add(wibox.widget.textbox(
              "<b>`expand = "..tostring(expand).."`, `homogeneous = "..tostring(homogeneous).."`</b>:"
          ))
          l:add(row)
          --DOC_HIDE_END
          for _, width in ipairs { 0, 1, 2, 4, 10 } do
              local w = wibox.widget {
                  generic_widget( "first"  ),
                  generic_widget( "second" ),
                  generic_widget( "third"  ),
                  generic_widget( "fourth" ),
                  generic_widget( "fifth" ),
                  generic_widget( "sixth" ),
                  forced_num_cols = 2,
                  forced_num_rows = 2,
                  homogeneous     = homogeneous,
                  spacing         = 10,
                  border_width    = {
                      inner = width,
                      outer = 1.5 * width,
                  },
                  border_color    = "red",
                  expand          = expand,
                  forced_height   = expand and 200 or nil,
                  layout          = wibox.layout.grid,
              }

              --DOC_HIDE_START
              row:add(wibox.widget {
                  wibox.widget.textbox("<i> `border_width = ".. width .."`</i>:    "),
                  w,
                  layout  = wibox.layout.fixed.vertical
              })
              --DOC_HIDE_END
          end
      end
   end
--DOC_HIDE_START

return l, l:fit({dpi=96}, 9999, 9999)
