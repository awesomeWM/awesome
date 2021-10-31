--DOC_GEN_IMAGE --DOC_HIDE_START
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

local ret = wibox.layout.fixed.vertical()

--DOC_HIDE_END
for _, valign in ipairs {"top", "center", "bottom"} do
    local w = wibox.widget {
        valign = valign,
        text   = "some text",
        widget = wibox.widget.textbox,
        forced_height = 50, --DOC_HIDE
    }

    --DOC_HIDE_START

    ret:add(
        wibox.widget {
            markup = "Alignement: <b>" .. valign .."</b>",
            widget = wibox.widget.textbox
        },
        wibox.widget{
            w,
            margins =  1,
            color = beautiful.bg_normal,
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

return 200, nil

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
