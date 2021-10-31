--DOC_GEN_IMAGE --DOC_HIDE_START
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

local ret = wibox.layout.fixed.vertical()

--DOC_HIDE_END
for _, wrap in ipairs {"word", "char", "word_char"} do
    local w = wibox.widget {
        wrap   = wrap,
        text   = "Notable dinausors: Tyrannosaurus-Rex, Triceratops, Velociraptor, Sauropods, Archaeopteryx.",
        widget = wibox.widget.textbox,
    }

    --DOC_HIDE_START

    ret:add(
        wibox.widget {
            markup = "Wrap: <b>" .. wrap .."</b>",
            widget = wibox.widget.textbox
        },
        wibox.widget{
            w,
            margins       = 1,
            color         = beautiful.bg_normal,
            forced_width  = 85,
            forced_height = 70,
            widget        = wibox.container.margin,
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
