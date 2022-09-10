--DOC_GEN_IMAGE  --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")

local l = wibox.layout.fixed.horizontal()
l.spacing = 5

--DOC_HIDE_END
for _, value in ipairs { 0, 0.2, 0.5, 1 } do

    local pb = --DOC_HIDE
    wibox.widget {
        value  = value,
        widget = wibox.widget.progressbar,
    }

    --DOC_HIDE_START
    l:add(wibox.widget {
        pb,
        {
            text   = value,
            halign = "center",
            widget = wibox.widget.textbox,
        },
        forced_height = 30,
        forced_width  = 75,
        layout = wibox.layout.stack
    })
    --DOC_HIDE_END
end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
