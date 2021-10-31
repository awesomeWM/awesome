--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

local ret = wibox.layout.fixed.vertical()

--DOC_HIDE_END

   for _, font in ipairs { "sans 8", "sans 10", "sans 12", "sans 14" } do
       local w = wibox.widget {
           font   = font,
           text   = "The quick brown fox jumps over the lazy dog!",
           widget = wibox.widget.textbox,
       }

    --DOC_HIDE_START

    ret:add(
        wibox.widget {
            markup = "Font: <b>" .. font .."</b>",
            widget = wibox.widget.textbox
        },
        wibox.widget{
            w,
            margins =  1,
            color = beautiful.bg_normal,
            forced_width = 500,
            widget = wibox.container.margin,
        },
        {
            forced_height = 10,
            widget = wibox.widget.base.make_widget()
        }
    )
    --DOC_HIDE_END
   end

--DOC_HIDE_START

l:add(ret)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
